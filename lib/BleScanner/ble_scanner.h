#pragma once

#include <Arduino.h>

// Yakındaki BLE cihazlarını tarar ve sonucu, command_protocol'un yanıt
// "data" alanına doğrudan gömebileceği bir JSON nesnesi (string) olarak
// döner: {"type":"ble_scan","devices":[{"name":..,"address":..,"rssi":..}]}
//
// Bloklayan bir çağrıdır (varsayılan 5 sn sürer). ESP32 aynı anda hem
// telefona bağlı bir GATT sunucusu (peripheral) hem de tarayıcı (central)
// rolünde davranır; bu, ESP32 BLE yığınının desteklediği bir senaryodur
// ama fiziksel donanımla doğrulanana kadar bağlantı kararlılığı garanti
// edilemez.
String scanBleDevices();
