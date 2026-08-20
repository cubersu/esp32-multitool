#include "wifi_scanner.h"

#include <ArduinoJson.h>
#include <WiFi.h>

namespace {
// MAC adreslerini karşılaştırırken büyük/küçük harf farkını yok say
// (kullanıcı BSSID'yi telefonda küçük harfle yazmış olabilir).
bool equalsIgnoreCase(const String &a, const String &b) {
  return a.equalsIgnoreCase(b);
}
}  // namespace

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

String scanForRogueAp(const String &ssid, const String &knownBssid) {
  WiFi.mode(WIFI_STA);

  int networkCount = WiFi.scanNetworks();

  JsonDocument dataDoc;
  dataDoc["type"] = "rogue_ap_scan";
  dataDoc["ssid"] = ssid;
  JsonArray accessPoints = dataDoc["access_points"].to<JsonArray>();

  bool suspicious = false;
  int matchCount = 0;
  for (int i = 0; i < networkCount; i++) {
    if (WiFi.SSID(i) != ssid) {
      continue;
    }
    matchCount++;

    String bssid = WiFi.BSSIDstr(i);
    bool isKnown = knownBssid.length() > 0 && equalsIgnoreCase(bssid, knownBssid);
    if (knownBssid.length() > 0 && !isKnown) {
      suspicious = true;
    }

    JsonObject ap = accessPoints.add<JsonObject>();
    ap["bssid"] = bssid;
    ap["rssi"] = WiFi.RSSI(i);
    ap["secure"] = WiFi.encryptionType(i) != WIFI_AUTH_OPEN;
    ap["is_known"] = isKnown;
  }
  if (matchCount > 1) {
    suspicious = true;
  }
  dataDoc["suspicious"] = suspicious;

  WiFi.scanDelete();

  String json;
  serializeJson(dataDoc, json);
  return json;
}
