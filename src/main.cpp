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

// command_protocol.h'daki komut işleyicileri OLED'e bir şey göstermek
// istediğinde bu fonksiyonları çağırır. OledDisplay bayrağa göre var
// olabildiği/olmayabildiği için (bkz. yukarısı) command_protocol.h'ın
// doğrudan "oledDisplay" global'ine erişmesi yerine bu tek noktadan geçen
// sarmalayıcılar kullanılıyor; böylece hangi mod aktifse (ya da hiçbiri
// değilse) davranış burada tek yerde yönetiliyor.
void showOledStatus(const String &title, const String &body) {
#if ENABLE_LOCAL_CONTROLS
  // Tam menü modunda OLED'i DeviceMenu yönetiyor; komut sonucu ekranını
  // menünün üzerine yazmak kafa karıştırıcı olur, bu yüzden bu modda
  // otomatik durum ekranları şimdilik yok sayılıyor.
#elif ENABLE_OLED_STATUS
  oledDisplay.showText(title, body);
#endif
}

// "oled_text" komutuyla telefondan gönderilen serbest metni gösterir.
void showPhoneMessage(const String &text) {
  showOledStatus("Telefondan", text);
}

// SubGhzManager::captureSignal()'e verilen ilerleme callback'i. Yakalama
// başında (elapsedMs == 0) kayan çubuk grafiği sıfırlar; sonraki her
// çağrıda son bir saniyede gelen yeni darbe sayısını ("aktivite seviyesi")
// grafiğe ekler — bir sinyal yakalandığında bu değer yükselir ("peak").
void showSubGhzProgress(uint16_t pulseCount, uint32_t elapsedMs) {
#if ENABLE_LOCAL_CONTROLS
  // Tam menü modunda OLED'i DeviceMenu yönetiyor; canlı grafik burada
  // gösterilmiyor.
#elif ENABLE_OLED_STATUS
  static uint16_t lastPulseCount = 0;
  if (elapsedMs == 0) {
    lastPulseCount = 0;
    oledDisplay.beginSignalGraph("Sub-GHz Dinleniyor");
    return;
  }
  uint16_t delta = (pulseCount >= lastPulseCount) ? (pulseCount - lastPulseCount) : 0;
  lastPulseCount = pulseCount;
  oledDisplay.pushSignalGraphSample(static_cast<uint8_t>(delta > 255 ? 255 : delta));
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
  showOledStatus("ESP32-MultiTool", "Bağlantı bekleniyor...");
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
      showOledStatus("ESP32-MultiTool", connected ? "Bağlı" : "Bağlantı bekleniyor...");
    }
  }
#endif
}
