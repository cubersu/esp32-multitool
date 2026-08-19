#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>

#include "ble_scanner.h"
#include "wifi_scanner.h"

// Bilinmeyen/geçersiz komutlar için ortak hata yanıtı.
inline String buildUnknownCommandResponse() {
  return "{\"status\":\"error\",\"msg\":\"unknown command\"}";
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
    return "{\"status\":\"ok\",\"data\":\"pong\"}";
  }

  if (strcmp(cmd, "wifi_scan") == 0) {
    return buildDataResponse(scanWifiNetworks());
  }

  if (strcmp(cmd, "ble_scan") == 0) {
    return buildDataResponse(scanBleDevices());
  }

  return buildUnknownCommandResponse();
}
