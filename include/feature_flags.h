#pragma once

// Tam etkileşimli yerel kontrol (OLED + 5 buton menüsü, Faz 1) şu an takılı
// değil; butonlar ileri bir tarihe ertelendi. Butonlar hazır olup bu
// özelliği tekrar aktif etmek istediğinde bu değeri 1 yapıp yeniden derle —
// button_input/oled_display/device_menu kodlarının hiçbiri silinmedi,
// sadece bu bayrak 0 iken derlemeye dahil edilmiyorlar.
//
// 1 olduğunda ENABLE_OLED_STATUS'u geçersiz kılar (OLED'i DeviceMenu
// yönetir, aşağıdaki basit durum ekranı değil).
//
// BLE kontrolü (telefon üzerinden) bu bayraktan tamamen bağımsızdır,
// her zaman aktiftir.
#define ENABLE_LOCAL_CONTROLS 0

// OLED ekran takılı ama butonlar henüz yok — bu yüzden gezinilebilir bir
// menü yerine, yalnızca BLE bağlantı durumunu gösteren statik bir ekran
// aktif. ENABLE_LOCAL_CONTROLS 1 olduğunda bu bayrak yok sayılır.
#define ENABLE_OLED_STATUS 1
