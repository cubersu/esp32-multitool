#include "wps_check.h"

#include <WiFi.h>
#include <cstring>

namespace {
constexpr uint16_t kDot11HeaderLen = 24;
constexpr uint16_t kBeaconFixedFieldsLen = 12;  // Timestamp(8) + Interval(2) + Capability(2)
constexpr uint8_t kWpsOui[3] = {0x00, 0x50, 0xF2};
constexpr uint8_t kWpsOuiType = 0x04;
constexpr uint8_t kVendorSpecificTag = 221;
}  // namespace

volatile bool WpsCheck::beaconSeen = false;
volatile bool WpsCheck::wpsIeFound = false;
uint8_t WpsCheck::targetBssid[6] = {0};

void WpsCheck::begin() {
  // Her checkWps() çağrısı promiscuous mode'u kendi başlatıp durduruyor;
  // burada ek bir başlatma gerekmiyor.
}

bool WpsCheck::checkWps(const uint8_t bssid[6], uint8_t channel, uint32_t timeoutMs, bool *wpsEnabled) {
  beaconSeen = false;
  wpsIeFound = false;
  memcpy(targetBssid, bssid, 6);

  WiFi.mode(WIFI_STA);
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);

  wifi_promiscuous_filter_t filter = {};
  filter.filter_mask = WIFI_PROMIS_FILTER_MASK_MGMT;
  esp_wifi_set_promiscuous_filter(&filter);
  esp_wifi_set_promiscuous_rx_cb(&WpsCheck::promiscuousCallback);

  uint32_t startMs = millis();
  while (millis() - startMs < timeoutMs && !beaconSeen) {
    delay(10);
  }

  esp_wifi_set_promiscuous(false);

  if (!beaconSeen) {
    return false;
  }
  *wpsEnabled = wpsIeFound;
  return true;
}

void WpsCheck::promiscuousCallback(void *buf, wifi_promiscuous_pkt_type_t type) {
  if (beaconSeen) {
    return;
  }

  auto *pkt = static_cast<wifi_promiscuous_pkt_t *>(buf);
  uint16_t len = pkt->rx_ctrl.sig_len;
  const uint8_t *payload = pkt->payload;

  if (len < kDot11HeaderLen + kBeaconFixedFieldsLen) {
    return;
  }

  // Frame Control baytı: Type (bit 2-3) == Management(0), Subtype (bit 4-7) == Beacon(8).
  uint8_t frameType = (payload[0] >> 2) & 0x3;
  uint8_t frameSubtype = (payload[0] >> 4) & 0xF;
  if (frameType != 0 || frameSubtype != 8) {
    return;
  }

  // Addr2 (verici/BSSID), 802.11 başlığında bayt 10-15.
  if (memcmp(payload + 10, targetBssid, 6) != 0) {
    return;
  }

  beaconSeen = true;

  // Etiketli parametreler (tagged parameters), sabit beacon alanlarından
  // sonra başlar. Her etiket: [tag id (1)] [uzunluk (1)] [değer (uzunluk)].
  uint16_t offset = kDot11HeaderLen + kBeaconFixedFieldsLen;
  while (offset + 2 <= len) {
    uint8_t tagId = payload[offset];
    uint8_t tagLen = payload[offset + 1];
    uint16_t valueOffset = offset + 2;
    if (valueOffset + tagLen > len) {
      break;
    }

    if (tagId == kVendorSpecificTag && tagLen >= 4 && memcmp(payload + valueOffset, kWpsOui, 3) == 0 &&
        payload[valueOffset + 3] == kWpsOuiType) {
      wpsIeFound = true;
      break;
    }

    offset = valueOffset + tagLen;
  }
}
