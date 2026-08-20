#pragma once

#include <Arduino.h>

// Belirtilen BSSID'den (kendi erişim noktan), belirtilen istemciye (veya
// tüm istemcilerine) ham 802.11 deauthentication çerçeveleri enjekte eder.
// Bu, kablosuz ağ güvenliği testinde (örn. yeniden bağlanma davranışı,
// WPA3 geçişi, IDS/IPS tepkisi testi) standart bir tekniktir.
//
// SADECE KENDİ AĞINI TEST ETMEK İÇİN KULLAN. Başkasının ağındaki
// istemcilerin bağlantısını izinsiz kesmek (deauth) birçok ülkede
// (Türkiye dahil) suç teşkil edebilir ve kablosuz hizmet kesintisine
// (DoS) yol açar.
//
// Kötüye kullanımı sınırlamak için tasarım kararları:
// - bssid HER ZAMAN çağıran tarafından (command_protocol.h) zorunlu
//   tutulur; "tara ve hepsine gönder" gibi otomatik/keşif tabanlı bir mod
//   kasıtlı olarak yok — her kullanım kendi BSSID'ini elle belirtmeli.
// - Tek çağrıda gönderilen çerçeve sayısı kMaxFrameCount ile sınırlıdır;
//   sürekli/sonsuz bir jammer modu yoktur, yalnızca kısa bir "burst".
class WifiDeauth {
 public:
  void begin();

  static constexpr uint16_t kMaxFrameCount = 40;

  // "AA:BB:CC:DD:EE:FF" biçimindeki bir string'i 6 baytlık ham MAC adresine
  // çevirir. Format geçersizse false döner.
  static bool parseMacAddress(const char *str, uint8_t out[6]);

  // clientMac tüm baytları 0xFF olan bir yayın adresiyse (FF:FF:FF:FF:FF:FF)
  // bssid'ye bağlı tüm istemciler hedeflenir — kendi AP'ni test etmek için
  // standart bir senaryo. frameCount, kMaxFrameCount'a kırpılır. Döndürülen
  // değer, radyoya gerçekten iletilen çerçeve sayısıdır.
  uint16_t sendDeauth(const uint8_t bssid[6], const uint8_t clientMac[6], uint8_t channel, uint16_t frameCount);
};
