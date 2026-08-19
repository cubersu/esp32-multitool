#include "oled_display.h"

namespace {
constexpr int kVisibleRows = 4;
constexpr int kRowHeight = 12;
constexpr int kFirstRowY = 24;
}  // namespace

void OledDisplay::begin() {
  u8g2.begin();
  // u8g2_font_6x10_tf yalnızca ASCII kapsıyordu; Türkçe karakterler (ç, ğ,
  // ı, İ, ö, ş, ü) için Latin Extended-A bloğunu da içeren unifont_t_latin
  // kullanılıyor. Daha büyük bir bitmap font olduğu için flash kullanımı
  // artar ama ESP32'nin flash'ı (tipik 4MB) buna fazlasıyla yeter.
  u8g2.setFont(u8g2_font_unifont_t_latin);
}

void OledDisplay::showMenu(const String &title, const std::vector<String> &items, int selectedIndex) {
  u8g2.clearBuffer();
  // drawStr() UTF-8 çok baytlı dizileri çözmez (her baytı ayrı bir glif
  // sanır); drawUTF8() Türkçe karakterlerin doğru gösterilmesi için şart.
  u8g2.drawUTF8(0, 10, title.c_str());
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
    u8g2.drawUTF8(0, y, line.c_str());
    y += kRowHeight;
  }

  u8g2.sendBuffer();
}

void OledDisplay::showText(const String &title, const String &body) {
  u8g2.clearBuffer();
  u8g2.drawUTF8(0, 10, title.c_str());
  u8g2.drawHLine(0, 12, 128);

  // unifont_t_latin ~6px/karakter genişliğinde; 128px genişlikte küçük bir
  // kenar payıyla satır başına ~20 (tek baytlı ASCII) karakter sığar.
  // NOT: wrapText() bayt sayıyor, Unicode karakter değil — Türkçe harfler
  // UTF-8'de 2 bayt tuttuğu için boşluksuz uzun bir kelime tam bu sınıra
  // denk gelirse, teoride çok baytlı bir karakterin ortasından bölünebilir.
  // Kelime araları (boşluk) olan normal mesajlarda bu risk oluşmaz.
  constexpr int kMaxCharsPerLine = 20;
  std::vector<String> lines = wrapText(body, kMaxCharsPerLine);

  int y = kFirstRowY;
  int rowsDrawn = 0;
  for (const String &line : lines) {
    if (rowsDrawn >= kVisibleRows) {
      break;
    }
    u8g2.drawUTF8(0, y, line.c_str());
    y += kRowHeight;
    rowsDrawn++;
  }

  u8g2.sendBuffer();
}

std::vector<String> OledDisplay::wrapText(const String &text, int maxCharsPerLine) {
  std::vector<String> lines;
  int start = 0;
  int length = static_cast<int>(text.length());

  while (start < length) {
    int remaining = length - start;
    int take = (remaining < maxCharsPerLine) ? remaining : maxCharsPerLine;

    // Kelime ortasından bölmemek için, sığdığı yerden geriye doğru son
    // boşluğu ara; bulursa oradan böl.
    if (take < remaining) {
      int lastSpace = text.lastIndexOf(' ', start + take);
      if (lastSpace > start) {
        take = lastSpace - start;
      }
    }

    String line = text.substring(start, start + take);
    line.trim();
    lines.push_back(line);

    start += take;
    while (start < length && text[start] == ' ') {
      start++;
    }
  }

  return lines;
}
