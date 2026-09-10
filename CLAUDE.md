# Android Auto Wireless — ESP32-P4 Project

## Цель

Реализовать беспроводной Android Auto head unit на базе ESP32-P4 с дисплеем 800×480.

Проект поддерживает **два режима подключения** — выбирается через `#define CONNECTION_MODE` в `main/config.h`:

---

## Режимы подключения

### MODE A — С ESP32-WROOM (рекомендуется)
**Работает как настоящая магнитола. Без доп. приложений. Без настроек на телефоне.**

Телефон видит head unit по Classic Bluetooth → автоматически запускает AA Wireless → переходит на WiFi.
Внешний ESP32-WROOM-32 (~$3) подключается по UART и занимается только BT handshake.

```c
#define CONNECTION_MODE MODE_BT_CLASSIC
```

### MODE B — Без ESP32-WROOM (только C6 на плате)
**Требует Wireless Helper APK на телефоне (одноразовая установка).**

Телефон находит head unit по mDNS через Wireless Helper → запускает AA Wireless.
Никакого дополнительного железа. Wireless Helper — open source, бесплатный:
https://github.com/andreknieriem/wireless-helper

```c
#define CONNECTION_MODE MODE_WIRELESS_HELPER
```

| | Mode A (BT Classic) | Mode B (Wireless Helper) |
|---|---|---|
| Доп. железо | ESP32-WROOM ~$3 | Нет |
| Доп. приложение | Нет | Wireless Helper APK |
| Опыт пользователя | Как настоящая магнитола | Поставить APK один раз |
| Сложность реализации | Выше | Ниже |

---

## Железо

Целевая плата: **Waveshare ESP32-P4-WIFI6-Touch-LCD-4.3**.

- Магазин: https://www.waveshare.com/esp32-p4-wifi6-touch-lcd-4.3.htm
- Wiki + примеры: https://github.com/waveshareteam/ESP32-P4-WIFI6-Touch-LCD-4.3
- Локальная копия примеров (для оффлайн-сравнения): `research/_sources/waveshare_p4_4_3/`
  - BSP, который мы взяли в `components/esp32_p4_wifi6_touch_lcd_4_3/`,
    скопирован из `examples/esp-idf/07_Displaycolorbar/components/`
  - Полезные демо: `08_lvgl_demo_v9` (LVGL UI), `09_video_lcd_display`,
    `10_mp4_player`, `11_esp_brookesia_phone`

| Компонент | Чип | Роль | Mode A | Mode B |
|---|---|---|---|---|
| Основная плата | ESP32-P4 | Главный процессор | ✅ | ✅ |
| На плате | ESP32-C6 | WiFi транспорт | ✅ | ✅ |
| Дисплей | 800×480 ST7701 MIPI-DSI + GT911 touch | Вывод видео | ✅ | ✅ |
| Внешний модуль | ESP32-WROOM-32 | Classic BT handshake | ✅ | ❌ |

### Подключение ESP32-WROOM (только Mode A)

```
ESP32-WROOM TX  →  ESP32-P4 GPIO (UART RX)
ESP32-WROOM RX  →  ESP32-P4 GPIO (UART TX)
ESP32-WROOM GND →  GND
ESP32-WROOM 3V3 →  3V3
```

---

## Архитектура

### Mode A — Classic BT

```
[Android телефон]
      |
      | 1. Classic BT (HFP handshake + WiFi credentials)
      |
[ESP32-WROOM] ──UART──> [ESP32-P4]
                              |
                    2. [ESP32-C6] (WiFi 6)
                              |
                    3. TCP + TLS (AA Wireless protocol)
                              |
                    4. H.264 decode → дисплей 800×480
```

### Mode B — Wireless Helper

```
[Android телефон]
      |
      | 1. Wireless Helper APK находит head unit по mDNS
      |
      | 2. TCP + TLS (AA Wireless protocol)
      |
                    [ESP32-C6] (WiFi 6)
                              |
                    [ESP32-P4]
                              |
                    H.264 decode → дисплей 800×480
```

---

## Стек технологий

- **Фреймворк:** ESP-IDF (последняя стабильная)
- **H.264 декодер:** компонент `esp_h264` от Espressif
- **TLS:** mbedTLS (встроен в ESP-IDF)
- **Protobuf:** nanopb (встроен в ESP-IDF)
- **mDNS:** компонент `mdns` (встроен в ESP-IDF)
- **BT стек:** Classic BT + HFP на ESP32-WROOM через esp-idf bluetooth classic
- **UI:** LVGL поверх декодированных фреймов

