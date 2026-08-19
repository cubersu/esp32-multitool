#include "device_menu.h"

#include <ArduinoJson.h>

#include "command_protocol.h"

const std::vector<String> DeviceMenu::kMainMenuItems = {"Ping Test", "WiFi Tara", "BLE Tara"};

void DeviceMenu::begin(OledDisplay *displayIn) {
  display = displayIn;
  renderMainMenu();
}

void DeviceMenu::onButton(ButtonId button) {
  switch (screen) {
    case Screen::kMainMenu:
      handleMainMenuButton(button);
      break;
    case Screen::kTextResult:
      // Sonuç ekranındayken herhangi bir tuş ana menüye döner.
      screen = Screen::kMainMenu;
      renderMainMenu();
      break;
    case Screen::kListResult:
      handleListResultButton(button);
      break;
  }
}

void DeviceMenu::renderMainMenu() {
  if (display == nullptr) {
    return;
  }
  display->showMenu("ESP32-MultiTool", kMainMenuItems, selectedIndex);
}

void DeviceMenu::handleMainMenuButton(ButtonId button) {
  switch (button) {
    case ButtonId::kUp:
      selectedIndex =
          (selectedIndex - 1 + static_cast<int>(kMainMenuItems.size())) % kMainMenuItems.size();
      renderMainMenu();
      break;
    case ButtonId::kDown:
      selectedIndex = (selectedIndex + 1) % kMainMenuItems.size();
      renderMainMenu();
      break;
    case ButtonId::kConfirm:
      runSelectedCommand();
      break;
    case ButtonId::kLeft:
    case ButtonId::kRight:
      // Ana menüde şu an kullanılmıyor; ileride alt menüler için ayrılmış.
      break;
  }
}

void DeviceMenu::handleListResultButton(ButtonId button) {
  if (resultLines.empty()) {
    screen = Screen::kMainMenu;
    renderMainMenu();
    return;
  }

  switch (button) {
    case ButtonId::kUp:
      resultSelectedIndex =
          (resultSelectedIndex - 1 + static_cast<int>(resultLines.size())) % resultLines.size();
      display->showMenu(resultTitle, resultLines, resultSelectedIndex);
      break;
    case ButtonId::kDown:
      resultSelectedIndex = (resultSelectedIndex + 1) % resultLines.size();
      display->showMenu(resultTitle, resultLines, resultSelectedIndex);
      break;
    default:
      screen = Screen::kMainMenu;
      renderMainMenu();
      break;
  }
}

void DeviceMenu::runSelectedCommand() {
  if (display == nullptr) {
    return;
  }

  const String &label = kMainMenuItems[selectedIndex];
  display->showText(label, "Calisiyor...");

  if (selectedIndex == 0) {
    // Ping: düz metin sonucu.
    String result = processCommand("{\"cmd\":\"ping\"}");
    display->showText(label, result);
    screen = Screen::kTextResult;
    return;
  }

  // WiFi/BLE tarama: sonucu, kaydırılabilir bir listeye dönüştür.
  const char *commandJson = (selectedIndex == 1) ? "{\"cmd\":\"wifi_scan\"}" : "{\"cmd\":\"ble_scan\"}";
  String result = processCommand(commandJson);

  JsonDocument responseDoc;
  DeserializationError parseError = deserializeJson(responseDoc, result);
  if (parseError || responseDoc["status"] != "ok") {
    display->showText(label, "Hata olustu");
    screen = Screen::kTextResult;
    return;
  }

  resultLines.clear();
  JsonObject data = responseDoc["data"];

  if (selectedIndex == 1) {
    resultTitle = "WiFi Aglari";
    for (JsonObject network : data["networks"].as<JsonArray>()) {
      String ssid = network["ssid"].as<String>();
      int rssi = network["rssi"].as<int>();
      resultLines.push_back(ssid + " (" + String(rssi) + ")");
    }
  } else {
    resultTitle = "BLE Cihazlari";
    for (JsonObject device : data["devices"].as<JsonArray>()) {
      String name = device["name"].as<String>();
      if (name.length() == 0) {
        name = "(isimsiz)";
      }
      int rssi = device["rssi"].as<int>();
      resultLines.push_back(name + " (" + String(rssi) + ")");
    }
  }

  if (resultLines.empty()) {
    resultLines.push_back("(sonuc yok)");
  }

  resultSelectedIndex = 0;
  screen = Screen::kListResult;
  display->showMenu(resultTitle, resultLines, resultSelectedIndex);
}
