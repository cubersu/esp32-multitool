#include <Arduino.h>

#include "ble_manager.h"
#include "command_protocol.h"
#include "feature_flags.h"
#include "subghz_manager.h"

#if ENABLE_LOCAL_CONTROLS
#include "button_input.h"
#include "device_menu.h"
#include "oled_display.h"
#endif

// BLE yöneticisi, servis/karakteristik kurulumunu ve callback'leri
// kendi içinde yönetir. OLED/buton donanımından bağımsız, her zaman aktif.
BleManager bleManager;

// CC1101 Sub-GHz modülü (Faz 4). command_protocol.h bu global'e "extern"
// ile erişir. Donanım henüz bağlı değil; SPI hattında karşılık veren bir
// çip yokken begin()'in/komutların davranışı doğrulanmadı, modül takılıp
// ilk test edildiğinde kontrol edilmeli.
SubGhzManager subGhzManager;

#if ENABLE_LOCAL_CONTROLS
// OLED + 5 butonla telefon olmadan yerel kontrol (Faz 1). Şu an
// feature_flags.h'daki ENABLE_LOCAL_CONTROLS ile devre dışı; donanıma
// dönüldüğünde o bayrağı 1 yapmak yeterli.
OledDisplay oledDisplay;
ButtonInput buttonInput;
DeviceMenu deviceMenu;
#endif

void setup() {
  // Sadece debug amaçlı; donanım bağlıyken seri monitörden izlenebilir.
  Serial.begin(115200);

  // BLE'yi başlat ve gelen komutları command_protocol'daki
  // processCommand fonksiyonuna yönlendir.
  bleManager.begin(processCommand);

  // CC1101'i 433.92MHz + ASK/OOK moduna ayarla (Faz 4).
  subGhzManager.begin();

#if ENABLE_LOCAL_CONTROLS
  // Yerel OLED + buton menüsünü başlat. deviceMenu, ping/wifi_scan/
  // ble_scan komutlarını BLE ile aynı processCommand() fonksiyonu
  // üzerinden çalıştırır.
  oledDisplay.begin();
  deviceMenu.begin(&oledDisplay);
  buttonInput.begin([](ButtonId button) { deviceMenu.onButton(button); });
#endif
}

void loop() {
  // BLE iletişimi tamamen callback tabanlı, burada yapılacak bir şey yok.
#if ENABLE_LOCAL_CONTROLS
  // Buton kesmeleri de callback tabanlı tetiklenir; poll() yalnızca
  // kesmenin bıraktığı bayrağı debounce'layıp menüye iletir (I2C/OLED
  // çizimi kesme bağlamında güvenli olmadığı için buraya ertelenir) —
  // bu, BLE komutlarını beklemek için "polling" yapmakla karıştırılmamalı.
  buttonInput.poll();
#endif
}
