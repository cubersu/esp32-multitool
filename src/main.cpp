#include <Arduino.h>

#include "ble_manager.h"
#include "command_protocol.h"

// BLE yöneticisi, servis/karakteristik kurulumunu ve callback'leri
// kendi içinde yönetir.
BleManager bleManager;

void setup() {
  // Sadece debug amaçlı; donanım bağlıyken seri monitörden izlenebilir.
  Serial.begin(115200);

  // BLE'yi başlat ve gelen komutları command_protocol'daki
  // processCommand fonksiyonuna yönlendir.
  bleManager.begin(processCommand);
}

void loop() {
  // Tüm iletişim BLE callback'leri üzerinden yürüdüğü için burada
  // yapılacak bir şey yok, polling kullanılmıyor.
}