---

## Компоненты для реализации

### config.h — выбор режима

```c
// main/config.h
#define MODE_BT_CLASSIC       1
#define MODE_WIRELESS_HELPER  2

// Выбрать один:
#define CONNECTION_MODE MODE_BT_CLASSIC
// #define CONNECTION_MODE MODE_WIRELESS_HELPER
```

---

### 1. Прошивка ESP32-WROOM — BT агент (только Mode A)

Задача: принять BT соединение от телефона, выполнить AA handshake, передать WiFi параметры по UART на P4.

```
Файлы:
  bt_agent/
    main/
      bt_hfp.c       — HFP профиль, обнаружение телефона
      wifi_bridge.c  — передача WiFi credentials по UART
      main.c
    CMakeLists.txt
    sdkconfig.defaults
```

Используемые API ESP-IDF:
- `esp_bt_controller_init()`
- `esp_bluedroid_init()`
- `esp_hf_client_register_callback()`
- UART: `uart_driver_install()`, `uart_write_bytes()`

---

### 2. Прошивка ESP32-P4 — главный процессор

#### 2a. WiFi менеджер (через ESP32-C6, оба режима)

```c
// ESP-Hosted SDIO транспорт к C6
// Mode A: получить WiFi credentials от WROOM по UART, подключиться к сети телефона
// Mode B: подключиться к известной WiFi сети (или поднять AP)
// Поднять TCP сервер на порту 5277 (стандарт AA Wireless)
```

#### 2b. mDNS анонс (оба режима)

```c
mdns_init();
mdns_hostname_set("android-auto");
mdns_service_add(NULL, "_androidauto", "_tcp", 5277, NULL, 0);
// Mode A: телефон находит сам после BT handshake
// Mode B: Wireless Helper находит по этому анонсу
```

#### 2c. Инициализация подключения (зависит от режима)

```c
#if CONNECTION_MODE == MODE_BT_CLASSIC
    // Ждать UART сообщение от ESP32-WROOM с WiFi credentials
    // Подключиться к сети телефона
    // Запустить mDNS + TCP сервер
#elif CONNECTION_MODE == MODE_WIRELESS_HELPER
    // Подключиться к WiFi (сохранённые credentials или WiFiManager)
    // Запустить mDNS + TCP сервер
    // Wireless Helper APK на телефоне найдёт по mDNS и запустит AA
#endif
```

#### 2d. AA Protocol stack (порт aasdk на ESP-IDF, оба режима)

Заменить Linux-зависимости:

| Оригинал (aasdk) | ESP-IDF замена |
|---|---|
| Boost.Asio | FreeRTOS tasks + lwIP sockets |
| Boost.Log | ESP_LOG |
| libusb | не нужен (WiFi) |
| OpenSSL | mbedTLS |
| protobuf | nanopb |

Структура протокола:
```
aa_protocol/
  transport/    — TCP соединение, frame parsing
  ssl/          — TLS handshake через mbedTLS
  channel/      — Video, Audio, Input, Sensor каналы
  messenger/    — protobuf encode/decode через nanopb
```

#### 2d. H.264 декодер

```c
// Использовать esp_h264 компонент
// Входной буфер: H.264 NAL units из AA Video канала
// Выходной буфер: RGB565 фреймы для дисплея
// Целевое разрешение: 640×480 @ 31fps (задокументировано Espressif)
// Апскейл до 800×480 через Pixel Processing Accelerator ESP32-P4
```

#### 2e. Вывод на дисплей

```c
// RGB/SPI интерфейс к дисплею 800×480
// Double buffering через DMA
// LVGL как UI слой поверх AA видео
// Touch события → AA InputChannel → телефон
```

---

## Порядок реализации

### Mode A (с ESP32-WROOM)

```
Этап 1  ESP32-WROOM BT агент
        └── HFP подключение к телефону
        └── Передача WiFi credentials по UART
        └── Тест: телефон видит head unit по BT

Этап 2  ESP32-P4 WiFi + TCP сервер
        └── Получение credentials от WROOM по UART
        └── Подключение к сети телефона
        └── TCP сервер на порту 5277
        └── mDNS анонс

Этап 3–7  (общие с Mode B, см. ниже)
```

