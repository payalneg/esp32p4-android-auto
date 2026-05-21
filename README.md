🇬🇧 **English** | 🇷🇺 [Русский](README.ru.md)

# ESP32-P4 Android Auto Head Unit 🚗📱

[![ESP-IDF](https://img.shields.io/badge/ESP--IDF-v5.5%2B-blue)](https://github.com/espressif/esp-idf)
[![Target](https://img.shields.io/badge/target-ESP32--P4-red)](https://www.espressif.com/en/products/socs/esp32-p4)
[![License](https://img.shields.io/badge/license-GPL--3.0-green)](LICENSE)
[![Board](https://img.shields.io/badge/board-Waveshare%204.3%22-orange)](https://www.waveshare.com/esp32-p4-wifi6-touch-lcd-4.3.htm)
[![Status](https://img.shields.io/badge/AA%20Wireless-working-success)]()

Wireless **Android Auto** head unit built on the ESP32-P4 with a 800×480 touch
display. Talks the native AA Wireless protocol — **no extra app on the phone**.
Pair once over Bluetooth, then Android Auto launches automatically on every
power-on. Bonus: a VESC CAN dashboard overlay for e-skates / e-bikes / DIY EVs.

![Android Auto running on a bike-mounted ESP32-P4 head unit](docs/images/hero.jpg)

> 🎬 Short ride demo: [`docs/demo.mp4`](docs/demo.mp4)

---

## ✨ Features

- 📺 **Native Android Auto Wireless** projection — no `Wireless Helper` APK,
  no developer mode tricks. Phone pairs over Classic Bluetooth, AA launches
  itself.
- 🎞️ **H.264 video decode** via `esp_h264` (SW decoder) + PPA-accelerated
  YUV420 → RGB565 shuffle on the ESP32-P4. **Native 800×480 @ ~10–15 fps**
  on the panel (phone streams at ~30 fps; the SW decoder + display pipeline
  is the bottleneck).
- 👆 **Touch input** forwarded to the phone (GT911 capacitive controller).
- 🔔 **System audio channel** (UI beeps / click feedback). Media / speech
  channels are deliberately dropped — phone keeps audio routing locally.
- 🛴 **VESC CAN dashboard overlay** — live battery %, speed, motor / FET
  temperatures, current draw, cruise indicator.
- 🔵 **BLE NUS bridge** — VESC Tool over BLE keeps working *while* Android
  Auto is projecting.
- 🆙 **HTTP OTA** with on-screen progress (`scripts/ota_push.sh`).
- 📦 **Embedded co-firmwares**: ESP32-C6 (`esp-hosted` slave) and the D1 Mini
  Bluetooth agent are bundled inside the main binary and auto-updated over
  SDIO / UART on version mismatch.
- ⚙️ **Settings screen**: WiFi / BT info, paired-phone history, firmware
  versions, in-PSRAM ring buffer log viewer.

---

## 📺 Screenshots

![VESC dashboard overlay — battery, speed, voltage, odometer, motor / controller temps](docs/images/vesc-hud.jpg)

<!-- TODO: Settings UI showing firmware versions / log viewer -->
![Settings UI](docs/images/settings.jpg)

---

## 🔧 Hardware

### Required

| Part | Notes |
|---|---|
| **Waveshare ESP32-P4-WIFI6-Touch-LCD-4.3** | [Shop](https://www.waveshare.com/esp32-p4-wifi6-touch-lcd-4.3.htm) — main board, 800×480 ST7701 MIPI-DSI panel, GT911 touch, on-board ESP32-C6 for WiFi over SDIO. |
| **D1 Mini ESP32-WROOM-32** | Any USB-C ESP32 dev board with **Classic Bluetooth**. Acts as the BT pairing front-end (ESP32-P4 and ESP32-C6 don't have BT Classic). ~$3. |
| Jumper wires, USB-C cable(s) | |

### Optional

| Part | Use |
|---|---|
| **TJA1051 CAN transceiver** (prefer the `T/3` variant) | VESC CAN bus → P4 GPIO |
| **DC-DC step-down 12V → 5V** (≥1 A) | Power from VESC / car battery |
| **CR2032 coin cell** on the H8 footprint | RTC backup (wall clock survives unplug) |
| **VESC** controller with CAN out | Live HUD overlay |

---

## 🔌 Wiring

<!-- TODO: close-up photo of the J3 header with D1 Mini wired in -->
![Wiring](docs/images/wiring.jpg)

### 1. Power chain

```
[12V battery / VESC bus]
        │
        ▼
 [DC-DC step-down  12V → 5V,  ≥1 A]
        │
        ▼   (USB-C or 5V pin)
 [Waveshare ESP32-P4 board]
        │  on-board LDO
        ▼
       3V3 rail ──────► D1 Mini ESP32 (3V3 pin)
```

> ⚠️ **Power the D1 Mini from the P4's 3V3 rail, NOT from 5V.** The D1 Mini's
> on-board AMS1117 will burn the extra volt as heat for no reason — the P4 board
> already gives you clean 3.3 V on the J3 header.

### 2. D1 Mini ESP32 ↔ ESP32-P4 (UART + OTA control)

The P4 board exposes a `J3` header on the bottom edge with the free expansion
pins. The USB-C debug console (GPIO 37/38) stays usable while this is wired up.

```
D1 Mini side                ESP32-P4 (J3 header)
─────────────────────────────────────────────────
GPIO 17 (TX2)    ────►      GPIO 22  (UART RX)
GPIO 16 (RX2)    ◄────      GPIO 21  (UART TX)
GPIO 5           ◄────      GPIO 24  (RST, for bt_agent OTA)
EN / IO0         ◄────      GPIO 25  (BOOT, for bt_agent OTA)
3V3              ◄────      3V3
GND              ────       GND
```

The RST/BOOT lines let the main P4 firmware automatically re-flash the BT
agent over UART when their versions diverge — you only ever flash the D1 Mini
manually once.

### 3. VESC CAN bus (optional — only if you want the HUD overlay)

```
[VESC CAN_H/CAN_L]──┬── [120 Ω terminator, if not already on the bus]
                    │
              [TJA1051 transceiver, 5V VCC]
                    │
        TXD ◄────── GPIO 48 (P4)            ← 3.3 V drives TJA1051 TXD directly
        RXD ──────► [resistor divider 5V→3.3V] ──► GPIO 47 (P4)
```

- **Divider values**: 1.8 kΩ (series) + 3.3 kΩ (to GND) — or 10 kΩ + 18 kΩ.
- **TJA1051 vs TJA1051T/3**: if you can choose, get the **T/3** variant — it
  has a dedicated `VIO` pin you can tie to 3.3 V and you can skip the divider
  entirely on RXD.
- Default CAN bitrate is **500 kbps**, controller ID **2** — both configurable
  in `idf.py menuconfig` under *VESC CAN* (`VESC_CAN_SPEED_KBPS`,
  `VESC_CAN_CONTROLLER_ID`).

---

## 🖨️ 3D-printable enclosure

STL / STEP files for the case live in [`3d-model/`](3d-model/). Print
settings, recommended materials and assembly notes will be added there as the
design firms up.

<!-- TODO: render or photo of the printed enclosure -->

---

## 🚀 Build & Flash

### Prerequisites

- **ESP-IDF v5.5 or newer** (tested with v5.5.3).
- Both targets need to be installed: `esp32p4` (main board) and `esp32` (D1 Mini
  BT agent).

```bash
git clone --recursive https://github.com/espressif/esp-idf.git
cd esp-idf && ./install.sh esp32,esp32p4
. ./export.sh
```

### 1. Main firmware — ESP32-P4

```bash
cd esp32p4-android-auto
idf.py set-target esp32p4
idf.py -p /dev/cu.usbmodem* flash monitor
```

> The ESP32-C6 WiFi co-firmware (`network_adapter.bin`) is bundled into the
> main binary via `EMBED_FILES` and pushed to the C6 over SDIO on boot when
> versions don't match — you don't flash the C6 separately.

### 2. BT agent firmware — D1 Mini ESP32

You only need this **once**. After that, the P4 reflashes the agent over UART
whenever the embedded version is newer.

```bash
cd tools/bt_agent
idf.py set-target esp32
idf.py -p /dev/cu.usbserial-* flash monitor
```

See [`tools/bt_agent/README.md`](tools/bt_agent/README.md) for the full agent
boot log walkthrough and what the SSP pairing dialogue looks like.

### 3. OTA updates after the first flash

Once the head unit is on its SoftAP (default IP `192.168.4.1`), push new
firmware over HTTP from any laptop joined to the same AP:

```bash
scripts/ota_push.sh 192.168.4.1
```

You'll see a progress bar on the device screen, then it reboots into the new
slot.

---

## 📱 First connection

1. Power the head unit. The screen shows **"Waiting for phone"** with the
   SoftAP SSID / IP.
2. On the phone: *Settings → Bluetooth → Pair new device → **ESP32-P4 AA***.
3. Accept the SSP pairing prompt on both sides.
4. The phone joins the head unit's WiFi automatically and launches Android
   Auto. The screen flips to the AA projection.

After the first pairing the head unit remembers the phone, and every subsequent
power-on auto-reconnects without prompts.

---

## 🗺️ Roadmap / Status

| Area | Status | Notes |
|---|---|---|
| AA Wireless video (H.264) | ✅ | Native 800×480 @ ~10–15 fps; bottleneck is SW decode + RGB shuffle |
| Touch input forwarding | ✅ | GT911 → AA `TouchEvent` protobuf |
| System audio channel | ✅ | UI beeps; required by Gearhead to project at all |
| VESC CAN dashboard overlay | ✅ | Battery %, speed, temps, cruise indicator |
| BLE NUS (VESC Tool bridge) | ✅ | Works concurrently with AA |
| HTTP OTA + on-screen progress | ✅ | `scripts/ota_push.sh` |
| Settings UI + PSRAM log viewer | ✅ | Logs survive resets, viewable on device |
| Auto-reconnect to last phone | ✅ | Bonded list in NVS on the BT agent |
| Media / Speech audio channels | 🟡 | Dropped on purpose — no audio sink |
| Pure BT Classic on P4 (no D1 Mini) | ❌ | Not possible — ESP32-P4 has no BT Classic radio |
| Native nav rendering (non-AA) | ❌ | Not planned |

---

## 📁 Repository layout

```
.
├── main/                       # ESP32-P4 firmware — AA stack, video, UI, OTA, VESC, BLE
├── components/
│   ├── esp32_p4_wifi6_touch_lcd_4_3/  # Waveshare BSP (LVGL, ST7701 DSI, GT911 touch)
│   ├── bt_agent_fw/            # Embeddable bt_agent firmware blob
│   ├── c6_ota_partition/       # Embedded ESP32-C6 firmware (network_adapter.bin)
│   ├── vesc_can/               # CAN driver for VESC (RT data + LISP poll)
│   ├── vesc_ui/                # Dashboard UI for the VESC overlay
│   ├── dev_settings/           # Settings screen + persisted prefs
│   ├── log_capture/            # PSRAM ring-buffer logger
│   └── qr_info/                # QR code with WiFi creds for the phone
├── tools/
│   ├── bt_agent/               # D1 Mini ESP32 firmware (Classic BT + SPP)
│   ├── c6_slave_fw/            # Sources of the bundled C6 firmware (gitignored, see CLAUDE.md)
│   └── c6_ota_flasher/         # Standalone fallback C6 flasher
├── scripts/                    # capture.sh (Wireshark), ota_push.sh, extract_yuv.py
├── 3d-model/                   # STL / STEP files for the printed enclosure
├── docs/images/                # Photos / screenshots used by this README
├── research/                   # Reference upstream sources (gitignored)
├── partitions.csv              # Dual-OTA layout (both slots below 16 MB boundary)
├── CLAUDE.md                   # Original architecture notes / Mode A vs B design
└── README.md
```

---

## 🙏 Credits & references

- [**aasdk**](https://github.com/f1xpl/aasdk) and [**openauto**](https://github.com/f1xpl/openauto) by *f1xpl* — the AA Wireless protocol bible.
- [**headunit-revived**](https://github.com/andreknieriem/headunit-revived) by *andreknieriem* — wireless-mode reference.
- [**WirelessAndroidAutoDongle**](https://github.com/Nicba1010/WirelessAndroidAutoDongle) by *Nicba1010* — Raspberry Pi based AA dongle.
- [**esp-h264**](https://github.com/espressif/esp-h264) and [**esp-hosted**](https://github.com/espressif/esp-hosted) by *Espressif*.
- [**Waveshare ESP32-P4-WIFI6-Touch-LCD-4.3 wiki**](https://github.com/waveshareteam/ESP32-P4-WIFI6-Touch-LCD-4.3) — board BSP and example code.

---

## 📜 License

Released under the **GNU General Public License v3.0** — see [`LICENSE`](LICENSE).
