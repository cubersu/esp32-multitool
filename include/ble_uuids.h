#pragma once

// BLE servis ve karakteristik UUID'leri.
// Bu UUID'ler rastgele üretilmiştir ancak sabittir; Android tarafındaki
// uygulama da BLE bağlantısını kurabilmek için aynı UUID'leri kullanmalıdır.

// Ana GATT servisi
#define BLE_SERVICE_UUID "1eac5c68-6cfd-46ca-b9dc-0d8dd2ade33f"

// Telefonun komut yazdığı karakteristik (WRITE)
#define BLE_COMMAND_CHAR_UUID "b10a8537-f7ed-46d1-801c-735e3c74ea5e"

// Cihazın yanıt gönderdiği karakteristik (NOTIFY)
#define BLE_RESPONSE_CHAR_UUID "f24e04f4-aec8-420c-9162-98efd1cd5fbf"

// Advertising sırasında görünecek cihaz adı
#define BLE_DEVICE_NAME "ESP32-MultiTool"