### Mode B (Wireless Helper, без доп. железа)

```
Этап 1  ESP32-P4 WiFi + TCP сервер
        └── Подключение к WiFi (сохранённые credentials)
        └── TCP сервер на порту 5277
        └── mDNS анонс _androidauto._tcp
        └── Тест: Wireless Helper на телефоне находит устройство

Этап 2–6  (общие, см. ниже)
```

### Общие этапы (оба режима)

```
Этап TLS  TLS handshake
          └── mbedTLS сервер
          └── AA SSL сертификаты (самоподписанные)
          └── Тест: телефон завершает TLS

Этап AA   AA протокол — базовые каналы
          └── nanopb .proto файлы из aasdk
          └── Version negotiation
          └── Service Discovery
          └── Тест: AA запускается на телефоне

Этап VID  Video канал
          └── H.264 через esp_h264
          └── Вывод на дисплей
          └── Тест: видео идёт на экран

Этап IN   Input канал
          └── Touch → AA TouchEvent protobuf
          └── Тест: тач работает в AA интерфейсе

Этап AUD  Audio канал
          └── AAC декодинг
          └── I2S вывод на динамик
```

---

## Ключевые репозитории для изучения

- https://github.com/f1xpl/aasdk — оригинальный AA протокол стек (Linux)
- https://github.com/f1xpl/openauto — AA head unit на RPi (Linux)
- https://github.com/andreknieriem/headunit-revived — WiFi/wireless режимы
- https://github.com/espressif/esp-idf/tree/master/examples/bluetooth/classic_bt — BT Classic примеры
- https://github.com/espressif/esp-h264 — H.264 декодер для P4

---

## Важные нюансы

1. **BT Classic только на ESP32-WROOM** — ESP32-C6 и ESP32-P4 Classic BT не имеют
2. **H.264 декодинг SW** — esp_h264 работает программно на P4, аппаратного декодера нет, но 640×480@31fps достаточно для AA
3. **AA Wireless версия 16.4+** — есть баги с некоторыми режимами подключения, тестировать на актуальном телефоне
4. **nanopb .proto файлы** — брать из aasdk репозитория, регенерировать под nanopb
5. **ESP-Hosted** — ESP32-P4 общается с ESP32-C6 по SDIO, не по UART

---

## Команды для старта

```bash
# Установить ESP-IDF
git clone --recursive https://github.com/espressif/esp-idf.git
cd esp-idf && ./install.sh esp32,esp32p4
. ./export.sh

# Сборка и прошивка
idf.py build
idf.py -p <PORT> flash monitor
```

## Поддержка нескольких девайсов (мульти-борд)

Прошивка собирается под несколько плат, и **не все на одном чипе**: две
головные ESP32-P4 (с Android Auto) и одна ESP32-S3 (только VESC-дашборд).
Обычный `idf.py build` собирает под **Waveshare 4.3"** (дефолт, обратная
совместимость). Для выбора платы есть `scripts/build_board.sh <board>
<аргументы idf.py>`:

```bash
scripts/build_board.sh                                   # собрать ВСЕ платы (или `all`)
scripts/build_board.sh waveshare flash monitor
scripts/build_board.sh jc4880 -p <PORT> flash monitor   # Guition JC4880P443C, 16 МБ флеш
scripts/build_board.sh s3touch4 -p <PORT> flash monitor # Waveshare ESP32-S3-Touch-LCD-4, 480x480
```

Механизм:
- **Kconfig `choice BOARD_MODEL`** (`main/Kconfig.projbuild`):
  `CONFIG_BOARD_WAVESHARE_43` (дефолт) / `CONFIG_BOARD_JC4880P443C` /
  `CONFIG_BOARD_S3_TOUCH_LCD_4`. Каждый вариант привязан к своему чипу
  (`depends on IDF_TARGET_*`). Глобален → читается в BSP, `main/bt_link.h`,
  `main/touch_input.c`, `main/display_init.c`.
- **`build_board.sh`** собирает в отдельную `build_<board>/` и накладывает
  оверлей: `SDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.defaults.<board>"`
  (поздний файл переопределяет ранний). Базовый `sdkconfig.defaults` = Waveshare.
