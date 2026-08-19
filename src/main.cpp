#include <Arduino.h>

#include "ble_manager.h"
#include "buzzer.h"
#include "command_protocol.h"
#include "feature_flags.h"
#include "subghz_manager.h"

#if ENABLE_LOCAL_CONTROLS
#include "button_input.h"
#include "device_menu.h"
#include "oled_display.h"
#elif ENABLE_OLED_STATUS
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

// Piezo buzzer ("RX2" pini / GPIO16). command_protocol.h ping komutunda
// bunu çağırır.
Buzzer buzzer;

#if ENABLE_LOCAL_CONTROLS
// OLED + 5 butonla telefon olmadan yerel kontrol (Faz 1). Şu an
// feature_flags.h'daki ENABLE_LOCAL_CONTROLS ile devre dışı; donanıma
// dönüldüğünde o bayrağı 1 yapmak yeterli.
OledDisplay oledDisplay;
ButtonInput buttonInput;
DeviceMenu deviceMenu;
#elif ENABLE_OLED_STATUS
// Butonlar henüz yok; OLED yalnızca BLE bağlantı durumunu gösteren statik
// bir ekran olarak kullanılıyor (gezinilebilir menü yok).
OledDisplay oledDisplay;
#endif

// command_protocol.h, "oled_text" komutunu işlerken bu fonksiyonu çağırır.
// OledDisplay bayrağa göre var olabildiği/olmayabildiği için (bkz. yukarısı)
// command_protocol.h'ın doğrudan "oledDisplay" global'ine erişmesi yerine bu
// tek noktadan geçen sarmalayıcı kullanılıyor; böylece hangi mod aktifse
// (ya da hiçbiri değilse) davranış burada tek yerde yönetiliyor.
void showPhoneMessage(const String &text) {
#if ENABLE_LOCAL_CONTROLS
  // Tam menü modunda OLED'i DeviceMenu yönetiyor; telefon mesajını
  // menünün üzerine yazmak kafa karıştırıcı olur, bu yüzden bu modda
  // "oled_text" komutu şimdilik yok sayılıyor.
#elif ENABLE_OLED_STATUS
  oledDisplay.showText("Telefondan", text);
#endif
}

void setup() {
  // Sadece debug amaçlı; donanım bağlıyken seri monitörden izlenebilir.
  Serial.begin(115200);

  // BLE'yi başlat ve gelen komutları command_protocol'daki
  // processCommand fonksiyonuna yönlendir.
  bleManager.begin(processCommand);

  // CC1101'i 433.92MHz + ASK/OOK moduna ayarla (Faz 4).
  subGhzManager.begin();

  buzzer.begin();

#if ENABLE_LOCAL_CONTROLS
  // Yerel OLED + buton menüsünü başlat. deviceMenu, ping/wifi_scan/
  // ble_scan komutlarını BLE ile aynı processCommand() fonksiyonu
  // üzerinden çalıştırır.
  oledDisplay.begin();
  deviceMenu.begin(&oledDisplay);
  buttonInput.begin([](ButtonId button) { deviceMenu.onButton(button); });
#elif ENABLE_OLED_STATUS
  oledDisplay.begin();
  oledDisplay.showText("ESP32-MultiTool", "Baglanti bekleniyor...");
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
#elif ENABLE_OLED_STATUS
  // Buton olmadığı için tetiklenecek bir kesme yok; bağlantı durumu
  // değiştiğinde ekranı güncellemek için ~500ms'de bir hafif bir kontrol
  // yapıyoruz. Bu, BLE komutlarını beklemek için "polling" yapmakla
  // karıştırılmamalı — yalnızca ekranı tazelemek için, BLE komut işleme
  // hâlâ tamamen callback tabanlı.
  static bool lastConnected = false;
  static uint32_t lastCheckMs = 0;
  uint32_t now = millis();
  if (now - lastCheckMs >= 500) {
    lastCheckMs = now;
    bool connected = bleManager.isConnected();
    if (connected != lastConnected) {
      lastConnected = connected;
      oledDisplay.showText("ESP32-MultiTool", connected ? "Bagli" : "Baglanti bekleniyor...");
    }
  }
#endif
}
