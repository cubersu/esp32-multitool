#pragma once

#include <Arduino.h>
#include <esp_wifi.h>

// Belirtilen BSSID'nin belirli bir kanaldaki beacon çerçevesini pasif
// olarak dinleyip WPS (Wi-Fi Protected Setup) bilgi elemanının (vendor
// specific IE, OUI 00:50:F2, tip 4) yayınlanıp yayınlanmadığını kontrol
// eder. Hiçbir şey yaymaz, yalnızca dinler.
//
// ÖNEMLİ KAPSAM SINIRI: Bu SADECE "WPS açık mı?" sorusuna cevap veren pasif
// bir keşif aracıdır — gerçek bir WPS PIN kaba kuvvet (brute force) aracı
// (Reaver/Pixie Dust benzeri) DEĞİLDİR. WPS'in 8 mesajlık M1-M8 registrar/
// enrollee el sıkışmasını ve PIN deneme mantığını uygulamak çok daha büyük
// bir iş; bu modül yalnızca "AP WPS'i beacon'ında ilan ediyor mu" bilgisini
// verir — bu genelde bir pentest'in ilk (keşif) adımıdır: WPS açıksa,
// bilinen masaüstü araçlarıyla (örn. Reaver) devam edilir.
//
// SADECE KENDİ AĞINI TEST ETMEK İÇİN KULLAN.
class WpsCheck {
 public:
  void begin();

  // Belirtilen kanalda BSSID'den bir beacon çerçevesi yakalanana kadar (ya
  // da timeoutMs geçene kadar) bekler. Bir beacon yakalanırsa true döner ve
  // *wpsEnabled'ı WPS IE'sinin bulunup bulunmadığına göre ayarlar. Zaman
  // aşımında (o kanalda/BSSID'de hiç beacon görülmediyse) false döner.
  bool checkWps(const uint8_t bssid[6], uint8_t channel, uint32_t timeoutMs, bool *wpsEnabled);

 private:
  static volatile bool beaconSeen;
  static volatile bool wpsIeFound;
  static uint8_t targetBssid[6];

  static void promiscuousCallback(void *buf, wifi_promiscuous_pkt_type_t type);
};
