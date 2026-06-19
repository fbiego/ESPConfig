#include <Arduino.h>
#include <ESPConfig.h>
#include <SerialInterface.h>

using namespace ESPConfig;

Manager config;
SerialInterface configSerial(Serial, config, "ESP32 demo");
String activeWifiSsid;

void setup() {
  Serial.begin(115200);

  config.addString("device_name", "Device Name", "ESP32 Configurator", true);
  config.addString("wifi_ssid", "WiFi SSID", "", true);
  config.addString("wifi_password", "WiFi Password");
  config.addBoolean("wifi_enabled", "Enable WiFi", true);
  config.addInteger("update_interval", "Update Interval Seconds", 60, false, [](const String& value) {
    const int number = value.toInt();
    return number >= 10 && number <= 3600;
  });
  config.addArray("allowed_channels", "Allowed Channels", ValueType::Integer);
  config.addArray("feature_flags", "Feature Flags", ValueType::Boolean);
  config.addPairArray("wifi_credentials", "Backup WiFi Credentials");
  config.addPairArray("channel_enabled", "Enabled Channels", ValueType::Integer, ValueType::Boolean);

  config.setOnChange("wifi_ssid", [](const ConfigField& field) {
    activeWifiSsid = field.value;
    // Apply or persist activeWifiSsid here. Avoid writing to Serial in callbacks.
  });

  config.addAction("identify", "Identify Device", []() {
    // Trigger an LED, buzzer, or another identify mechanism here.
  });
  config.addAction("factory_reset", "Factory Reset", []() {
    // Erase persisted settings and restart here.
  }, true);

  // Set this before begin() to require authentication in the configurator.
  configSerial.setPassword("change-me");
  configSerial.begin();
}

void loop() {
  configSerial.update();
}
