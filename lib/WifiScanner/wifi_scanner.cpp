#include "wifi_scanner.h"

#include <ArduinoJson.h>
#include <WiFi.h>

String scanWifiNetworks() {
  // Tarama için önce istasyon (STA) moduna geçilmeli; BLE aktifken bu mod
  // değişikliği radyo paylaşımını etkilemez, Wi-Fi ve BLE aynı çipte ayrı
  // zaman dilimlerinde çalışır.
  WiFi.mode(WIFI_STA);

  int networkCount = WiFi.scanNetworks();

  JsonDocument dataDoc;
  dataDoc["type"] = "wifi_scan";
  JsonArray networks = dataDoc["networks"].to<JsonArray>();

  for (int i = 0; i < networkCount; i++) {
    JsonObject network = networks.add<JsonObject>();
    network["ssid"] = WiFi.SSID(i);
    network["rssi"] = WiFi.RSSI(i);
    network["secure"] = WiFi.encryptionType(i) != WIFI_AUTH_OPEN;
  }

  // Tarama sonuçlarının ayırdığı belleği serbest bırak.
  WiFi.scanDelete();

  String json;
  serializeJson(dataDoc, json);
  return json;
}
