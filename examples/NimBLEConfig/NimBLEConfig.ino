#include <Arduino.h>
#include <NimBLEDevice.h>
#include <ESPConfig.h>
#include <NimBLEInterface.h>

using namespace ESPConfig;

Manager config;
NimBLEInterface* configBle = nullptr;

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(NimBLEServer *pServer, NimBLEConnInfo &connInfo) {

    }
    void onDisconnect(NimBLEServer *pServer, NimBLEConnInfo &connInfo, int reason) {
      BLEDevice::startAdvertising();
    }
};


void setup() {
  Serial.begin(115200);

  config.addString("device_name", "Device Name", "ESP32 BLE Configurator", true);
  config.addInteger("sample_interval", "Sample Interval Seconds", 60);
  config.addBoolean("enabled", "Enabled", true);
  config.addArray("channels", "Channels", ValueType::Integer);
  config.addPairArray("feature_flags", "Feature Flags", ValueType::String, ValueType::Boolean);
  config.addAction("identify", "Identify Device", []() {
    Serial.println("Identify action invoked");
  });

  // The application owns NimBLE initialization, server, advertising, and security.
  NimBLEDevice::init("ESPConfig BLE");
  NimBLEServer* server = NimBLEDevice::createServer();
  NimBLEDevice::setMTU(517);
  server->setCallbacks(new MyServerCallbacks(), false);

  configBle = new NimBLEInterface(*server, config, "ESP32 BLE demo");
  configBle->setPassword("change-me");
  if (!configBle->begin()) {
    Serial.println("Failed to start ESPConfig BLE service");
    return;
  }

  NimBLEAdvertising* advertising = NimBLEDevice::getAdvertising();
  advertising->addServiceUUID(NimBLEInterface::ServiceUuid);
  advertising->enableScanResponse(true);
  advertising->setPreferredParams(0x06, 0x12);
  advertising->setName("ESPConfig");
  advertising->start();


  Serial.println("ESPConfig BLE service is advertising");
}

void loop() {
  delay(1000);
}
