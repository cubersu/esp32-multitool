#include "subghz_manager.h"

#include <ELECHOUSE_CC1101_SRC_DRV.h>
#include <base64.h>

volatile uint16_t SubGhzManager::pulseDurations[SubGhzManager::kMaxPulses];
volatile uint16_t SubGhzManager::pulseCount = 0;
volatile uint32_t SubGhzManager::lastEdgeMicros = 0;

void SubGhzManager::begin() {
  ELECHOUSE_cc1101.setGDO0(kGdo0Pin);
  ELECHOUSE_cc1101.Init();
  setFrequencyMhz(kDefaultFrequencyMhz);

  // Ham/raw mod: CC1101'in kendi paket yönetimini kapat (setCCMode(0)),
  // ASK/OOK demodülasyonu (setModulation(2)) ve asenkron seri çıkış
  // (setPktFormat(3)) seç. Bu kombinasyonla GDO0 pini, RX'te alınan RF
  // sinyalinin demodüle edilmiş halini doğrudan dijital (HIGH/LOW) olarak
  // verir; TX'te ise ESP32'nin bu pine yazdığı HIGH/LOW'u RF'e modüle
  // eder. Donanım gelmeden doğrulanamadı, ilk denemede kontrol edilmeli.
  ELECHOUSE_cc1101.setCCMode(0);
  ELECHOUSE_cc1101.setModulation(2);
  ELECHOUSE_cc1101.setPktFormat(3);
}

void SubGhzManager::setFrequencyMhz(float mhz) {
  ELECHOUSE_cc1101.setMHZ(mhz);
}

String SubGhzManager::captureSignal(uint32_t timeoutMs, CaptureProgressCallback onProgress) {
  pulseCount = 0;
  lastEdgeMicros = micros();

  pinMode(kGdo0Pin, INPUT);
  ELECHOUSE_cc1101.SetRx();
  attachInterrupt(digitalPinToInterrupt(kGdo0Pin), onEdge, CHANGE);

  uint32_t startMs = millis();
  uint32_t lastProgressMs = startMs;
  if (onProgress) {
    onProgress(0, 0);
  }

  while (millis() - startMs < timeoutMs) {
    uint32_t nowMs = millis();
    if (onProgress && (nowMs - lastProgressMs) >= 1000) {
      lastProgressMs = nowMs;
      onProgress(pulseCount, nowMs - startMs);
    }

    // İlk kenardan sonra yeterince sessizlik geçtiyse (burst bitti kabul
    // edilir) taramayı erken sonlandır.
    if (pulseCount > 0 && (micros() - lastEdgeMicros) > kEndOfBurstGapUs) {
      break;
    }
    if (pulseCount >= kMaxPulses) {
      break;
    }
    delay(1);
  }

  detachInterrupt(digitalPinToInterrupt(kGdo0Pin));
  ELECHOUSE_cc1101.setSidle();

  uint16_t count = pulseCount;
  if (count == 0) {
    return "";
  }

  // volatile diziyi, base64::encode'un beklediği düz uint8_t buffer'a
  // kopyala (little-endian uint16 çiftleri).
  uint8_t rawBytes[kMaxPulses * 2];
  for (uint16_t i = 0; i < count; i++) {
    rawBytes[i * 2] = pulseDurations[i] & 0xFF;
    rawBytes[i * 2 + 1] = (pulseDurations[i] >> 8) & 0xFF;
  }

  return base64::encode(rawBytes, count * 2);
}

void SubGhzManager::replaySignal(const String &pulsesBase64) {
  int estimatedLength = base64::decodedLength(pulsesBase64.c_str(), pulsesBase64.length());
  if (estimatedLength <= 0 || estimatedLength > static_cast<int>(kMaxPulses * 2)) {
    // Boş ya da beklenenden büyük veri; sabit boyutlu buffer'ı taşırmamak
    // için reddet.
    return;
  }

  uint8_t rawBytes[kMaxPulses * 2];
  int actualLength =
      base64::decode(reinterpret_cast<char *>(rawBytes), pulsesBase64.c_str(), pulsesBase64.length());
  if (actualLength <= 0) {
    return;
  }

  uint16_t count = actualLength / 2;

  pinMode(kGdo0Pin, OUTPUT);
  ELECHOUSE_cc1101.SetTx();

  for (uint16_t i = 0; i < count; i++) {
    uint16_t durationUs = rawBytes[i * 2] | (rawBytes[i * 2 + 1] << 8);
    // Çift indeksler HIGH, tek indeksler LOW segment kabul edilir
    // (captureSignal ile simetrik; ilk segment her zaman HIGH).
    digitalWrite(kGdo0Pin, (i % 2 == 0) ? HIGH : LOW);
    delayMicroseconds(durationUs);
  }
  digitalWrite(kGdo0Pin, LOW);

  ELECHOUSE_cc1101.setSidle();
  pinMode(kGdo0Pin, INPUT);
}

void IRAM_ATTR SubGhzManager::onEdge() {
  uint32_t now = micros();
  uint32_t duration = now - lastEdgeMicros;
  lastEdgeMicros = now;

  if (pulseCount < kMaxPulses) {
    pulseDurations[pulseCount] = (duration > 0xFFFF) ? 0xFFFF : static_cast<uint16_t>(duration);
    pulseCount++;
  }
}
