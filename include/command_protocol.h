#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>

#include "ble_scanner.h"
#include "buzzer.h"
#include "subghz_manager.h"
#include "wifi_scanner.h"

// SubGhzManager ve Buzzer, setup() içinde bir kez begin() çağrılan durumlu
// (stateful) modüller oldukları için main.cpp'de tanımlanır; burada
// yalnızca referans ediliyor.
extern SubGhzManager subGhzManager;
extern Buzzer buzzer;

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
