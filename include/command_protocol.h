#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>

#include "ble_manager.h"
#include "ble_scanner.h"
#include "buzzer.h"
#include "net_scanner.h"
#include "subghz_manager.h"
#include "wifi_deauth.h"
#include "wifi_scanner.h"
#include "wifi_sniffer.h"
#include "wps_check.h"

// BleManager, SubGhzManager, Buzzer, WifiSniffer, WifiDeauth, NetScanner ve
// WpsCheck, setup() içinde bir kez begin() çağrılan durumlu (stateful)
// modüller oldukları için main.cpp'de tanımlanır; burada yalnızca referans
// ediliyor. bleManager'a doğrudan erişim yalnızca wifi_capture'ın parçalı
// (chunked) bildirim göndermesi için gerekiyor — normal komutlar hâlâ tek
// bir dönüş değeriyle cevap veriyor, BleManager::CommandCharCallbacks bunu
// otomatik notify ediyor.
extern BleManager bleManager;
extern SubGhzManager subGhzManager;
extern Buzzer buzzer;
extern WifiSniffer wifiSniffer;
extern WifiDeauth wifiDeauth;
extern NetScanner netScanner;
extern WpsCheck wpsCheck;

// OLED'in hangi modda (tam menü/durum ekranı/yok) aktif olduğuna göre
// davranan sarmalayıcılar; main.cpp'de tanımlı.
extern void showOledStatus(const String &title, const String &body);
extern void showPhoneMessage(const String &text);
extern void showSubGhzProgress(uint16_t pulseCount, uint32_t elapsedMs);

// Verilen mesajla genel bir hata yanıtı üretir.
inline String buildErrorResponse(const char *message) {
  JsonDocument responseDoc;
  responseDoc["status"] = "error";
  responseDoc["msg"] = message;

  String response;
  serializeJson(responseDoc, response);
  return response;
}

// Bilinmeyen/geçersiz komutlar için ortak hata yanıtı.
inline String buildUnknownCommandResponse() {
  return buildErrorResponse("unknown command");
}

// dataJson, zaten tam bir JSON değeri (nesne ya da dizi) olan bir string'i
// yanıtın "data" alanına, tekrar escape etmeden gömer.
inline String buildDataResponse(const String &dataJson) {
  JsonDocument responseDoc;
  responseDoc["status"] = "ok";
  responseDoc["data"] = serialized(dataJson);

  String response;
  serializeJson(responseDoc, response);
  return response;
}