- **Пины/тайминги, не выражаемые через готовый Kconfig** (подсветка/reset LCD,
  I2S DSIN/PA, DPI-тайминги панели, vendor-init ST7701, пины BT-агента) — через
  `#if CONFIG_BOARD_JC4880P443C` в BSP (`components/esp32_p4_wifi6_touch_lcd_4_3/`)
  и `main/bt_link.h`.
- **sdkconfig-оверлеи** задают флеш, имя партишен-файла, CAN-пины, выбор борда:
  `sdkconfig.defaults.waveshare` (32 МБ, `partitions.csv`, CAN 48/47) и
  `sdkconfig.defaults.jc4880` (16 МБ, `partitions_16mb.csv`, CAN 51/52) и
  `sdkconfig.defaults.s3touch4` (`CONFIG_IDF_TARGET="esp32s3"`, 16 МБ,
  `partitions_s3_16mb.csv`, CAN 6/0, PSRAM octal + XIP, консоль USB-JTAG, -O2).
- **JC = 16 МБ** → отдельная `partitions_16mb.csv`: OTA 5 МБ ×2 + storage 1 МБ +
  triplog ~4.9 МБ. Образ ~3.8 МБ → запас в слоте ~1.2 МБ (24%), следить за ростом.
- **JC пины** (свободный хедер): BT-агент `TX=33 RX=31 RST=30 IO0=29`,
  CAN `RX=52 TX=51`, подсветка LCD `23`, reset LCD `5`. Дисплей ST7701S,
  DPI 34 МГц, vendor-init по умолчанию драйвера; WiFi (SDIO→C6), I2C тача,
  SD — совпадают с Waveshare.
- **Идентификатор модели** `BOARD_MODEL_ID` (`main/board.h`, `"waveshare"`/
  `"jc4880"`/`"s3touch4"`) прошивка сообщает приложению (BLE OTA-info `…0006`
  6-м полем + `GET /info`), чтобы APK выбрал правильный из вшитых бинарей.
- **Релиз** (`scripts/release.sh`) собирает все борды, кладёт per-device бинари
  `release/esp32p4_android_auto-<board>-<ver>.bin` и один APK с обеими прошивками.
  Блобы C6 / BT-агента общие для всех плат.

### ESP32-S3-Touch-LCD-4 (`s3touch4`) — дашборд без Android Auto

Waveshare ESP32-S3-Touch-LCD-4: ESP32-S3-WROOM-1-N16R8 (16 МБ флеш, 8 МБ octal
PSRAM), экран **480×480** ST7701 по RGB565 (16 бит параллельно), тач GT911.
Wi-Fi и BLE — на самом чипе, без ESP32-C6. Плата собирается из этого же
дерева и получает всё, кроме Android Auto: дашборд, CAN/VESC, BLE
(NUS/OTA/файлы/уведомления), PAS, статистика поездок, web-UI и OTA.

- **Что выключено и почему.** `CONFIG_AA_ENABLE` (`depends on
  IDF_TARGET_ESP32P4`) убирает AA-стек: видеотракт держится на PPA-блиттере P4,
  рукопожатие с телефоном — на внешнем BT-агенте. Исходники выпадают из сборки
  в `main/CMakeLists.txt`, вызовы в `main.c` — по `#if`, `ui_mode`
  сворачивается в «единственный режим — дашборд».
- **Два чипа в одном дереве.** `scripts/build_board.sh` экспортирует
  `IDF_TARGET` под плату и даёт каждому чипу свой lock-файл
  (`dependencies.lock.esp32p4` / `.esp32s3`); корневой `CMakeLists.txt` по
  `IDF_TARGET` исключает компоненты чужого чипа (`EXCLUDE_COMPONENTS`), иначе
  менеджер компонентов упрётся в `targets: [esp32p4]` у P4-BSP.
  `managed_components/` общий — при переключении платы часть компонентов
  докачивается из кэша, это нормально.
- **BSP** `components/esp32_s3_touch_lcd_4/` — вендорский компонент из реестра,
  переложенный на `esp_lvgl_adapter` (как у P4), поэтому
  `main/display_init.c` один на все платы: `ROTATE_0` вместо `ROTATE_90`,
  `DOUBLE_DIRECT` в два PSRAM-фреймбуфера, DMA читает их через bounce-буферы
  во внутренней RAM. **Ревизия платы выбирается в Kconfig**
  (`BSP_S3T4_HW_V4` — помощник CH32V003 на 0x24 с PWM-подсветкой, дефолт;
  `BSP_S3T4_HW_V3` — экспандер TCA9554 на 0x20, подсветка только вкл/выкл):
  сначала определи ревизию (маркировка / кто отвечает на I2C), потом прошивай.
