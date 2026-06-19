#include "SerialInterface.h"

#include <cstdio>

namespace ESPConfig {

SerialInterface::SerialInterface(Stream& serial, Manager& manager, const String& deviceName)
    : _serial(serial), _manager(manager), _deviceName(deviceName) {
}

void SerialInterface::setPassword(const String& password) {
    _password = password;
    _authenticated = password.length() == 0;
    _failedAuthenticationAttempts = 0;
    _authenticationBlockedUntil = 0;
}

void SerialInterface::clearPassword() {
    setPassword("");
}

bool SerialInterface::isProtected() const {
    return _password.length() > 0;
}

bool SerialInterface::isAuthenticated() const {
    return _authenticated;
}

void SerialInterface::begin() {
    _parser.reset();
    _authenticated = !isProtected();
    sendReady();
}

void SerialInterface::update() {
    while (_serial.available()) {
        const uint8_t byte = static_cast<uint8_t>(_serial.read());
        _parser.feed(&byte, 1, [this](Protocol::PacketType type, const Protocol::Payload& payload) {
            processPacket(type, payload);
        });
    }
}

void SerialInterface::processPacket(Protocol::PacketType type, const Protocol::Payload& payload) {
    if (type == Protocol::PacketType::Hello) {
        if (!payload.empty()) {
            sendError("Malformed hello packet");
            return;
        }
        _authenticated = !isProtected();
        sendReady();
        return;
    }
    if (type == Protocol::PacketType::Authenticate) {
        processAuthentication(payload);
        return;
    }
    if (!_authenticated) {
        Protocol::PayloadReader reader(payload);
        uint32_t requestId = 0;
        if (reader.readUint32(requestId)) {
            sendResult(requestId, false, "Authentication required");
        } else {
            sendError("Authentication required");
        }
        return;
    }
    if (type == Protocol::PacketType::GetSchema && payload.empty()) {
        sendSchema();
        return;
    }
    if (type == Protocol::PacketType::GetConfig && payload.empty()) {
        sendConfig();
        return;
    }
    processRequest(type, payload);
}

void SerialInterface::processAuthentication(const Protocol::Payload& payload) {
    Protocol::PayloadReader reader(payload);
    uint32_t requestId = 0;
    String password;
    if (!reader.readUint32(requestId) || !reader.readString(password) || !reader.finished()) {
        sendError("Malformed authentication packet");
        return;
    }
    if (isProtected() && static_cast<int32_t>(millis() - _authenticationBlockedUntil) < 0) {
        sendResult(requestId, false, "Too many attempts; try again shortly");
        return;
    }

    const bool accepted = !isProtected() || passwordMatches(password);
    if (accepted) {
        _authenticated = true;
        _failedAuthenticationAttempts = 0;
        _authenticationBlockedUntil = 0;
    } else {
        if (_failedAuthenticationAttempts < 10) {
            ++_failedAuthenticationAttempts;
        }
        _authenticationBlockedUntil = millis() + static_cast<uint32_t>(_failedAuthenticationAttempts) * 1000;
    }
    sendResult(requestId, accepted, accepted ? "" : "Incorrect password");
}

void SerialInterface::processRequest(Protocol::PacketType type, const Protocol::Payload& payload) {
    Protocol::PayloadReader reader(payload);
    uint32_t requestId = 0;
    String key;
    if (!reader.readUint32(requestId) || !reader.readString(key)) {
        sendError("Malformed request packet");
        return;
    }

    bool ok = false;
    String message = "Unknown or malformed command";
    String first;
    String second;
    switch (type) {
        case Protocol::PacketType::SetValue:
            if (reader.readString(first) && reader.finished()) {
                ok = _manager.setFieldValue(key, first);
                message = "Value rejected";
            }
            break;
        case Protocol::PacketType::InvokeAction:
            if (reader.finished()) {
                ok = _manager.hasAction(key);
                message = "Action not found";
                sendResult(requestId, ok, message);
                if (ok) {
                    _manager.invokeAction(key);
                }
                return;
            }
            break;
        case Protocol::PacketType::ClearItems:
            if (reader.finished()) {
                ok = _manager.clearItems(key);
                message = "Array not found";
            }
            break;
        case Protocol::PacketType::AddItem:
            if (reader.readString(first) && reader.finished()) {
                ok = _manager.addItem(key, first);
                message = "Array not found or value rejected";
            }
            break;
        case Protocol::PacketType::ClearPairs:
            if (reader.finished()) {
                ok = _manager.clearPairs(key);
                message = "Pair array not found";
            }
            break;
        case Protocol::PacketType::AddPair:
            if (reader.readString(first) && reader.readString(second) && reader.finished()) {
                ok = _manager.addPair(key, first, second);
                message = "Pair array not found or value rejected";
            }
            break;
        default:
            break;
    }
    sendResult(requestId, ok, message);
}

bool SerialInterface::passwordMatches(const String& candidate) const {
    const size_t maximumLength = max(_password.length(), candidate.length());
    size_t difference = _password.length() ^ candidate.length();
    for (size_t i = 0; i < maximumLength; ++i) {
        const unsigned char expected = i < _password.length() ? _password[i] : 0;
        const unsigned char actual = i < candidate.length() ? candidate[i] : 0;
        difference |= expected ^ actual;
    }
    return difference == 0;
}

void SerialInterface::sendReady() {
    sendPacket(Protocol::PacketType::Ready,
               "{\"type\":\"ready\",\"protocol\":4,\"device\":\"" + escapeJson(_deviceName) +
               "\",\"protected\":" + (isProtected() ? "true" : "false") +
               ",\"authenticated\":" + (_authenticated ? "true" : "false") + "}");
}

void SerialInterface::sendSchema() {
    sendPacket(Protocol::PacketType::Schema,
               "{\"type\":\"schema\",\"fields\":" + _manager.schemaJson() +
               ",\"actions\":" + _manager.actionsJson() + "}");
}

void SerialInterface::sendConfig() {
    sendPacket(Protocol::PacketType::Config,
               "{\"type\":\"config\",\"values\":" + _manager.toJson() + "}");
}

void SerialInterface::sendResult(uint32_t requestId, bool ok, const String& message) {
    String packet = "{\"type\":\"result\",\"id\":" + String(requestId) +
                    ",\"ok\":" + (ok ? "true" : "false");
    if (!ok) {
        packet += ",\"message\":\"" + escapeJson(message) + "\"";
    }
    sendPacket(Protocol::PacketType::Result, packet + "}");
}

void SerialInterface::sendError(const String& message) {
    sendPacket(Protocol::PacketType::Error,
               "{\"type\":\"error\",\"message\":\"" + escapeJson(message) + "\"}");
}

void SerialInterface::sendPacket(Protocol::PacketType type, const String& json) {
    const Protocol::Payload packet = Protocol::frame(type, json);
    if (!packet.empty()) {
        _serial.write(packet.data(), packet.size());
    }
}

String SerialInterface::escapeJson(const String& value) const {
    String escaped;
    for (size_t i = 0; i < value.length(); ++i) {
        const char c = value[i];
        switch (c) {
            case '\\': escaped += "\\\\"; break;
            case '"': escaped += "\\\""; break;
            case '\b': escaped += "\\b"; break;
            case '\f': escaped += "\\f"; break;
            case '\n': escaped += "\\n"; break;
            case '\r': escaped += "\\r"; break;
            case '\t': escaped += "\\t"; break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    char sequence[7];
                    std::snprintf(sequence, sizeof(sequence), "\\u%04x", static_cast<unsigned char>(c));
                    escaped += sequence;
                } else {
                    escaped += c;
                }
                break;
        }
    }
    return escaped;
}

} // namespace ESPConfig
