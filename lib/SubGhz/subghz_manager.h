#pragma once

#include <Arduino.h>

// CC1101 433MHz modülüyle ham (raw) RF sinyali yakalama/tekrar oynatma
// (replay). Yalnızca ASK/OOK modülasyonlu, SABİT KODLU (fixed code)
// kumandalar için çalışır — rolling-code'lu sistemler (araç anahtarı
// gibi) bu yöntemle klonlanamaz.
//
// Bağlantı (SPI):
//   CC1101 SCK  -> GPIO 18
//   CC1101 MISO -> GPIO 19
//   CC1101 MOSI -> GPIO 23
//   CC1101 CSN  -> GPIO 5
//   CC1101 GDO0 -> GPIO 4
//   CC1101 VCC  -> 3.3V (5V DEĞİL — modül 5V'a dayanmaz)
//   CC1101 GND  -> GND
//
// Yakalama, CC1101'i "ham/async seri" moduna alıp GDO0 pininden gelen
// demodüle edilmiş dijital sinyalin (HIGH/LOW) süre geçişlerini donanım
// kesmesiyle (interrupt) zamanlayarak yapılır — Flipper Zero'nun sabit
// kodlu kumandaları yakalama yöntemiyle aynı temel prensip.
//
// Yakalanan/gönderilecek sinyal, BLE bildiriminin sınırlı paket boyutuna
// (bkz. ble_manager.cpp'deki MTU ayarı) sığması için ham mikrosaniye
// darbe sürelerinin (her biri uint16_t, little-endian) base64 ile
// kodlanmış hâli olarak taşınır.
class SubGhzManager {
 public:
  // CC1101'i başlatır, 433.92MHz'e ve ASK/OOK + async seri (raw) moduna
  // ayarlar. setup() içinde bir kez çağrılmalı.
  void begin();

  // GDO0 pinini dinleyip bir sinyal yakalanana ya da timeoutMs süresi
  // dolana kadar bekler. İlk kenardan sonra ~5ms sessizlik olursa (burst
  // bitti kabul edilir) erken döner. Yakalanan darbeleri base64 string
  // olarak döner; hiçbir şey yakalanmazsa boş string döner. Bloklayan
  // bir çağrıdır.
  String captureSignal(uint32_t timeoutMs);

  // Daha önce captureSignal()'in döndürdüğü formattaki base64 darbe
  // dizisini CC1101 üzerinden tekrar gönderir (replay). Format hatalı ya
  // da kapasiteden (kMaxPulses) büyükse sessizce hiçbir şey yapmaz.
  void replaySignal(const String &pulsesBase64);

 private:
  static constexpr uint8_t kGdo0Pin = 4;
  // Tek bir "burst" (bir kumanda tuşuna basışın tek bir tekrarı) tipik
  // olarak 24-40 bit ~ 50-80 kenar içerir; 150, makul bir tekrar payı da
  // bırakan güvenli bir üst sınır. Bu sınır aynı zamanda base64 sonrası
  // JSON yanıtının tek bir BLE bildirimine sığmasını garanti etmek için
  // kasıtlı olarak küçük tutuldu (bkz. sınıf üstü yorum).
  static constexpr uint16_t kMaxPulses = 150;
  static constexpr uint32_t kEndOfBurstGapUs = 5000;

  static volatile uint16_t pulseDurations[kMaxPulses];
  static volatile uint16_t pulseCount;
  static volatile uint32_t lastEdgeMicros;

  static void IRAM_ATTR onEdge();
};
