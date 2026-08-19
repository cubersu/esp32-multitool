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
