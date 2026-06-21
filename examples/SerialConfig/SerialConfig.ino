#include <Arduino.h>
#include <ESPConfig.h>
#include <ESPConfigSerialInterface.h>

using namespace ESPConfig;

ESPConfigManager config;
ESPConfigSerialInterface configSerial(Serial, config, "ESP32 demo");
String activeWifiSsid;
String lastChangedKey;

void handleWifiSsidChanged(const ConfigField& field) {
  activeWifiSsid = field.value;
  // Apply or persist activeWifiSsid here. Avoid writing to Serial in callbacks.
}

void handleAnyConfigChanged(const ConfigField& field) {
  lastChangedKey = field.key;
  // Persist the changed field or mark the complete configuration as dirty.
}

void setup() {
  Serial.begin(115200);

  config.addString("device_name", "Device Name", "ESP32 Configurator", true);
  config.addString("wifi_ssid", "WiFi SSID", "", true);
  config.addString("wifi_password", "WiFi Password");
  config.addBoolean("wifi_enabled", "Enable WiFi", true);
  config.addString("accent_color", "Accent Color", "#087f68");
  config.addInteger("update_interval", "Update Interval Seconds", 60, false, [](const String& value) {
    const int number = value.toInt();
    return number >= 10 && number <= 3600;
  });
  config.addArray("allowed_channels", "Allowed Channels", ValueType::Integer);
  config.addArray("feature_flags", "Feature Flags", ValueType::Boolean);
  config.addPairArray("wifi_credentials", "Backup WiFi Credentials");
  config.addPairArray("channel_enabled", "Enabled Channels", ValueType::Integer, ValueType::Boolean);

  config.setSwitch("wifi_enabled");
  config.setColorPicker("accent_color");
  config.setSlider("update_interval", 10, 3600, 10);
  config.setImmediateUpdate("wifi_enabled");
  config.setImmediateUpdate("accent_color");
  config.setImmediateUpdate("update_interval");

  config.setOnChange("wifi_ssid", handleWifiSsidChanged);
  config.setOnChange(handleAnyConfigChanged);

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
