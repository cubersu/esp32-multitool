#include "ble_manager.h"

#include <BLE2902.h>

#include "ble_uuids.h"

void BleManager::begin(CommandHandler handler) {
  commandHandler = handler;

  BLEDevice::init(BLE_DEVICE_NAME);
  // Varsayılan BLE paket boyutu (23 byte) büyük yanıtlar (örn. Sub-GHz ham
  // sinyal verisi) için yetersiz; daha büyük bir MTU talep ediyoruz.
  // Telefon bunu kabul etmeyebilir, o zaman GATT varsayılana düşer —
  // AndroidBleManager tarafında da eşleşen bir requestMtu() çağrısı var.
  BLEDevice::setMTU(517);

  server = BLEDevice::createServer();
  serverCallbacks = new ServerCallbacks();
  server->setCallbacks(serverCallbacks);

  BLEService *service = server->createService(BLE_SERVICE_UUID);

  // Telefonun komut yazacağı karakteristik (WRITE)
  commandCharacteristic = service->createCharacteristic(
      BLE_COMMAND_CHAR_UUID, BLECharacteristic::PROPERTY_WRITE);
  commandCallbacks = new CommandCharCallbacks(this);
  commandCharacteristic->setCallbacks(commandCallbacks);

  // Cihazın yanıt göndereceği karakteristik (NOTIFY)
  responseCharacteristic = service->createCharacteristic(
      BLE_RESPONSE_CHAR_UUID, BLECharacteristic::PROPERTY_NOTIFY);
  responseCharacteristic->addDescriptor(new BLE2902());

  service->start();

  // Advertising'i başlat, böylece telefon cihazı tarayabilir
  BLEAdvertising *advertising = BLEDevice::getAdvertising();
  advertising->addServiceUUID(BLE_SERVICE_UUID);
  advertising->setScanResponse(true);
  BLEDevice::startAdvertising();
}

void BleManager::notifyResponse(const String &payload) {
  if (responseCharacteristic == nullptr) {
    return;
  }
  responseCharacteristic->setValue(payload.c_str());
  responseCharacteristic->notify();
}

void BleManager::CommandCharCallbacks::onWrite(
    BLECharacteristic *characteristic) {
  // getValue() std::string döner; String(const char*) kurucusu ile
  // güvenli şekilde Arduino String'e çeviriyoruz.
  String receivedValue = String(characteristic->getValue().c_str());
  if (receivedValue.length() == 0 || owner->commandHandler == nullptr) {
    return;
  }

  String response = owner->commandHandler(receivedValue);
  owner->notifyResponse(response);
}

void BleManager::ServerCallbacks::onConnect(BLEServer *server) {
  // Şu an için ek bir işlem gerekmiyor, ileride bağlantı durumu takibi
  // eklenebilir.
}

void BleManager::ServerCallbacks::onDisconnect(BLEServer *server) {
  // Bağlantı koptuğunda advertising durur; yeniden keşfedilebilir olmak
  // için advertising'i tekrar başlatıyoruz.
  BLEDevice::startAdvertising();
}
