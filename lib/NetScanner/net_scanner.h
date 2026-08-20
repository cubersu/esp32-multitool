#pragma once

#include <Arduino.h>

// Belirtilen Wi-Fi ağına katılıp (kendi ağın olmalı — bağlanabilmek için
// zaten şifresini bilmen gerekiyor) aynı /24 alt ağındaki canlı
// cihazları ve üzerlerinde açık olan (verilen listedeki) TCP portlarını
// bulan basit bir ağ keşif/port tarayıcı. nmap'in çok küçük ölçekli,
// tek bağlantılı (paralel olmayan) bir benzeri — ESP32'nin tek TCP soketi
// üzerinden sırayla bağlanmayı deneyerek çalışır, bu yüzden geniş alt
// ağlarda/çok port listesinde yavaş olabilir; bu yüzden hem taranan port
// sayısı hem de toplam süre sınırlıdır.
//
// SADECE KENDİ AĞINI TEST ETMEK İÇİN KULLAN.
class NetScanner {
 public:
  void begin();

  static constexpr uint8_t kMaxPorts = 8;
  static constexpr uint16_t kMaxHostsReported = 20;

  // Wi-Fi'ye connectTimeoutMs içinde bağlanamazsa boş string döner.
  // Bağlanabilirse, kendi /24 alt ağındaki (kendi IP'si hariç) ana
  // bilgisayarları dolaşıp her birinde ports[0..portCount) portlarını
  // portTimeoutMs zaman aşımıyla dener; maxTotalMs toplam tarama süresini
  // sınırlar (süre dolarsa o ana kadar bulunanlarla kısmi bir sonuç döner).
  // Tarama bitince Wi-Fi bağlantısı kapatılır (diğer komutlar kendi Wi-Fi
  // modunu bağımsız yönetiyor). Sonucu JSON string olarak döner:
  // {"type":"net_scan","local_ip":"...","hosts":[{"ip":"...","open_ports":[80,443]}]}
  String scan(const String &ssid, const String &password, const uint16_t *ports, uint8_t portCount,
              uint32_t connectTimeoutMs, uint32_t portTimeoutMs, uint32_t maxTotalMs);
};