// Gelen komutları JSON olarak işleyip yine JSON formatında yanıt üreten
// header-only yardımcı fonksiyon. BLE katmanından bağımsızdır; sadece
// string alır, string döner. Böylece BLE ve komut mantığı birbirinden
// ayrı test edilebilir/değiştirilebilir.
inline String processCommand(const String &input) {
  JsonDocument requestDoc;

  // Gelen veri geçerli bir JSON değilse bilinmeyen komut gibi davran
  DeserializationError parseError = deserializeJson(requestDoc, input);
  if (parseError) {
    return buildUnknownCommandResponse();
  }

  const char *cmd = requestDoc["cmd"];
  if (cmd == nullptr) {
    return buildUnknownCommandResponse();
  }

  if (strcmp(cmd, "ping") == 0) {
    buzzer.beep();
    showOledStatus("Ping", "PONG");
    return "{\"status\":\"ok\",\"data\":\"pong\"}";
  }

  if (strcmp(cmd, "wifi_scan") == 0) {
    showOledStatus("Wi-Fi", "Wifi taranıyor...");
    String data = scanWifiNetworks();
    showOledStatus("Wi-Fi", "Tarama tamamlandı");
    return buildDataResponse(data);
  }

  if (strcmp(cmd, "ble_scan") == 0) {
    showOledStatus("BLE", "BLE taranıyor...");
    String data = scanBleDevices();
    showOledStatus("BLE", "Tarama tamamlandı");
    return buildDataResponse(data);
  }

  if (strcmp(cmd, "subghz_capture") == 0) {
    // İsteğe bağlı "frequency_hz" (vars. 433.92MHz) ve "timeout_ms" (vars.
    // 15sn, güvenlik için en fazla 60sn) alanları.
    long frequencyHz = requestDoc["frequency_hz"] | 433920000L;
    uint32_t timeoutMs = requestDoc["timeout_ms"] | 15000;
    if (timeoutMs > 60000) {
      timeoutMs = 60000;
    }

    subGhzManager.setFrequencyMhz(frequencyHz / 1000000.0f);
    String pulsesBase64 = subGhzManager.captureSignal(timeoutMs, showSubGhzProgress);
    if (pulsesBase64.length() == 0) {
      showOledStatus("Sub-GHz", "Sinyal yok");
      return buildErrorResponse("no signal captured");
    }
    showOledStatus("Sub-GHz", "Yakalandı!");

    JsonDocument dataDoc;
    dataDoc["type"] = "subghz_capture";
    dataDoc["frequency_hz"] = frequencyHz;
    dataDoc["pulses_b64"] = pulsesBase64;

    String dataJson;
    serializeJson(dataDoc, dataJson);
    return buildDataResponse(dataJson);
  }

  if (strcmp(cmd, "wifi_capture") == 0) {
    // "channel" (1-13, Wi-Fi kanalı) zorunlu; "timeout_ms" isteğe bağlı
    // (vars. 15sn, güvenlik için en fazla 60sn).
    int channel = requestDoc["channel"] | 1;
    if (channel < 1 || channel > 13) {
      return buildErrorResponse("channel must be 1-13");
    }
    uint32_t timeoutMs = requestDoc["timeout_ms"] | 15000;
    if (timeoutMs > 60000) {
      timeoutMs = 60000;
    }

    showOledStatus("Wi-Fi Yakala", "Dinleniyor...");
    String pcapBase64 = wifiSniffer.captureHandshake(static_cast<uint8_t>(channel), timeoutMs);
    if (pcapBase64.length() == 0) {
      showOledStatus("Wi-Fi Yakala", "Paket yok");
      return buildErrorResponse("no packets captured");
    }
    showOledStatus("Wi-Fi Yakala", "Yakalandı!");

    // PCAP verisi (base64 kodlanmış) tek bir BLE bildirimine sığmayacak
    // kadar büyük olabilir; parçalara bölüp her birini ayrı notify ile
    // gönderiyoruz. kChunkSize donanımla doğrulanmamış, muhafazakâr bir
    // tahmin — MTU pazarlığı düşük bir değerde kalırsa bile sığması
    // hedeflendi (bkz. ble_manager.cpp'deki MTU notu).
    constexpr int kChunkSize = 200;
    int totalLength = pcapBase64.length();
    int totalChunks = (totalLength + kChunkSize - 1) / kChunkSize;
    uint32_t captureId = millis();
    uint16_t packetCount = wifiSniffer.lastPacketCount();

    for (int i = 0; i < totalChunks; i++) {
      int start = i * kChunkSize;
      int len = (start + kChunkSize > totalLength) ? (totalLength - start) : kChunkSize;

      JsonDocument chunkDoc;
      chunkDoc["type"] = "wifi_capture_chunk";
      chunkDoc["capture_id"] = captureId;
      chunkDoc["seq"] = i;
      chunkDoc["total"] = totalChunks;
      // packet_count her parçada tekrarlanıyor (redundant ama basit); Android
      // tarafı yalnızca birleştirme bitince (son parçada) kullanıyor.
      chunkDoc["packet_count"] = packetCount;
      chunkDoc["chunk_b64"] = pcapBase64.substring(start, start + len);

      String chunkDataJson;
      serializeJson(chunkDoc, chunkDataJson);
      bleManager.notifyResponse(buildDataResponse(chunkDataJson));
      delay(20);  // BLE bildirim kuyruğunu taşırmamak için küçük bir ara.
    }

    JsonDocument summaryDoc;
    summaryDoc["type"] = "wifi_capture_summary";
    summaryDoc["capture_id"] = captureId;
    summaryDoc["total_chunks"] = totalChunks;
    summaryDoc["packet_count"] = packetCount;

    String summaryJson;
    serializeJson(summaryDoc, summaryJson);
    return buildDataResponse(summaryJson);
  }

  if (strcmp(cmd, "wifi_deauth") == 0) {
    // "bssid" (kendi erişim noktan) zorunlu — otomatik/keşif tabanlı bir
    // hedef seçimi kasıtlı olarak yok, bkz. wifi_deauth.h. "client_mac"
    // isteğe bağlı (vars. yayın adresi FF:FF:FF:FF:FF:FF — BSSID'ye bağlı
    // tüm istemciler). "count" en fazla WifiDeauth::kMaxFrameCount'a
    // kırpılır (sendDeauth() içinde).
    const char *bssidStr = requestDoc["bssid"];
    if (bssidStr == nullptr) {
      return buildErrorResponse("missing bssid");
    }
    uint8_t bssid[6];
    if (!WifiDeauth::parseMacAddress(bssidStr, bssid)) {
      return buildErrorResponse("invalid bssid");
    }

    const char *clientMacStr = requestDoc["client_mac"] | "FF:FF:FF:FF:FF:FF";
    uint8_t clientMac[6];
    if (!WifiDeauth::parseMacAddress(clientMacStr, clientMac)) {
      return buildErrorResponse("invalid client_mac");
    }

    int channel = requestDoc["channel"] | 1;
    if (channel < 1 || channel > 13) {
      return buildErrorResponse("channel must be 1-13");
    }
    int count = requestDoc["count"] | 10;
    if (count < 1) {
      count = 1;
    }

    showOledStatus("Deauth", "Gonderiliyor...");
    uint16_t sent =
        wifiDeauth.sendDeauth(bssid, clientMac, static_cast<uint8_t>(channel), static_cast<uint16_t>(count));
    showOledStatus("Deauth", String(sent) + " cerceve");

    JsonDocument dataDoc;
    dataDoc["type"] = "wifi_deauth";
    dataDoc["bssid"] = bssidStr;
    dataDoc["client_mac"] = clientMacStr;
    dataDoc["frames_sent"] = sent;

    String dataJson;
    serializeJson(dataDoc, dataJson);
    return buildDataResponse(dataJson);
  }

  if (strcmp(cmd, "rogue_ap_scan") == 0) {
    // "ssid" zorunlu (izlenecek ağ adı). "known_bssid" isteğe bağlı — kendi
    // gerçek erişim noktanın MAC'i verilirse ona eşleşmeyen her BSSID
    // şüpheli işaretlenir; verilmezse yalnızca o SSID'yi yayınlayan tüm
    // BSSID'ler listelenir (birden fazlaysa zaten şüpheli demektir).
    // Tamamen pasif tarama — hiçbir şey yayınlamaz.
    const char *ssid = requestDoc["ssid"];
    if (ssid == nullptr) {
      return buildErrorResponse("missing ssid");
    }
    const char *knownBssid = requestDoc["known_bssid"] | "";

    showOledStatus("Sahte AP Tarama", "Taranıyor...");
    String data = scanForRogueAp(String(ssid), String(knownBssid));
    showOledStatus("Sahte AP Tarama", "Tamamlandı");
    return buildDataResponse(data);
  }

  if (strcmp(cmd, "wps_check") == 0) {
    // "bssid" ve "channel" (1-13) zorunlu — belirli bir AP'nin beacon'ını
    // pasif olarak dinleyip WPS bilgi elemanının var olup olmadığını
    // kontrol eder. Bu bir PIN kaba kuvvet aracı DEĞİL, yalnızca "WPS açık
    // mı" keşfi — bkz. wps_check.h üstündeki kapsam notu.
    const char *bssidStr = requestDoc["bssid"];
    if (bssidStr == nullptr) {
      return buildErrorResponse("missing bssid");
    }
    uint8_t bssid[6];
    if (!WifiDeauth::parseMacAddress(bssidStr, bssid)) {
      return buildErrorResponse("invalid bssid");
    }
    int channel = requestDoc["channel"] | 1;
    if (channel < 1 || channel > 13) {
      return buildErrorResponse("channel must be 1-13");
    }
    uint32_t timeoutMs = requestDoc["timeout_ms"] | 10000;
    if (timeoutMs > 30000) {
      timeoutMs = 30000;
    }

    showOledStatus("WPS Kontrolü", "Dinleniyor...");
    bool wpsEnabled = false;
    bool found = wpsCheck.checkWps(bssid, static_cast<uint8_t>(channel), timeoutMs, &wpsEnabled);
    if (!found) {
      showOledStatus("WPS Kontrolü", "Beacon yok");
      return buildErrorResponse("no beacon seen from this bssid/channel");
    }
    showOledStatus("WPS Kontrolü", wpsEnabled ? "WPS AÇIK" : "WPS kapalı");

    JsonDocument dataDoc;
    dataDoc["type"] = "wps_check";
    dataDoc["bssid"] = bssidStr;
    dataDoc["wps_enabled"] = wpsEnabled;

    String dataJson;
    serializeJson(dataDoc, dataJson);
    return buildDataResponse(dataJson);
  }

  if (strcmp(cmd, "net_scan") == 0) {
    // "ssid" zorunlu (katılınacak kendi ağın); "password" isteğe bağlı
    // (açık ağ için boş bırak). "ports" isteğe bağlı bir tamsayı dizisi
    // (en fazla NetScanner::kMaxPorts); verilmezse yaygın portlar
    // kullanılır. Süre alanları güvenlik için üst sınırlara kırpılır.
    const char *ssid = requestDoc["ssid"];
    if (ssid == nullptr) {
      return buildErrorResponse("missing ssid");
    }
    const char *password = requestDoc["password"] | "";

    uint16_t ports[NetScanner::kMaxPorts];
    uint8_t portCount = 0;
    JsonArray requestedPorts = requestDoc["ports"].as<JsonArray>();
    for (JsonVariant v : requestedPorts) {
      if (portCount >= NetScanner::kMaxPorts) {
        break;
      }
      ports[portCount++] = v.as<uint16_t>();
    }
    if (portCount == 0) {
      constexpr uint16_t kDefaultPorts[] = {21, 22, 23, 80, 443, 3389, 8080};
      portCount = sizeof(kDefaultPorts) / sizeof(kDefaultPorts[0]);
      memcpy(ports, kDefaultPorts, sizeof(kDefaultPorts));
    }

    uint32_t connectTimeoutMs = requestDoc["connect_timeout_ms"] | 10000;
    if (connectTimeoutMs > 20000) {
      connectTimeoutMs = 20000;
    }
    uint32_t portTimeoutMs = requestDoc["port_timeout_ms"] | 150;
    if (portTimeoutMs > 1000) {
      portTimeoutMs = 1000;
    }
    uint32_t maxTotalMs = requestDoc["timeout_ms"] | 30000;
    if (maxTotalMs > 60000) {
      maxTotalMs = 60000;
    }

    showOledStatus("Ağ Taraması", "Bağlanılıyor...");
    String data =
        netScanner.scan(String(ssid), String(password), ports, portCount, connectTimeoutMs, portTimeoutMs, maxTotalMs);
    if (data.length() == 0) {
      showOledStatus("Ağ Taraması", "Bağlantı hatası");
      return buildErrorResponse("could not connect to wifi network");
    }
    showOledStatus("Ağ Taraması", "Tamamlandı");
    return buildDataResponse(data);
  }

  if (strcmp(cmd, "oled_text") == 0) {
    const char *text = requestDoc["text"];
    if (text == nullptr) {
      return buildErrorResponse("missing text");
    }

    showPhoneMessage(String(text));
    return "{\"status\":\"ok\",\"data\":\"shown\"}";
  }

  if (strcmp(cmd, "subghz_replay") == 0) {
    const char *pulsesBase64 = requestDoc["pulses_b64"];
    if (pulsesBase64 == nullptr) {
      return buildErrorResponse("missing pulses_b64");
    }

    // Yakalandığı frekansta geri gönder; belirtilmezse varsayılana düşer.
    long frequencyHz = requestDoc["frequency_hz"] | 433920000L;
    subGhzManager.setFrequencyMhz(frequencyHz / 1000000.0f);
    subGhzManager.replaySignal(String(pulsesBase64));
    return "{\"status\":\"ok\",\"data\":\"replayed\"}";
  }

  return buildUnknownCommandResponse();
}
