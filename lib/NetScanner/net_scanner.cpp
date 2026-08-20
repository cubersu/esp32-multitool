#include "net_scanner.h"

#include <ArduinoJson.h>
#include <WiFi.h>

void NetScanner::begin() {
  // Her scan() çağrısı kendi Wi-Fi bağlantısını kurup kapatıyor; burada ek
  // bir başlatma gerekmiyor.
}

String NetScanner::scan(const String &ssid, const String &password, const uint16_t *ports, uint8_t portCount,
                         uint32_t connectTimeoutMs, uint32_t portTimeoutMs, uint32_t maxTotalMs) {
  WiFi.mode(WIFI_STA);
  if (password.length() > 0) {
    WiFi.begin(ssid.c_str(), password.c_str());
  } else {
    WiFi.begin(ssid.c_str());
  }

  uint32_t connectStart = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - connectStart < connectTimeoutMs) {
    delay(100);
  }
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.disconnect(true);
    return "";
  }

  IPAddress localIp = WiFi.localIP();
  // IPAddress <-> uint32_t dönüşümünün iç bayt sırası önemli değil: mask,
  // network/broadcast sınırları ve host döngüsü hepsi AYNI dönüşümü
  // kullandığı için (operator uint32_t / IPAddress(uint32_t)) aritmetik
  // kendi içinde tutarlı kalıyor.
  uint32_t ipInt = static_cast<uint32_t>(localIp);
  uint32_t maskInt = static_cast<uint32_t>(WiFi.subnetMask());
  uint32_t networkInt = ipInt & maskInt;
  uint32_t broadcastInt = networkInt | (~maskInt);

  JsonDocument dataDoc;
  dataDoc["type"] = "net_scan";
  dataDoc["local_ip"] = localIp.toString();
  JsonArray hosts = dataDoc["hosts"].to<JsonArray>();

  uint32_t scanStart = millis();
  uint16_t reportedHosts = 0;
  bool timedOut = false;

  for (uint32_t hostInt = networkInt + 1; hostInt < broadcastInt && reportedHosts < kMaxHostsReported; hostInt++) {
    if (millis() - scanStart > maxTotalMs) {
      timedOut = true;
      break;
    }
    if (hostInt == ipInt) {
      continue;  // kendi IP'mizi atla
    }

    IPAddress hostIp(hostInt);
    uint16_t openPorts[kMaxPorts];
    uint8_t openCount = 0;

    for (uint8_t p = 0; p < portCount; p++) {
      WiFiClient client;
      if (client.connect(hostIp, ports[p], portTimeoutMs)) {
        openPorts[openCount++] = ports[p];
        client.stop();
      }
      if (millis() - scanStart > maxTotalMs) {
        timedOut = true;
        break;
      }
    }

    if (openCount > 0) {
      JsonObject host = hosts.add<JsonObject>();
      host["ip"] = hostIp.toString();
      JsonArray portsArray = host["open_ports"].to<JsonArray>();
      for (uint8_t i = 0; i < openCount; i++) {
        portsArray.add(openPorts[i]);
      }
      reportedHosts++;
    }

    if (timedOut) {
      break;
    }
  }
  dataDoc["timed_out"] = timedOut;

  WiFi.disconnect(true);

  String json;
  serializeJson(dataDoc, json);
  return json;
}
