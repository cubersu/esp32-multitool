#pragma once

// Yerel kontrol donanımı (OLED + 5 buton, Faz 1) şu an takılı değil;
// "buton işi" ileri bir tarihe ertelendi. Donanım hazır olup bu özelliği
// tekrar aktif etmek istediğinde bu değeri 1 yapıp yeniden derle —
// button_input/oled_display/device_menu kodlarının hiçbiri silinmedi,
// sadece bu bayrak 0 iken derlemeye dahil edilmiyorlar.
//
// BLE kontrolü (telefon üzerinden) bu bayraktan tamamen bağımsızdır,
// her zaman aktiftir.
#define ENABLE_LOCAL_CONTROLS 0
