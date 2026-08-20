#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>

#include "ble_manager.h"
#include "ble_scanner.h"
#include "buzzer.h"
#include "subghz_manager.h"
#include "wifi_scanner.h"
#include "wifi_sniffer.h"

// BleManager, SubGhzManager, Buzzer ve WifiSniffer, setup() içinde bir kez
// begin() çağrılan durumlu (stateful) modüller oldukları için main.cpp'de
// tanımlanır; burada yalnızca referans ediliyor. bleManager'a doğrudan
// erişim yalnızca wifi_capture'ın parçalı (chunked) bildirim göndermesi
// için gerekiyor — normal komutlar hâlâ tek bir dönüş değeriyle cevap
// veriyor, BleManager::CommandCharCallbacks bunu otomatik notify ediyor.
extern BleManager bleManager;
extern SubGhzManager subGhzManager;
extern Buzzer buzzer;
extern WifiSniffer wifiSniffer;

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
