🇷🇺 **Русский** | 🇬🇧 [English](README.md)

# VESC Dashboard на ESP32-P4 🛴⚡

[![ESP-IDF](https://img.shields.io/badge/ESP--IDF-v5.5%2B-blue)](https://github.com/espressif/esp-idf)
[![Target](https://img.shields.io/badge/target-ESP32--P4-red)](https://www.espressif.com/en/products/socs/esp32-p4)
[![License](https://img.shields.io/badge/license-GPL--3.0-green)](LICENSE)
[![Board](https://img.shields.io/badge/board-Waveshare%204.3%22-orange)](https://www.waveshare.com/esp32-p4-wifi6-touch-lcd-4.3.htm)
[![VESC](https://img.shields.io/badge/VESC-CAN-success)]()

Open-source **800×480 тач-дашборд для VESC-транспорта** — электроскейтов,
электровелосипедов, самокатов, DIY-электромобилей. В реальном времени:
батарея %, скорость, температуры мотора и контроллера, напряжение, ток,
одометр — всё прямо с CAN-шины VESC. Заодно играет роль BLE-моста для
VESC Tool — настраиваешь контроллер прямо во время поездки.

**Бонусом**: то же самое устройство умеет в нативный Android Auto Wireless —
спариваешь телефон по Bluetooth и Google Maps / Spotify / Cycleway проецируются
на тот же экран. Никаких приложений на телефон ставить не надо.

![VESC дашборд — батарея, скорость, напряжение, одометр, температуры мотора и контроллера](docs/images/hero.jpg)

> 🎬 Короткое демо на ходу: [`docs/demo.mp4`](docs/demo.mp4)

---

## ✨ Возможности

### 🛴 Основное — VESC дашборд
- **Живая телеметрия по CAN**: батарея %, скорость, напряжение, ток,
  температуры мотора и FET, одометр, trip, остаточный запас хода,
  индикатор круиз-контроля.
- **BLE NUS мост** — VESC Tool по Bluetooth LE общается с контроллером
  через этот дашборд, отдельный адаптер не нужен.
- **Экран настроек** прямо на устройстве: CAN bitrate, controller ID,
  единицы измерения, просмотр лога из PSRAM ring buffer.
- **HTTP OTA** с прогрессом на экране (`scripts/ota_push.sh`).

### 📺 Бонус — беспроводной Android Auto
- Нативная **Android Auto Wireless** проекция на том же 800×480 экране —
  никаких `Wireless Helper` APK, никаких developer-режимов. Один раз
  спариваешь телефон по Classic Bluetooth, дальше AA сам стартует при
  каждом включении.
- Декодирование H.264 через `esp_h264` (SW-декодер) + PPA-ускоренная
  конверсия YUV420 → RGB565. Нативно 800×480 @ ~10–15 fps на экране.
- Сенсорный ввод проксируется в телефон (GT911 ёмкостный контроллер).
- UI-бипы / клики (System audio канал — без него Gearhead отказывается
  проецировать).
- Авто-реконнект к последнему спаренному телефону.

![Android Auto Cycleway navigation на том же экране](docs/images/aa-bonus.jpg)

### 📦 Под капотом
- **Встроенные ко-прошивки**: ESP32-C6 (`esp-hosted` WiFi slave) и D1 Mini
  Bluetooth-агент зашиты в основной бинарь и автоматически обновляются
  по SDIO / UART при несовпадении версий.
- **Dual-OTA** layout в 32 MB flash (оба слота ниже границы 16 MB).

---

## 🔧 Железо

| Деталь | Для чего | Заметки |
|---|---|---|
| **Waveshare ESP32-P4-WIFI6-Touch-LCD-4.3** | всегда | [Магазин](https://www.waveshare.com/esp32-p4-wifi6-touch-lcd-4.3.htm). Главный мозг — ESP32-P4 + ESP32-C6 для WiFi на борту, 800×480 ST7701 MIPI-DSI, GT911 тач. |
| **VESC контроллер с CAN** | VESC дашборд | Любая ревизия с CAN-выходом. Это вообще весь смысл. |
| **TJA1051 CAN-трансивер** (лучше `T/3`) | VESC дашборд | Согласование CAN 5В → 3.3В |
| **DC-DC step-down 12В → 5В** (≥1 А) | VESC дашборд | Питание от VESC / батареи транспорта |
| **Модуль ESP32-WROOM-32** (или любая плата с ним) | бонус AA | Голый модуль WROOM-32 можно припаять прямо к плате P4 — включаешь `BT_AGENT_OTA_ENABLED=y` в `idf.py menuconfig`, и P4 сам его прошивает по UART + RST/BOOT линиям, USB-to-serial не нужен. Если паять не хочется — подойдёт D1 Mini ESP32 (или любая USB-C плата на ESP32 с Classic BT). У ESP32-P4 / C6 нет BT Classic. ~$2–3. |
| Провода-перемычки, USB-C кабели | всегда | |

Если Android Auto не нужен — WROOM можно вообще не ставить, дашборд
работает сам по себе. Если на UART ничего не подключено и
`BT_AGENT_OTA_ENABLED` оставлен в дефолте (`n`), P4 на каждой загрузке
просто молча пропускает всю BT-цепочку.

---

## 🔌 Подключение

<!-- TODO: фото с подключением VESC + D1 Mini к J3 хедеру -->
![Подключение](docs/images/wiring.jpg)

### 1. Цепь питания

```
[Батарея 12В / шина VESC]
        │
        ▼
 [DC-DC step-down  12В → 5В,  ≥1 А]
        │
        ▼   (USB-C или 5V pin)
 [Плата Waveshare ESP32-P4]
        │  встроенный LDO
        ▼
       3V3 ──────► (опционально) D1 Mini ESP32 3V3 pin
```

> ⚠️ Если ставишь D1 Mini ради AA-бонуса, **питай его от 3.3В шины P4,
> НЕ от 5В**. Встроенный на D1 Mini AMS1117 просто будет греть лишний
> вольт впустую — на J3 хедере P4 уже выдаются чистые 3.3 В.

### 2. VESC CAN шина (основное)

```
[VESC CAN_H/CAN_L]──┬── [терминатор 120 Ом, если ещё не стоит на шине]
                    │
              [TJA1051 трансивер, 5В VCC]
                    │
        TXD ◄────── GPIO 48 (P4)            ← 3.3В драйвит TJA1051 TXD напрямую
        RXD ──────► [делитель 5В→3.3В] ──── GPIO 47 (P4)
```

- **Номиналы делителя**: 1.8 кОм (последовательно) + 3.3 кОм (на землю) или
  10 кОм + 18 кОм.
- **TJA1051 vs TJA1051T/3**: если есть выбор — берите **T/3** версию. У неё
  есть отдельный пин `VIO`, который заводят на 3.3В, и тогда делитель на RXD
  не нужен совсем.
- По умолчанию **500 kbps**, controller ID **2** — обе настройки в
  `idf.py menuconfig`, секция *VESC CAN* (`VESC_CAN_SPEED_KBPS`,
  `VESC_CAN_CONTROLLER_ID`).

### 3. D1 Mini ESP32 ↔ P4 (только если нужен AA-бонус)

J3 хедер — нижний торец платы Waveshare, там свободные пины расширения.
USB-C debug консоль (GPIO 37/38) остаётся свободной при этом подключении.

```
WROOM сторона               ESP32-P4 (J3 хедер)
─────────────────────────────────────────────────
GPIO 17 (TX2)    ────►      GPIO 22  (UART RX)
GPIO 16 (RX2)    ◄────      GPIO 21  (UART TX)
EN (он же RST)   ◄────      GPIO 24  (RST, для OTA bt_agent)
GPIO 0 (BOOT)    ◄────      GPIO 25  (IO0, для OTA bt_agent)
3V3              ◄────      3V3
GND              ────       GND
```

`EN` / `GPIO 0` — стандартная связка reset + boot-mode у любого ESP32,
именно её дёргает `esp_serial_flasher`. На D1 Mini-плате они уже выведены
на хедер: `EN` подписан как есть, `GPIO 0` обычно сидит на пине `D3`.

RST/BOOT линии позволяют основной прошивке P4 автоматически перепрошивать
BT-агент по UART, если версии разъехались. Так что D1 Mini вы прошиваете
руками **только один раз**.

---

## 🚀 Сборка и прошивка

### Что нужно

- **ESP-IDF v5.5+** (проверено на v5.5.3).
- Установить таргет `esp32p4` обязательно; `esp32` — только если нужен
  Android Auto бонус.

```bash
git clone --recursive https://github.com/espressif/esp-idf.git
cd esp-idf && ./install.sh esp32,esp32p4
. ./export.sh
```

### 1. Основная прошивка — ESP32-P4 (дашборд + AA)

```bash
cd esp32p4-android-auto
idf.py set-target esp32p4
idf.py -p /dev/cu.usbmodem* flash monitor
```

> WiFi прошивка ESP32-C6 (`network_adapter.bin`) встроена в основной бинарь
> через `EMBED_FILES` и заливается на C6 по SDIO при старте, если версии не
> совпадают. Отдельно C6 прошивать не нужно.

### 2. (Опционально) Прошивка BT-агента — D1 Mini ESP32

Нужна только если хочется Android Auto бонус. Два варианта:

**A. Дать P4 прошить её самому (для голого припаянного WROOM — самое то).**
Включаешь `BT_AGENT_OTA_ENABLED=y` в `idf.py menuconfig` (раздел
*Project → BT Agent OTA*) и пересобираешь. На каждой загрузке P4 читает
`BT-VER:` строку с агента по UART и, если версия не совпадает, перешивает
агент блобом из `components/bt_agent_fw/`. Голый припаянный WROOM-32
поднимается полностью провижионированным при первом же включении. (Дефолт
этой опции — `n`; без неё BT-путь становится no-op, даже если WROOM
подключён.)

**B. Прошить руками.** Если есть дев-плата с USB:

```bash
cd tools/bt_agent
idf.py set-target esp32
idf.py -p /dev/cu.usbserial-* flash monitor
```

Полный walk-through boot-лога агента и как выглядит SSP pairing — в
[`tools/bt_agent/README.md`](tools/bt_agent/README.md).

### 3. VESC LISP скрипт — для круиз-контроля и профилей скорости

[`lisp/main.lisp`](lisp/main.lisp) выполняется **на самом VESC контроллере**
(не на P4). Добавляет круиз-контроль по кнопке на PPM-пине `RX`, три
профиля скорости по кнопке на `TX`, и отдаёт состояние круиза в дашборд
по LISP poll каналу — именно это зажигает индикатор круиза на экране.

Открываешь *VESC Tool → VESC Packages → Lisp Scripting*, грузишь
`lisp/main.lisp`, **Upload** → **Activate** → сохранить во flash. Подробности
и как поменять пресеты скорости — в [`lisp/README.md`](lisp/README.md).

Без этого скрипта дашборд продолжает показывать живую телеметрию — только
индикатор круиза и бипы при переключении профилей не работают.

### 4. OTA-обновления после первой прошивки

Когда head unit поднял свою SoftAP (по умолчанию IP `192.168.4.1`), можно
пушить новые прошивки по HTTP с ноутбука, подключённого к той же AP:

```bash
scripts/ota_push.sh 192.168.4.1
```

На экране устройства появится прогресс, после загрузки оно ребутнётся в
новый OTA-слот.

---

## 📱 Как пользоваться

### Режим дашборда (по умолчанию)

Подаёшь питание — на экране сразу появляется дашборд с тем, что VESC
выдаёт по CAN: батарея, скорость, температуры и т.д. Тапнуть в Settings —
там единицы измерения, CAN bitrate, controller ID, просмотр лога.

Параллельно с дашбордом **к устройству можно подключиться из VESC Tool
по Bluetooth LE** (NUS мост) — точно так же, как к любому VESC адаптеру.

### Переключение между режимами

**Тап тремя пальцами** (любые три пальца на экран на ~100 мс) переключает
между **VESC дашбордом** и **Android Auto проекцией**. Жест работает в
обе стороны, в любом режиме, и срабатывает ровно один раз за жест —
пальцы должны полностью оторваться от экрана, чтобы он мог сработать
снова. Так что в AA-режиме он не утечёт в телефон как случайный тач.

### Android Auto бонус (после прошивки BT-агента)

1. На экране в углу также висит **«Waiting for phone»** с SSID / IP
   SoftAP.
2. На телефоне: *Настройки → Bluetooth → Добавить устройство → **ESP32-P4 AA***.
3. Принять SSP pairing prompt.
4. Телефон сам подключается к WiFi head unit'а и запускает Android Auto.
   Экран переключается на AA-проекцию; VESC дашборд продолжает работать
   как оверлей (батарея %, скорость) поверх видео AA.

После первого спаривания head unit запоминает телефон, и каждое следующее
включение — авто-реконнект без диалогов.

---

## 🗺️ Roadmap / Статус

| Что | Статус | Комментарии |
|---|---|---|
| VESC RT-данные по CAN | ✅ | Батарея %, скорость, напряжение, ток, температуры, одометр |
| VESC LISP poll | ✅ | Индикатор круиза + кастомные счётчики (требует [`lisp/main.lisp`](lisp/main.lisp) залитого в контроллер) |
| BLE NUS мост (VESC Tool по BLE) | ✅ | Работает одновременно с AA |
| Settings UI + log viewer | ✅ | Логи переживают ребут, читаются прямо с устройства |
| HTTP OTA + прогресс на экране | ✅ | `scripts/ota_push.sh` |
| (Бонус) AA Wireless видео (H.264) | ✅ | Нативно 800×480 @ ~10–15 fps; упирается в SW-декод + RGB-шаффл |
| (Бонус) Touch input → телефон | ✅ | GT911 → AA `TouchEvent` protobuf |
| (Бонус) System audio канал | ✅ | UI-бипы; без них Gearhead не проецирует вообще |
| (Бонус) Авто-реконнект к последнему телефону | ✅ | NVS bonded list на BT-агенте |
| Media / Speech audio каналы | 🟡 | Отключены намеренно — нет аудио-выхода |
| Чистый BT Classic на P4 (без D1 Mini) | ❌ | Невозможно — у ESP32-P4 нет радио BT Classic |

---

## 🖨️ Корпус под 3D-печать

STL / STEP файлы корпуса лежат в [`3d-model/`](3d-model/). Параметры печати,
рекомендации по материалу и инструкция сборки будут дописываться там по мере
финализации дизайна.

<!-- TODO: рендер или фото распечатанного корпуса -->

---

## 📁 Структура репозитория

```
.
├── main/                       # Прошивка ESP32-P4 — VESC, UI, AA, OTA, BLE
├── components/
│   ├── esp32_p4_wifi6_touch_lcd_4_3/  # BSP Waveshare (LVGL, ST7701 DSI, GT911 тач)
│   ├── vesc_can/               # CAN-драйвер для VESC (RT data + LISP poll)
│   ├── vesc_ui/                # UI дашборда
│   ├── dev_settings/           # Экран настроек + персист
│   ├── log_capture/            # PSRAM ring-buffer логгер
│   ├── bt_agent_fw/            # Встраиваемый блоб прошивки bt_agent (AA бонус)
│   ├── c6_ota_partition/       # Встраиваемая прошивка ESP32-C6 (network_adapter.bin)
│   └── qr_info/                # QR-код с WiFi creds для телефона
├── tools/
│   ├── bt_agent/               # Прошивка D1 Mini ESP32 (Classic BT + SPP)
│   ├── c6_slave_fw/            # Исходники встроенной C6-прошивки (gitignored, см. CLAUDE.md)
│   └── c6_ota_flasher/         # Запасной standalone-флешер C6
├── scripts/                    # capture.sh (Wireshark), ota_push.sh, extract_yuv.py
├── lisp/                       # VESC LISP скрипт (круиз + профили скорости) — выполняется на VESC, не на P4
├── 3d-model/                   # STL / STEP файлы корпуса под 3D-печать
├── docs/images/                # Фото / скриншоты для этого README
├── research/                   # Reference-исходники апстримов (gitignored)
├── partitions.csv              # Dual-OTA layout (оба слота ниже границы 16 MB)
├── CLAUDE.md                   # Изначальный архитектурный дизайн / история
└── README.md
```

---

## 🙏 Благодарности и ссылки

- **[Проект VESC](https://vesc-project.com/)** от *Benjamin Vedder* —
  open-source мотор-контроллер, ради которого этот дашборд и существует.
- [**aasdk**](https://github.com/f1xpl/aasdk) и [**openauto**](https://github.com/f1xpl/openauto) от *f1xpl* — референс по AA Wireless протоколу (для бонусного режима).
- [**headunit-revived**](https://github.com/andreknieriem/headunit-revived) от *andreknieriem* — референс по wireless-режимам.
- [**esp-h264**](https://github.com/espressif/esp-h264) и [**esp-hosted**](https://github.com/espressif/esp-hosted) от *Espressif*.
- [**Wiki Waveshare ESP32-P4-WIFI6-Touch-LCD-4.3**](https://github.com/waveshareteam/ESP32-P4-WIFI6-Touch-LCD-4.3) — BSP и примеры.

---

## 📜 Лицензия

Распространяется под **GNU General Public License v3.0** — см. [`LICENSE`](LICENSE).
