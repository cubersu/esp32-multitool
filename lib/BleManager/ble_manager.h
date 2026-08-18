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

 private:
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

  // Bağlantı koptuğunda advertising'i yeniden başlatmak için kullanılan
  // callback sınıfı (aksi halde cihaz tekrar keşfedilebilir olmuyor).
  class ServerCallbacks : public BLEServerCallbacks {
   public:
    void onConnect(BLEServer *server) override;
    void onDisconnect(BLEServer *server) override;
  };

  CommandCharCallbacks *commandCallbacks = nullptr;
  ServerCallbacks *serverCallbacks = nullptr;
};
