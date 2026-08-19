#include "oled_display.h"

namespace {
constexpr int kVisibleRows = 4;
constexpr int kRowHeight = 12;
constexpr int kFirstRowY = 24;
}  // namespace

void OledDisplay::begin() {
  u8g2.begin();
  u8g2.setFont(u8g2_font_6x10_tf);
}

void OledDisplay::showMenu(const String &title, const std::vector<String> &items, int selectedIndex) {
  u8g2.clearBuffer();
  u8g2.drawStr(0, 10, title.c_str());
  u8g2.drawHLine(0, 12, 128);

  int itemCount = static_cast<int>(items.size());

  int windowStart = 0;
  if (itemCount > kVisibleRows) {
    windowStart = selectedIndex - kVisibleRows / 2;
    if (windowStart < 0) {
      windowStart = 0;
    }
    if (windowStart > itemCount - kVisibleRows) {
      windowStart = itemCount - kVisibleRows;
    }
  }

  int windowEnd = windowStart + kVisibleRows;
  if (windowEnd > itemCount) {
    windowEnd = itemCount;
  }

  int y = kFirstRowY;
  for (int i = windowStart; i < windowEnd; i++) {
    String line = (i == selectedIndex ? "> " : "  ") + items[i];
    u8g2.drawStr(0, y, line.c_str());
    y += kRowHeight;
  }

  u8g2.sendBuffer();
}

void OledDisplay::showText(const String &title, const String &body) {
  u8g2.clearBuffer();
  u8g2.drawStr(0, 10, title.c_str());
  u8g2.drawHLine(0, 12, 128);
  u8g2.drawStr(0, 24, body.c_str());
  u8g2.sendBuffer();
}
