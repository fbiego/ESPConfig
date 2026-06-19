# ESPConfig Documentation

ESPConfig separates a transport-independent configuration registry from Serial
and NimBLE transports. The external page in [`web/index.html`](web/index.html)
discovers the registry and renders the configuration interface.

## Configuration Registry

Create one manager and register fields before starting a transport:

```cpp
ESPConfig::Manager config;

config.addString("device_name", "Device name", "Sensor", true);
config.addInteger("sample_seconds", "Sample interval", 60);
config.addBoolean("enabled", "Enabled", true);
```

The final boolean argument marks a field as required. Integer and scalar fields
can also receive validators.

Use `setOnChange()` to apply or persist changes:

```cpp
config.setOnChange("sample_seconds", [](const ESPConfig::ConfigField& field) {
  const int updatedValue = field.intValue;
  // Apply or persist updatedValue.
});
```

The callback runs after the new value has been validated and stored. Its
`ConfigField` parameter contains the updated value:

- String: `field.value`
- Integer: `field.intValue`
- Boolean: `field.boolValue`
- Array: `field.items`
- Pair array: `field.pairs`

No-argument callbacks remain supported for code that does not need the value.

ESPConfig intentionally does not select a persistence mechanism.

## Typed Arrays

Arrays support string, integer, and boolean values:

```cpp
config.addArray("names", "Names");
config.addArray("ports", "Ports", ESPConfig::ValueType::Integer);
config.addArray("flags", "Flags", ESPConfig::ValueType::Boolean);
```

Integer and boolean values are validated and emitted as their proper JSON types.

## Typed Pair Arrays

Pair arrays default to string/string values:

```cpp
config.addPairArray("headers", "HTTP headers");
```

Each side can instead use a different type:

```cpp
config.addPairArray("channel_enabled", "Enabled channels",
                    ESPConfig::ValueType::Integer,
                    ESPConfig::ValueType::Boolean);
```

`PairValueType` remains available as a compatibility alias for `ValueType`.

## Device Actions

Actions appear as buttons in the external configurator:

```cpp
config.addAction("identify", "Identify device", identifyDevice);
config.addAction("factory_reset", "Factory reset", factoryReset, true);
```

The final `true` asks the browser to show a confirmation prompt. The ESP32
acknowledges a valid action before invoking its callback, allowing callbacks to
restart or otherwise interrupt the device connection.

## Password Protection

Set a password on the selected transport before calling `begin()`:

```cpp
configSerial.setPassword("replace-with-device-password");
configSerial.begin();
```

The same methods are available on `NimBLEInterface`:

- `setPassword(password)`
- `clearPassword()`
- `isProtected()`
- `isAuthenticated()`

The schema, values, writes, arrays, pairs, and actions remain inaccessible until
authentication succeeds. New `HELLO` sessions and device restarts relock the
transport. Failed attempts receive a progressively longer cooldown.

This provides protocol access control, not encrypted transport. Serial traffic
can be monitored. For BLE, configure NimBLE pairing and encryption in the
application.

## Serial Transport

```cpp
#include <ESPConfig.h>
#include <SerialInterface.h>

ESPConfig::Manager config;
ESPConfig::SerialInterface configSerial(Serial, config, "My ESP32");

void setup() {
  Serial.begin(115200);
  // Register fields and actions.
  configSerial.begin();
}

void loop() {
  configSerial.update();
}
```

Only `SerialInterface` should read from its selected serial stream. Avoid
writing application logs to that stream because they can interrupt a binary
frame. Use another UART or disable logs while a configuration client is active.

## NimBLE Transport

Install `NimBLE-Arduino` before using the optional BLE transport.

`NimBLEInterface` only adds the service and required characteristics to a
user-provided `NimBLEServer`. The application owns NimBLE initialization,
server creation, advertising, connection callbacks, and BLE security.

