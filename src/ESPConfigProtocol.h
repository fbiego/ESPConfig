#pragma once

#include <Arduino.h>
#include <functional>
#include <vector>

namespace ESPConfig {
namespace Protocol {

static constexpr uint8_t MagicFirst = 0xEC;
static constexpr uint8_t MagicSecond = 0xCF;
static constexpr uint8_t Version = 1;
static constexpr size_t MaxPayloadLength = 16384;

enum class PacketType : uint8_t {
    Hello = 0x01,
    Authenticate = 0x02,
    GetSchema = 0x03,
    GetConfig = 0x04,
    SetValue = 0x10,
    ClearItems = 0x11,
    AddItem = 0x12,
    ClearPairs = 0x13,
    AddPair = 0x14,
    InvokeAction = 0x15,

    Ready = 0x80,
    Schema = 0x81,
    Config = 0x82,
    Result = 0x83,
    Error = 0x84
};

using Payload = std::vector<uint8_t>;
using PacketHandler = std::function<void(PacketType, const Payload&)>;

uint16_t crc16(const uint8_t* data, size_t length);
Payload frame(PacketType type, const uint8_t* payload, size_t length);
Payload frame(PacketType type, const String& payload);
void appendUint32(Payload& payload, uint32_t value);
void appendString(Payload& payload, const String& value);

class PayloadReader {
public:
    explicit PayloadReader(const Payload& payload);

    bool readUint32(uint32_t& value);
    bool readString(String& value);
    bool finished() const;

private:
    const Payload& _payload;
    size_t _offset = 0;
};

class FrameParser {
public:
    void reset();
    void feed(const uint8_t* data, size_t length, const PacketHandler& handler);

private:
    Payload _buffer;
};

} // namespace Protocol
} // namespace ESPConfig
