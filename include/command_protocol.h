#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>

// Gelen komutları JSON olarak işleyip yine JSON formatında yanıt üreten
// header-only yardımcı fonksiyon. BLE katmanından bağımsızdır; sadece
// string alır, string döner. Böylece BLE ve komut mantığı birbirinden
// ayrı test edilebilir/değiştirilebilir.
inline String processCommand(const String &input) {
  JsonDocument requestDoc;

  // Gelen veri geçerli bir JSON değilse bilinmeyen komut gibi davran
  DeserializationError parseError = deserializeJson(requestDoc, input);
  if (parseError) {
    return "{\"status\":\"error\",\"msg\":\"unknown command\"}";
  }

  const char *cmd = requestDoc["cmd"];
  if (cmd == nullptr) {
    return "{\"status\":\"error\",\"msg\":\"unknown command\"}";
  }

  JsonDocument responseDoc;

  if (strcmp(cmd, "ping") == 0) {
    responseDoc["status"] = "ok";
    responseDoc["data"] = "pong";
  } else {
    responseDoc["status"] = "error";
    responseDoc["msg"] = "unknown command";
  }

  String response;
  serializeJson(responseDoc, response);
  return response;
}