```cpp
#include <NimBLEDevice.h>
#include <NimBLEInterface.h>

NimBLEDevice::init("ESPConfig My ESP32");
NimBLEServer* server = NimBLEDevice::createServer();

ESPConfig::NimBLEInterface configBle(*server, config, "My ESP32");
configBle.setPassword("configuration-password");
configBle.begin();

NimBLEAdvertising* advertising = NimBLEDevice::getAdvertising();
advertising->enableScanResponse(true);
advertising->addServiceUUID(ESPConfig::NimBLEInterface::ServiceUuid);
advertising->setName("ESPConfig My ESP32");
advertising->start();
```

See [`examples/NimBLEConfig/NimBLEConfig.ino`](examples/NimBLEConfig/NimBLEConfig.ino)
for a complete example with advertising diagnostics.

### Current BLE UUIDs

These values match [`NimBLEInterface.h`](src/NimBLEInterface.h) and the external
configurator:

```text
Service:  fb1e4000-54ae-4a28-9f74-dfccb248601d
Command:  fb1e4004-54ae-4a28-9f74-dfccb248601d  write/write-without-response
Response: fb1e4005-54ae-4a28-9f74-dfccb248601d  read/notify
```

Clients may split commands across BLE writes. Responses may be split across
notifications according to the negotiated MTU and must be reassembled through
the frame length in the binary header. `NimBLEInterface` maintains one active
configuration client session; commands from a different connection start a new
locked session.

## External Configurator

Serve the repository locally:

```sh
python3 -m http.server 8000
```

Open `http://localhost:8000/web/` in a supported Chromium-based browser.
Web Serial and Web Bluetooth require a secure context; localhost is accepted.

The page provides:

- **Connect Serial**
- **Connect BLE**, showing the unfiltered Bluetooth device list
- Dynamic field, array, pair, and action rendering
- Password prompt for protected transports
- Connection and protocol log
- Automatic handshake retries
- Automatic BLE GATT and primary-service discovery retries

Missing `ready`, `schema`, or `config` responses are retried up to three times.
BLE GATT/service discovery is also retried up to three times without reopening
the device chooser. Disconnecting clears the loaded configuration and actions
while retaining the connection log.

## Wire Protocol

Serial and BLE use the same binary protocol. Every packet has this layout:

```text
Offset  Size  Description
0       1     Magic byte 0xEC
1       1     Magic byte 0xCF
2       1     Frame version 0x01
3       1     Packet type
4       2     Payload length, unsigned little-endian
6       N     Payload
6 + N   2     CRC-16/CCITT-FALSE, little-endian
```

The CRC covers the version, packet type, payload length, and payload. It does
not include the two magic bytes. Maximum payload length is 16384 bytes.

Command packet types:

```text
0x01 HELLO        empty payload
0x02 AUTH         request-id + password
0x03 GET_SCHEMA   empty payload
0x04 GET_CONFIG   empty payload
0x10 SET_VALUE    request-id + key + value
0x11 CLEAR_ITEMS  request-id + key
0x12 ADD_ITEM     request-id + key + value
0x13 CLEAR_PAIRS  request-id + key
0x14 ADD_PAIR     request-id + key + first + second
0x15 ACTION       request-id + action key
```

Response packet types are `0x80` ready, `0x81` schema, `0x82` config, `0x83`
result, and `0x84` error. Response payloads are UTF-8 JSON. Command payloads use
a four-byte little-endian request ID followed by zero or more strings. Each
string is encoded as a two-byte little-endian byte length followed by its UTF-8
bytes. Tabs, newlines, and Unicode therefore need no escaping or Base64
conversion.

## Troubleshooting

### BLE Device Is Not Listed

The browser chooser accepts all nearby BLE devices. Verify the ESP32 serial
monitor reports successful advertising:

```text
ESPConfig service UUID advertised: yes
ESPConfig name advertised: yes
ESPConfig advertising started: yes
```

### BLE Primary Service Discovery Fails

The page reconnects and retries service/characteristic discovery three times.
Confirm the firmware and HTML use the same current UUIDs listed above.

### Configuration Does Not Load

Open the connection log. The page retries missing `ready`, `schema`, and
`config` responses three times before reporting which packets remain missing.
