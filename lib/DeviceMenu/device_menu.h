#pragma once

#include <Arduino.h>
#include <vector>

#include "button_input.h"
#include "oled_display.h"

// Telefon olmadan cihazı OLED + 5 yön tuşuyla yerel olarak kontrol eden
// basit bir menü durum makinesi. Ana menüdeki komutlar, command_protocol.h
// içindeki processCommand()'ı çağırarak çalışır; böylece BLE üzerinden
// telefonla ve yerel butonlarla tetiklenen komutlar aynı mantığı paylaşır,
// iki ayrı implementasyon olmaz.
class DeviceMenu {
 public:
  void begin(OledDisplay *displayIn);

  // ButtonInput'un handler'ı olarak kaydedilir.
  void onButton(ButtonId button);

 private:
  enum class Screen {
    kMainMenu,
    // Tek satırlık düz sonuç (örn. ping). Herhangi bir tuş ana menüye
    // döner.
    kTextResult,
    // wifi_scan/ble_scan gibi kaydırılabilir liste sonucu. Yukarı/Aşağı
    // kaydırır, diğer tuşlar ana menüye döner.
    kListResult,
  };

  static const std::vector<String> kMainMenuItems;

  OledDisplay *display = nullptr;
  Screen screen = Screen::kMainMenu;
  int selectedIndex = 0;

  std::vector<String> resultLines;
  int resultSelectedIndex = 0;
  String resultTitle;

  void renderMainMenu();
  void handleMainMenuButton(ButtonId button);
  void handleListResultButton(ButtonId button);
  void runSelectedCommand();
};
