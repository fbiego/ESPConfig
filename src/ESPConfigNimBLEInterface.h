#pragma once

#include <Arduino.h>

#include "ESPConfig.h"
#include "ESPConfigProtocol.h"

#if __has_include(<NimBLEDevice.h>)
#include <NimBLEDevice.h>

namespace ESPConfig {

class ESPConfigNimBLEInterface {
public:
    static constexpr const char* ServiceUuid = "fb1e4000-54ae-4a28-9f74-dfccb248601d";
    static constexpr const char* CommandCharacteristicUuid = "fb1e4004-54ae-4a28-9f74-dfccb248601d";
    static constexpr const char* ResponseCharacteristicUuid = "fb1e4005-54ae-4a28-9f74-dfccb248601d";

    ESPConfigNimBLEInterface(NimBLEServer& server, ESPConfigManager& manager, const String& deviceName = "ESP32");
    ~ESPConfigNimBLEInterface();

    void setPassword(const String& password);
    void clearPassword();
    bool isProtected() const;
    bool isAuthenticated() const;

    bool begin();
    NimBLEService* getService() const;
    NimBLECharacteristic* getCommandCharacteristic() const;
    NimBLECharacteristic* getResponseCharacteristic() const;

private:
    class CommandCallbacks;
    static const size_t MaxNotificationChunkSize = 180;

    NimBLEServer& _server;
    ESPConfigManager& _manager;
    String _deviceName;
    String _password;
    Protocol::FrameParser _parser;
    bool _authenticated = true;
    uint8_t _failedAuthenticationAttempts = 0;
    uint32_t _authenticationBlockedUntil = 0;
    uint16_t _connectionHandle = BLE_HS_CONN_HANDLE_NONE;
    uint16_t _connectionMtu = 23;
    NimBLEService* _service = nullptr;
    NimBLECharacteristic* _commandCharacteristic = nullptr;
    NimBLECharacteristic* _responseCharacteristic = nullptr;
    CommandCallbacks* _callbacks = nullptr;

    void receive(const uint8_t* data, size_t length, uint16_t connectionHandle, uint16_t mtu);
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
#else
#error "ESPConfigNimBLEInterface requires the NimBLE-Arduino library"
#endif