- **Железо, которого нет на S3**, спрятано за по одному шиму на каждое:
  `main/img_jpeg.{c,h}` (обложка трека и скриншоты debug-моста — аппаратный
  движок против `esp_new_jpeg`), анимированный splash из JPEG-кадров
  недоступен → играет `/vescfs/splash.gif` через LVGL, debug-мост говорит по
  USB-Serial-JTAG, `flash_shade` (гашение подсветки на запись флеша) — только
  P4: на S3 панель переживает запись за счёт
  `CONFIG_SPIRAM_FETCH_INSTRUCTIONS/RODATA` (код и константы в PSRAM, кэш не
  выключается под LCD DMA). Цена — ~2.5 МБ PSRAM под копию `.text/.rodata`,
  поэтому Wi-Fi-OTA умеет писать поток мимо стейджинга, а BLE-OTA при нехватке
  PSRAM честно отказывается и просит идти через Wi-Fi.
- **Дашборд 480×480** — второй GUI-Guider проект `Super_VESC_Display_480/`,
  сгенерированный из 800×480 скриптом
  `Super_VESC_Display/tools/make_480_project.py` (x и ширины ×0.6, круглые
  виджеты сохраняют размер и едут центром, у подписей ширина не меньше нужной
  тексту; в конце печатается список пар, которые стали пересекаться — их
  доводить глазами в GUI Guider). Проект владеет только `generated/` —
  `custom/`, шрифты, картинки и `import/` общие, имена виджетов совпадают,
  поэтому `custom/` и `theme_generic` собираются под обе геометрии.
  Экраны, собранные кодом, спрашивают ширину у дисплея через
  `Super_VESC_Display/custom/ui_geom.h` (`UI_W`/`UI_H`/`UI_SX()`).
  Симулятор: `cd Super_VESC_Display/lvgl-simulator && make dash480`.

## Воспроизведение игнорируемых артефактов

Каталоги `tools/` и `research/_sources/` намеренно не закоммичены
(см. `.gitignore`). Восстановить их можно так:

```bash
# Reference-исходники AA / dongle / Waveshare BSP
mkdir -p research/_sources && cd research/_sources
git clone --depth 1 https://github.com/f1xpl/aasdk
git clone --depth 1 https://github.com/f1xpl/openauto
git clone --depth 1 https://github.com/andreknieriem/headunit-revived headunit
git clone --depth 1 https://github.com/Nicba1010/WirelessAndroidAutoDongle
git clone --depth 1 https://github.com/waveshareteam/ESP32-P4-WIFI6-Touch-LCD-4.3 waveshare_p4_4_3
# Вендорские примеры и схема ESP32-S3-Touch-LCD-4 (ревизия платы, пины, тайминги)
git clone --depth 1 https://github.com/waveshareteam/ESP32-S3-Touch-LCD-4 waveshare_s3_4

# tools/c6_slave_fw — slave firmware build (нужно при апдейте network_adapter.bin)
cd <repo_root>/tools && idf.py create-project-from-example "espressif/esp_hosted^2.12.6:slave"
mv slave c6_slave_fw && cd c6_slave_fw && idf.py set-target esp32c6 && idf.py build
cp build/network_adapter.bin ../../components/c6_ota_partition/slave_fw_bin/

# tools/c6_ota_flasher — standalone OTA-flasher (на случай если основная прошивка
# поломается и нужно прошить C6 отдельным проектом)
cd .. && idf.py create-project-from-example "espressif/esp_hosted^2.12.6:host_performs_slave_ota"
mv host_performs_slave_ota c6_ota_flasher
```

## Прошивка C6 (ESP-Hosted slave) встраивается в основной бинарь

`network_adapter.bin` под ESP32-C6 лежит в
`components/c6_ota_partition/slave_fw_bin/` и через `EMBED_FILES`
встраивается в `esp32p4_android_auto.bin` как блоб в `.rodata`.
При старте `main/c6_ota.c` сравнивает версию C6 с версией хоста
(через `esp_hosted_get_coprocessor_fwversion`) и при несовпадении
шлёт встроенный блоб через SDIO как OTA-апдейт. После апдейта
P4 ребутится; на следующем заходе версии совпадают, OTA пропускается.