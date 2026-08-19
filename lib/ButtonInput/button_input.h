#pragma once

#include <Arduino.h>
#include <functional>

// Cihazın 5 yönlü butonundan biri.
enum class ButtonId {
  kUp,
  kDown,
  kLeft,
  kRight,
  kConfirm,
};

using ButtonHandler = std::function<void(ButtonId)>;

// Pinout (INPUT_PULLUP; her buton basılınca ilgili pini GND'ye kısa
// devre yapar):
//   Yukarı  -> GPIO 32
//   Aşağı   -> GPIO 33
//   Sol     -> GPIO 14
//   Sağ     -> GPIO 13   (GPIO 12 kasıtlı olarak KULLANILMADI: ESP32'de
//                          flash voltaj seçimini belirleyen bir boot
//                          strapping pini; açılışta basılı tutulursa bazı
//                          kartlarda önyükleme sorunu yaratabilir.)
//   Onayla  -> GPIO 27
//
// Sol/Sağ, ileride Faz 6'daki IR modülüyle (GPIO 25/26) çakışmaması için
// bilinçli olarak 25/26 dışında seçildi.
//
// 5 butonu donanım kesmesi (interrupt) ile dinleyen, yazılımsal debounce
// uygulayan giriş katmanı. Kesmeler yalnızca bir "basıldı" bayrağı set
// eder; asıl debounce kontrolü ve handler çağrısı poll() içinde, loop()'tan
// çağrılarak yapılır. Böylece I2C/OLED gibi kesme bağlamında güvenli
// olmayan işler ana döngü bağlamında çalışır.
class ButtonInput {
 public:
  void begin(ButtonHandler handlerIn);

  // loop() içinden her turda çağrılmalı; kesme bayraklarını debounce'layıp
  // gerekiyorsa handler'ı tetikler.
  void poll();

 private:
  static constexpr uint8_t kButtonCount = 5;
  static constexpr uint32_t kDebounceMs = 200;

  struct ButtonPin {
    ButtonId id;
    uint8_t pin;
  };

  static const ButtonPin kPins[kButtonCount];

  ButtonHandler handler;
  uint32_t lastTriggerMs[kButtonCount] = {0, 0, 0, 0, 0};

  // Her pin için ayrı, statik ISR (attachInterrupt üye fonksiyon kabul
  // etmez); hangi pinin tetiklendiğini volatile bayrak dizisiyle
  // paylaşırlar.
  static volatile bool pressedFlags[kButtonCount];
  static void IRAM_ATTR isrUp();
  static void IRAM_ATTR isrDown();
  static void IRAM_ATTR isrLeft();
  static void IRAM_ATTR isrRight();
  static void IRAM_ATTR isrConfirm();
};
