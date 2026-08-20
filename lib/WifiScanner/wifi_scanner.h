#pragma once

#include <Arduino.h>

// Yakındaki Wi-Fi ağlarını tarar ve sonucu, command_protocol'un yanıt
// "data" alanına doğrudan gömebileceği bir JSON nesnesi (string) olarak
// döner: {"type":"wifi_scan","networks":[{"ssid":..,"rssi":..,"secure":..}]}
//
// Bloklayan bir çağrıdır (tipik olarak birkaç saniye sürer); bu süre
// boyunca yeni BLE komutları işlenemez. Tarama kullanıcı tarafından nadiren
// tetiklendiği için bu kabul edilebilir bir tradeoff.
String scanWifiNetworks();

// Belirtilen SSID'yi yayınlayan TÜM erişim noktalarını (her BSSID ayrı bir
// giriş) listeler. Aynı SSID'yi birden fazla BSSID yayınlıyorsa (veya
// knownBssid verilip eşleşmeyen bir BSSID bulunursa) bu "sahte AP / evil
// twin" şüphesi için bir işarettir — hiçbir şey yaymaz, tamamen pasif
// tarama. knownBssid boş string ("") verilirse sadece bulunanları listeler,
// hiçbirini "beklenen" olarak işaretlemez.
//
// Sonucu, "data" alanına doğrudan gömülebilecek bir JSON nesnesi (string)
// olarak döner:
// {"type":"rogue_ap_scan","ssid":"...","access_points":[
//   {"bssid":"AA:BB:...","rssi":-45,"secure":true,"is_known":true}
// ],"suspicious":true}
// "suspicious", ya birden fazla BSSID bulunduğunda ya da knownBssid verilip
// onunla eşleşmeyen en az bir BSSID bulunduğunda true olur.
String scanForRogueAp(const String &ssid, const String &knownBssid);
