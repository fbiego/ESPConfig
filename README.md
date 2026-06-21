# ESPConfig

ESPConfig exposes configurable ESP32 properties and device actions to an
external browser configurator. The ESP32 does not host a webpage.

Current library version: **1.1.0**

Supported transports:

- USB serial through `ESPConfigSerialInterface`
- Bluetooth Low Energy through optional NimBLE `ESPConfigNimBLEInterface`
- Shared framed binary protocol with CRC-16

Supported configuration types:

- String, integer, and boolean fields
- Text, number, slider, switch, and color controls
- Typed arrays
- Typed pair arrays
- Device action buttons
- Optional password protection

## Quick Start

```cpp
#include <ESPConfig.h>
#include <ESPConfigSerialInterface.h>

ESPConfig::ESPConfigManager config;
ESPConfig::ESPConfigSerialInterface configSerial(Serial, config, "My ESP32");

void setup() {
  Serial.begin(115200);

  config.addString("device_name", "Device name", "Sensor");
  config.addInteger("sample_seconds", "Sample interval", 60);
  config.addBoolean("enabled", "Enabled", true);

  configSerial.begin();
}

void loop() {
  configSerial.update();
}
```

Open the hosted configurator:

<https://fbiego.com/espconfig/>

The page displays its own version and detects the connected ESPConfig library
and protocol versions during the handshake.

Alternatively, serve it locally for development:

```sh
python3 -m http.server 8000
```

Open `http://localhost:8000/web/`, then select **Connect Serial** or
**Connect BLE**.

## Examples

- [`SerialConfig`](examples/SerialConfig/SerialConfig.ino)
- [`NimBLEConfig`](examples/NimBLEConfig/NimBLEConfig.ino)

## Documentation

See [`DOCS.md`](DOCS.md) for the complete API, NimBLE setup, current BLE UUIDs,
wire protocol, password protection, browser behavior, and troubleshooting.
