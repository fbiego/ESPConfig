#pragma once

#include <Arduino.h>

#include "ESPConfig.h"
#include "ESPConfigProtocol.h"

namespace ESPConfig {

class ESPConfigSerialInterface {
public:
    ESPConfigSerialInterface(Stream& serial, ESPConfigManager& manager, const String& deviceName = "ESP32");

    void setPassword(const String& password);
    void clearPassword();
    bool isProtected() const;
    bool isAuthenticated() const;

    void begin();
    void update();

private:
    Stream& _serial;
    ESPConfigManager& _manager;
    String _deviceName;
    String _password;
    Protocol::FrameParser _parser;
    bool _authenticated = true;
    uint8_t _failedAuthenticationAttempts = 0;
    uint32_t _authenticationBlockedUntil = 0;

    void processPacket(Protocol::PacketType type, const Protocol::Payload& payload);
    void processAuthentication(const Protocol::Payload& payload);
    void processRequest(Protocol::PacketType type, const Protocol::Payload& payload);
    bool passwordMatches(const String& candidate) const;
    void sendReady();
    void sendSchema();
    void sendConfig();
    void sendResult(uint32_t requestId, bool ok, const String& message);
    void sendError(const String& message);
    void sendPacket(Protocol::PacketType type, const String& json);
    String escapeJson(const String& value) const;
};

} // namespace ESPConfig
