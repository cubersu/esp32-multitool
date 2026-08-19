#pragma once

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <functional>

// Gelen komut string'ini işleyip yanıt string'i üreten fonksiyon tipi.
// BLE katmanı komutun içeriğini bilmez; sadece bu callback'i çağırır.
using CommandHandler = std::function<String(const String &)>;

// BLE GATT sunucusunu (servis + WRITE/NOTIFY karakteristikleri) kurup
// yöneten sınıf. Tüm iletişim callback tabanlıdır, polling içermez.
class BleManager {
 public:
  // BLE'yi başlatır, servisi/karakteristikleri oluşturur ve advertising'i
  // başlatır. handler, komut karakteristiğine yazılan her veri için
  // çağrılır ve dönen yanıt otomatik olarak NOTIFY karakteristiğinden
  // gönderilir.
  void begin(CommandHandler handler);

  // Yanıt karakteristiğinin değerini günceller ve bağlı istemciye
  // notify gönderir.
  void notifyResponse(const String &payload);

  // Şu an bir telefonun bağlı olup olmadığını döner. BLE komut işleme bu
  // değere bağımlı değildir; yerel OLED durum ekranı gibi tüketiciler için
  // eklendi (bkz. main.cpp / feature_flags.h ENABLE_OLED_STATUS).
  bool isConnected() const { return connected; }

 private:
  bool connected = false;
  BLEServer *server = nullptr;
  BLECharacteristic *commandCharacteristic = nullptr;
  BLECharacteristic *responseCharacteristic = nullptr;
  CommandHandler commandHandler;

  // Komut karakteristiğine yazma olduğunda tetiklenen callback sınıfı.
  class CommandCharCallbacks : public BLECharacteristicCallbacks {
   public:
    explicit CommandCharCallbacks(BleManager *owner) : owner(owner) {}
    void onWrite(BLECharacteristic *characteristic) override;

   private:
    BleManager *owner;
  };

  // Bağlantı koptuğunda advertising'i yeniden başlatmak, bağlıyken de
  // owner->connected'ı güncel tutmak için kullanılan callback sınıfı.
  class ServerCallbacks : public BLEServerCallbacks {
   public:
    explicit ServerCallbacks(BleManager *owner) : owner(owner) {}
    void onConnect(BLEServer *server) override;
    void onDisconnect(BLEServer *server) override;

   private:
    BleManager *owner;
  };

  CommandCharCallbacks *commandCallbacks = nullptr;
  ServerCallbacks *serverCallbacks = nullptr;
};
