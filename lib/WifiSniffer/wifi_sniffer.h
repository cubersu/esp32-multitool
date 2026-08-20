#pragma once

#include <Arduino.h>
#include <esp_wifi.h>

// ESP32'nin Wi-Fi radyosunu "promiscuous mode"a alıp havadaki ham 802.11
// çerçevelerini pasif olarak dinler; yalnızca EAPOL (WPA/WPA2 4 yönlü el
// sıkışma) çerçeveleriyle ilgilenir. Yakalanan çerçeveleri standart PCAP
// formatında, base64 kodlanmış olarak döner — Wireshark/aircrack-ng gibi
// araçlarla doğrudan açılabilir.
//
// SADECE KENDİ AĞINI TEST ETMEK İÇİN KULLAN. Başka birinin ağının
// trafiğini izinsiz dinlemek/yakalamak birçok ülkede (Türkiye dahil) suç
// teşkil edebilir.
//
// Bilinen basitleştirmeler (donanımla ilk testte gözden geçirilmeli):
// - Yalnızca standart (24 bayt) ve QoS (26 bayt) veri çerçeveleri
//   ayrıştırılır; WDS (4 adresli) çerçeveler desteklenmez (nadir görülür).
// - PCAP zaman damgaları gerçek saati değil, cihazın açılışından bu yana
//   geçen süreyi yansıtır (ESP32'de pilli bir RTC yok).
class WifiSniffer {
 public:
  void begin();

  // Belirtilen Wi-Fi kanalında (1-13) durationMs süresince ya da paket
  // arabelleği dolana kadar dinler. Yalnızca EAPOL çerçevelerini biriktirir.
  // Bloklayan bir çağrıdır. Sonucu PCAP formatında, base64 kodlanmış
  // olarak döner; hiçbir şey yakalanmazsa boş string döner.
  String captureHandshake(uint8_t channel, uint32_t durationMs);

  // Son captureHandshake() çağrısında yakalanan gerçek EAPOL paket sayısı.
  uint16_t lastPacketCount() const { return packetCount; }

 private:
  static constexpr uint16_t kMaxPackets = 20;
  static constexpr uint16_t kMaxPacketBytes = 260;

  struct CapturedPacket {
    uint16_t length;
    uint8_t data[kMaxPacketBytes];
  };

  static CapturedPacket packets[kMaxPackets];
  static volatile uint16_t packetCount;

  static void promiscuousCallback(void *buf, wifi_promiscuous_pkt_type_t type);
  static String buildPcap();
};
