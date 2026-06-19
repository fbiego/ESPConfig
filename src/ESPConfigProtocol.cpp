#include "ESPConfigProtocol.h"

namespace ESPConfig {
namespace Protocol {

uint16_t crc16(const uint8_t* data, size_t length) {
    uint16_t crc = 0xffff;
    for (size_t i = 0; i < length; ++i) {
        crc ^= static_cast<uint16_t>(data[i]) << 8;
        for (uint8_t bit = 0; bit < 8; ++bit) {
            crc = (crc & 0x8000) ? static_cast<uint16_t>((crc << 1) ^ 0x1021)
                                 : static_cast<uint16_t>(crc << 1);
        }
    }
    return crc;
}

Payload frame(PacketType type, const uint8_t* payload, size_t length) {
    if (length > MaxPayloadLength || length > 0xffff) {
        return {};
    }

    Payload output;
    output.reserve(length + 8);
    output.push_back(MagicFirst);
    output.push_back(MagicSecond);
    output.push_back(Version);
    output.push_back(static_cast<uint8_t>(type));
    output.push_back(static_cast<uint8_t>(length & 0xff));
    output.push_back(static_cast<uint8_t>((length >> 8) & 0xff));
    output.insert(output.end(), payload, payload + length);

    const uint16_t checksum = crc16(output.data() + 2, length + 4);
    output.push_back(static_cast<uint8_t>(checksum & 0xff));
    output.push_back(static_cast<uint8_t>((checksum >> 8) & 0xff));
    return output;
}

Payload frame(PacketType type, const String& payload) {
    return frame(type, reinterpret_cast<const uint8_t*>(payload.c_str()), payload.length());
}

void appendUint32(Payload& payload, uint32_t value) {
    payload.push_back(static_cast<uint8_t>(value & 0xff));
    payload.push_back(static_cast<uint8_t>((value >> 8) & 0xff));
    payload.push_back(static_cast<uint8_t>((value >> 16) & 0xff));
    payload.push_back(static_cast<uint8_t>((value >> 24) & 0xff));
}

void appendString(Payload& payload, const String& value) {
    const size_t length = min(value.length(), static_cast<size_t>(0xffff));
    payload.push_back(static_cast<uint8_t>(length & 0xff));
    payload.push_back(static_cast<uint8_t>((length >> 8) & 0xff));
    payload.insert(payload.end(), value.c_str(), value.c_str() + length);
}

PayloadReader::PayloadReader(const Payload& payload) : _payload(payload) {
}

bool PayloadReader::readUint32(uint32_t& value) {
    if (_offset + 4 > _payload.size()) {
        return false;
    }
    value = static_cast<uint32_t>(_payload[_offset]) |
            (static_cast<uint32_t>(_payload[_offset + 1]) << 8) |
            (static_cast<uint32_t>(_payload[_offset + 2]) << 16) |
            (static_cast<uint32_t>(_payload[_offset + 3]) << 24);
    _offset += 4;
    return true;
}

bool PayloadReader::readString(String& value) {
    if (_offset + 2 > _payload.size()) {
        return false;
    }
    const size_t length = static_cast<size_t>(_payload[_offset]) |
                          (static_cast<size_t>(_payload[_offset + 1]) << 8);
    _offset += 2;
    if (_offset + length > _payload.size()) {
        return false;
    }
    value = String(reinterpret_cast<const char*>(_payload.data() + _offset), length);
    _offset += length;
    return true;
}

bool PayloadReader::finished() const {
    return _offset == _payload.size();
}

void FrameParser::reset() {
    _buffer.clear();
}

void FrameParser::feed(const uint8_t* data, size_t length, const PacketHandler& handler) {
    _buffer.insert(_buffer.end(), data, data + length);

    while (_buffer.size() >= 2) {
        if (_buffer[0] != MagicFirst || _buffer[1] != MagicSecond) {
            _buffer.erase(_buffer.begin());
            continue;
        }
        if (_buffer.size() < 6) {
            return;
        }
        if (_buffer[2] != Version) {
            _buffer.erase(_buffer.begin());
            continue;
        }

        const size_t payloadLength = static_cast<size_t>(_buffer[4]) |
                                     (static_cast<size_t>(_buffer[5]) << 8);
        if (payloadLength > MaxPayloadLength) {
            _buffer.erase(_buffer.begin());
            continue;
        }

        const size_t frameLength = payloadLength + 8;
        if (_buffer.size() < frameLength) {
            return;
        }

        const uint16_t expected = static_cast<uint16_t>(_buffer[frameLength - 2]) |
                                  (static_cast<uint16_t>(_buffer[frameLength - 1]) << 8);
        const uint16_t actual = crc16(_buffer.data() + 2, payloadLength + 4);
        if (expected != actual) {
            _buffer.erase(_buffer.begin());
            continue;
        }

        Payload payload(_buffer.begin() + 6, _buffer.begin() + 6 + payloadLength);
        handler(static_cast<PacketType>(_buffer[3]), payload);
        _buffer.erase(_buffer.begin(), _buffer.begin() + frameLength);
    }
}

} // namespace Protocol
} // namespace ESPConfig
