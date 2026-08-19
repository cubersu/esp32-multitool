#include "ble_scanner.h"

#include <ArduinoJson.h>
#include <BLEAdvertisedDevice.h>
#include <BLEDevice.h>
#include <BLEScan.h>

namespace {
constexpr uint32_t kScanDurationSeconds = 5;
}  // namespace

String scanBleDevices() {
  BLEScan *scanner = BLEDevice::getScan();
  scanner->setActiveScan(true);

  BLEScanResults results = scanner->start(kScanDurationSeconds, false);

  JsonDocument dataDoc;
  dataDoc["type"] = "ble_scan";
  JsonArray devices = dataDoc["devices"].to<JsonArray>();

  int count = results.getCount();
  for (int i = 0; i < count; i++) {
    BLEAdvertisedDevice device = results.getDevice(i);
    JsonObject entry = devices.add<JsonObject>();
    entry["name"] = device.haveName() ? device.getName() : "";
    entry["address"] = device.getAddress().toString();
    entry["rssi"] = device.getRSSI();
  }

  // Tarama sonuçlarının ayırdığı belleği serbest bırak.
  scanner->clearResults();

  String json;
  serializeJson(dataDoc, json);
  return json;
}
