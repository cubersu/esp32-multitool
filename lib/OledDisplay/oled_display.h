#pragma once

#include <Arduino.h>
#include <U8g2lib.h>

#include <vector>

// 0.96" I2C SSD1306 OLED (128x64) için ince bir sarmalayıcı.
// Bağlantı: SDA -> GPIO 21, SCL -> GPIO 22 (ESP32'nin varsayılan donanım
// I2C pinleri; U8g2'nin HW_I2C constructor'ı bunları otomatik kullanır,
// ekstra Wire.begin() çağrısına gerek yoktur), VCC -> 3.3V, GND -> GND.
//
// Ekranın kendisi yalnızca "şu anki içeriği çiz" bilir; hangi metnin/
// menünün gösterileceğine DeviceMenu karar verir.
class OledDisplay {
 public:
  void begin();

  // Bir başlık ve seçilebilir/kaydırılabilir satırlardan oluşan bir menü
  // çizer; selectedIndex'teki satır ">" ile vurgulanır. Ekrana sığandan
  // fazla satır varsa görünüm otomatik olarak seçili satırı ortalayacak
  // şekilde kaydırılır.
  void showMenu(const String &title, const std::vector<String> &items, int selectedIndex);

  // Düz bir başlık + gövde metni çizer (örn. ping sonucu, hata mesajı).
  void showText(const String &title, const String &body);

 private:
  U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2{U8G2_R0};
};
