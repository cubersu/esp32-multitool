#pragma once

#include <Arduino.h>

// Basit, pasif piezo buzzer sürücüsü. Kartın üzerinde "RX2" yazan pine
// bağlı (= GPIO16); bu projede UART2 kullanılmadığı için o pin sıradan
// bir dijital çıkış olarak serbestçe kullanılabilir.
//
// tone(), ESP32 Arduino core'da donanımsal PWM (LEDC) üzerinden çalışır ve
// non-blocking'tir: beep() çağrısı hemen döner, ton süresi dolunca arka
// planda kendiliğinden kesilir — BLE komut işleme akışını bloklamaz.
// (Not: tone()/noTone() nispeten yeni bir arduino-esp32 core özelliği;
// derlemede bulunamazsa LEDC API'sine (ledcAttach/ledcWriteTone) elle
// geçilmesi gerekebilir, ilk derlemede kontrol edilmeli.)
class Buzzer {
 public:
  void begin();

  // Kısa bir bip sesi çalar. Varsayılan 2kHz/120ms, çoğu piezo buzzer için
  // net duyulabilir bir "beep" verir.
  void beep(uint16_t frequencyHz = 2000, uint16_t durationMs = 120);

 private:
  static constexpr uint8_t kPin = 16;  // Kartın üzerinde "RX2" yazan pin.
};
