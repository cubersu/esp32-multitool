#include <Arduino.h>

#include "ble_manager.h"
#include "button_input.h"
#include "command_protocol.h"
#include "device_menu.h"
#include "oled_display.h"

// BLE yöneticisi, servis/karakteristik kurulumunu ve callback'leri
// kendi içinde yönetir.
BleManager bleManager;

// OLED + 5 butonla telefon olmadan yerel kontrol (Faz 1). Buton girişleri
// (INPUT_PULLUP) donanım takılı olmasa da güvenle başlatılabilir. OLED
// fiziksel olarak takılı değilken begin()'in davranışı doğrulanmadı;
// donanım gelince ilk açılışta kontrol edilmeli.
OledDisplay oledDisplay;
ButtonInput buttonInput;
DeviceMenu deviceMenu;

void setup() {
  // Sadece debug amaçlı; donanım bağlıyken seri monitörden izlenebilir.
  Serial.begin(115200);

  // BLE'yi başlat ve gelen komutları command_protocol'daki
  // processCommand fonksiyonuna yönlendir.
  bleManager.begin(processCommand);

  // Yerel OLED + buton menüsünü başlat. deviceMenu, ping/wifi_scan/
  // ble_scan komutlarını BLE ile aynı processCommand() fonksiyonu
  // üzerinden çalıştırır.
  oledDisplay.begin();
  deviceMenu.begin(&oledDisplay);
  buttonInput.begin([](ButtonId button) { deviceMenu.onButton(button); });
}

void loop() {
  // BLE iletişimi tamamen callback tabanlı, burada yapılacak bir şey yok.
  // Buton kesmeleri de callback tabanlı tetiklenir; poll() yalnızca
  // kesmenin bıraktığı bayrağı debounce'layıp menüye iletir (I2C/OLED
  // çizimi kesme bağlamında güvenli olmadığı için buraya ertelenir) —
  // bu, BLE komutlarını beklemek için "polling" yapmakla karıştırılmamalı.
  buttonInput.poll();
}
