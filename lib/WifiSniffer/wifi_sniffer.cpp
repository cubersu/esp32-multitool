#include "wifi_sniffer.h"

#include <WiFi.h>
#include <base64.h>
#include <cstring>

WifiSniffer::CapturedPacket WifiSniffer::packets[WifiSniffer::kMaxPackets];
volatile uint16_t WifiSniffer::packetCount = 0;

namespace {
// EAPOL (802.1X) EtherType; WPA/WPA2 4 yönlü el sıkışma çerçeveleri LLC/SNAP
// başlığından sonra bu EtherType ile taşınır.
constexpr uint16_t kEapolEtherType = 0x888E;
constexpr uint16_t kDot11HeaderLen = 24;
constexpr uint16_t kQosControlLen = 2;
constexpr uint16_t kLlcHeaderLen = 8;
}  // namespace

void WifiSniffer::begin() {
  // Promiscuous mode her yakalamada captureHandshake() içinde ayrı
  // başlatılıp durduruluyor; burada ek bir başlatma gerekmiyor.
}

String WifiSniffer::captureHandshake(uint8_t channel, uint32_t durationMs) {
  packetCount = 0;

  WiFi.mode(WIFI_STA);
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);

  wifi_promiscuous_filter_t filter = {};
  filter.filter_mask = WIFI_PROMIS_FILTER_MASK_DATA;
  esp_wifi_set_promiscuous_filter(&filter);
  esp_wifi_set_promiscuous_rx_cb(&WifiSniffer::promiscuousCallback);

  uint32_t startMs = millis();
  while (millis() - startMs < durationMs && packetCount < kMaxPackets) {
    delay(10);
  }

  esp_wifi_set_promiscuous(false);

  if (packetCount == 0) {
    return "";
  }

  return buildPcap();
}

void WifiSniffer::promiscuousCallback(void *buf, wifi_promiscuous_pkt_type_t type) {
  if (packetCount >= kMaxPackets) {
    return;
  }

  auto *pkt = static_cast<wifi_promiscuous_pkt_t *>(buf);
  uint16_t len = pkt->rx_ctrl.sig_len;
  const uint8_t *payload = pkt->payload;

  if (len < kDot11HeaderLen) {
    return;
  }

  // Frame Control baytının Type (bit 2-3) ve Subtype (bit 4-7) alanları.
  // Yalnızca Data çerçeveleriyle (Type == 2) ilgileniyoruz; QoS alt tipinde
  // (Subtype'ın 0x08 biti set) standart 24 baytlık başlıktan sonra 2 baytlık
  // ek bir QoS Control alanı gelir — bunu atlamazsak EtherType'ı yanlış
  // konumdan okuruz.
  uint8_t frameType = (payload[0] >> 2) & 0x3;
  uint8_t frameSubtype = (payload[0] >> 4) & 0xF;
  if (frameType != 2) {
    return;
  }
  bool isQos = (frameSubtype & 0x08) != 0;

  uint16_t llcOffset = kDot11HeaderLen + (isQos ? kQosControlLen : 0);
  if (len < static_cast<uint16_t>(llcOffset + kLlcHeaderLen)) {
    return;
  }

  uint16_t etherType = (payload[llcOffset + 6] << 8) | payload[llcOffset + 7];
  if (etherType != kEapolEtherType) {
    return;
  }

  uint16_t captureLen = (len > kMaxPacketBytes) ? kMaxPacketBytes : len;
  packets[packetCount].length = captureLen;
  memcpy(packets[packetCount].data, payload, captureLen);
  packetCount++;
}

String WifiSniffer::buildPcap() {
  // PCAP global başlığı (24 bayt) + her paket için 16 baytlık kayıt
  // başlığı + ham çerçeve verisi. Standart bir formattır; Wireshark/
  // aircrack-ng gibi araçlarla doğrudan açılabilir.
  size_t totalSize = 24;
  for (uint16_t i = 0; i < packetCount; i++) {
    totalSize += 16 + packets[i].length;
  }

  uint8_t *buffer = new uint8_t[totalSize];
  size_t offset = 0;

  auto writeU32 = [&](uint32_t value) {
    buffer[offset++] = value & 0xFF;
    buffer[offset++] = (value >> 8) & 0xFF;
    buffer[offset++] = (value >> 16) & 0xFF;
    buffer[offset++] = (value >> 24) & 0xFF;
  };
  auto writeU16 = [&](uint16_t value) {
    buffer[offset++] = value & 0xFF;
    buffer[offset++] = (value >> 8) & 0xFF;
  };

  writeU32(0xa1b2c3d4);       // magic number
  writeU16(2);                // version major
  writeU16(4);                // version minor
  writeU32(0);                // thiszone
  writeU32(0);                // sigfigs
  writeU32(kMaxPacketBytes);  // snaplen
  writeU32(105);              // network = LINKTYPE_IEEE802_11

  // Gerçek saat yok; cihazın açılışından bu yana geçen süreyi yazıyoruz.
  uint32_t nowSec = millis() / 1000;
  for (uint16_t i = 0; i < packetCount; i++) {
    writeU32(nowSec);
    writeU32(0);
    writeU32(packets[i].length);
    writeU32(packets[i].length);
    memcpy(buffer + offset, packets[i].data, packets[i].length);
    offset += packets[i].length;
  }

  String encoded = base64::encode(buffer, totalSize);
  delete[] buffer;
  return encoded;
}
