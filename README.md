🇬🇧 **English** | 🇷🇺 [Русский](README.ru.md)

# VESC Dashboard for ESP32-P4 🛴⚡

[![ESP-IDF](https://img.shields.io/badge/ESP--IDF-v5.5%2B-blue)](https://github.com/espressif/esp-idf)
[![Target](https://img.shields.io/badge/target-ESP32--P4-red)](https://www.espressif.com/en/products/socs/esp32-p4)
[![License](https://img.shields.io/badge/license-GPL--3.0-green)](LICENSE)
[![Board](https://img.shields.io/badge/board-Waveshare%204.3%22-orange)](https://www.waveshare.com/esp32-p4-wifi6-touch-lcd-4.3.htm)
[![VESC](https://img.shields.io/badge/VESC-CAN-success)]()

Open-source **800×480 touch dashboard for VESC-powered e-vehicles** —
electric skateboards, e-bikes, scooters, DIY EVs. Live battery percent,
speed, motor / controller temps, voltage, current, odometer — straight off
the VESC CAN bus. Acts as a BLE bridge to VESC Tool so you can tune the
controller while the dashboard is running.

**Bonus**: the same device also speaks the native Android Auto Wireless
protocol — pair your phone over Bluetooth and Google Maps / Spotify /
Cycleway projects onto the same screen. No phone app required.

![VESC dashboard — battery, speed, voltage, odometer, motor / controller temps](docs/images/hero.jpg)

> 🎬 Short ride demo: [`docs/demo.mp4`](docs/demo.mp4) · [`docs/demo-2.mp4`](docs/demo-2.mp4) · [`docs/demo-3.mp4`](docs/demo-3.mp4)

---

## ✨ Features

### 🛴 Primary — VESC dashboard
- **Live CAN telemetry**: battery %, speed, voltage, current draw, motor
  and FET temperatures, odometer, trip, range estimate, cruise indicator.
- **Trip tracking** — on-screen trip statistics window (distance, Wh,
  efficiency, charts) + a **Reset trip** button, backed by a raw circular
  trip log on a dedicated `triplog` flash partition that survives reboots.
- **On-device VESC Tool config menu** — read / write the controller's motor
  and app config straight from the dashboard screen, run FOC detection, keep
  config / trip backups — no laptop required. Parameter tables are
  code-generated from the VESC Tool XML.
- **BLE NUS bridge** — VESC Tool over Bluetooth Low Energy talks straight
  to the controller through this dashboard, no extra adapter needed.
- **Settings UI** on the device: CAN bitrate, controller ID, persisted
  units / preferences, in-PSRAM ring buffer log viewer.
- **HTTP OTA** with on-screen progress (`scripts/ota_push.sh`).

### 📺 Bonus — Wireless Android Auto
- Native **Android Auto Wireless** projection on the same 800×480 panel —
  no `Wireless Helper` APK, no developer-mode tricks. Pair phone over
  Classic BT once, AA launches itself on every power-on.
- H.264 video decode via `esp_h264` (SW decoder) + PPA-accelerated
  YUV420 → RGB565 shuffle. Native 800×480 @ ~10–15 fps on the panel.
- Touch input forwarded to the phone (GT911 capacitive controller).
- UI beeps / click feedback (System audio channel — required by Gearhead).
- Auto-reconnect to the last paired phone.

![Android Auto Cycleway navigation projected on the same screen](docs/images/aa-bonus.jpg)

### 📱 Companion app (optional)
`aa_bridge` — a small Flutter app ([`flutter-application/`](flutter-application/))
that talks to the head unit over BLE, independently of Android Auto:
- **Notification & media bridge** — phone notifications and now-playing
  metadata / album art mirrored onto the dashboard.
- **Wall clock** — pushes local time over BLE so the dashboard shows `HH:MM`
  with no on-device RTC / coin cell.
- **Firmware update over WiFi** — the P4 image is bundled in the APK and
  flashed to the head unit over its SoftAP, straight from the phone. Creds
  are read over BLE automatically, or entered / scanned manually.

### 📦 Under the hood
- **Embedded co-firmwares**: ESP32-C6 (`esp-hosted` WiFi slave) and the
  D1 Mini Bluetooth agent are bundled inside the main binary and auto-flashed
  over SDIO / UART on version mismatch.
- **Dual-OTA** layout in 32 MB flash (both slots below the 16 MB boundary),
  plus a dedicated ~12 MB `triplog` partition above it for the raw trip log.

---

## 🔧 Hardware

| Part | Used for | Notes |
|---|---|---|
| **Waveshare ESP32-P4-WIFI6-Touch-LCD-4.3** | always | [Shop](https://www.waveshare.com/esp32-p4-wifi6-touch-lcd-4.3.htm). Main brain — ESP32-P4 + on-board ESP32-C6 WiFi, 800×480 ST7701 MIPI-DSI, GT911 touch. |
| **VESC controller with CAN out** | VESC dashboard | Any rev with CAN. This is the whole point. |
| **TJA1051 CAN transceiver** (prefer `T/3`) | VESC dashboard | 5V → 3.3V CAN level translation |
| **DC-DC step-down 12V → 5V** (≥1 A) | VESC dashboard | Power from VESC / vehicle battery |
| **ESP32-WROOM-32 module** (or any dev board with one) | Android Auto bonus | A bare WROOM-32 module soldered to the P4 board works fine — flip `BT_AGENT_OTA_ENABLED=y` in `idf.py menuconfig` and the P4 flashes it itself over the UART + RST/BOOT lines, no USB-to-serial needed. A D1 Mini ESP32 (or any USB-C ESP32 dev board with Classic BT) works too if you'd rather flash it directly. ESP32-P4 / C6 don't have BT Classic. ~$2–3. |
| Jumper wires, USB-C cables | always | |

If you don't care about Android Auto, you can drop the WROOM entirely —
the dashboard works standalone. With nothing connected on UART and
`BT_AGENT_OTA_ENABLED` left at its default `n`, the P4 silently no-ops
the BT path on every boot.

---

## 🔌 Wiring

<!-- TODO: close-up photo of the J3 header with VESC + D1 Mini wired in -->

### 1. Power chain

```mermaid
flowchart LR
    BAT["🔋 12V battery<br/>/ VESC bus"]
    DCDC["DC-DC<br/>12V → 5V<br/>≥1 A"]
    P4["Waveshare<br/>ESP32-P4 board<br/>(USB-C / 5V pin)"]
    V3V3["3V3 rail<br/>(J3 header)"]
    WROOM["(optional)<br/>WROOM 3V3 pin"]
    BAT -->|+12V| DCDC -->|+5V| P4 -->|on-board LDO| V3V3 --> WROOM
```

> ⚠️ If you wire up a D1 Mini for the AA bonus, **power it from the P4's
> 3V3 rail, not from 5V**. The D1 Mini's on-board AMS1117 will burn the
> extra volt as heat for no reason — the P4 board already gives you clean
> 3.3 V on the J3 header.

### 2. VESC CAN bus (primary)

```mermaid
flowchart LR
    subgraph P4["ESP32-P4 (3.3V GPIO)"]
        direction TB
        P48(["GPIO 48 — TWAI TX (output)"])
        P47(["GPIO 47 — TWAI RX (input)"])
        P5V(["5V"])
        PGND(["GND"])
    end

    subgraph XCVR["TJA1051 (prefer T/3 variant)"]
        direction TB
        TXD(["TXD (input)"])
        RXD(["RXD (output)"])
        VCC(["VCC"])
        XGND(["GND"])
        CANH(["CANH"])
        CANL(["CANL"])
    end

    subgraph BUS["CAN bus"]
        VESC["VESC controller"]
        TERM["120Ω resistor<br/>across CANH ↔ CANL"]
    end

    P48 -->|"3.3V drives TXD directly"| TXD
    RXD -->|"5V — divider 1.8k + 3.3k → 3.3V"| P47
    P5V --> VCC
    PGND --- XGND
    CANH <--> VESC
    CANL <--> VESC
    CANH -.- TERM -.- CANL
```

Pin direction is from the MCU's perspective (NXP convention): on the
TJA1051, **TXD is an input** (MCU drives it), **RXD is an output**
(transceiver drives the MCU).

- **Divider values on RXD**: 1.8 kΩ (series) + 3.3 kΩ (to GND) — or 10 kΩ + 18 kΩ.
- **TJA1051 vs TJA1051T/3**: if you can choose, get the **T/3** variant — it
  has a dedicated `VIO` pin you can tie to 3.3 V and you can skip the divider
  entirely on RXD.
- Default CAN bitrate is **500 kbps**, controller ID **2** — both configurable
  in `idf.py menuconfig` under *VESC CAN* (`VESC_CAN_SPEED_KBPS`,
  `VESC_CAN_CONTROLLER_ID`).

### 3. D1 Mini ESP32 ↔ P4 (only for the AA bonus)

The P4 board exposes a `J3` header on the bottom edge with the free expansion
pins. The USB-C debug console (GPIO 37/38) stays usable while this is wired up.

```mermaid
flowchart LR
    subgraph WROOM["ESP32-WROOM-32<br/>(bare module or D1 Mini dev board)"]
        direction TB
        W17(["GPIO 17 (TX2) — UART TX (out)"])
        W16(["GPIO 16 (RX2) — UART RX (in)"])
        WEN(["EN — chip reset (input)"])
        WIO0(["GPIO 0 — BOOT-mode select (input)"])
        W3V3(["3V3 (power in)"])
        WGND(["GND"])
    end

    subgraph P4J3["ESP32-P4 (J3 header)"]
        direction TB
        P22(["GPIO 22 — UART1 RX (in)"])
        P21(["GPIO 21 — UART1 TX (out)"])
        P24(["GPIO 24 — drives WROOM RST"])
        P25(["GPIO 25 — drives WROOM IO0"])
        P3V3(["3V3 out"])
        PGND(["GND"])
    end

    W17 -->|UART data| P22
    P21 -->|UART data| W16
    P24 -->|reset pulse| WEN
    P25 -->|boot select| WIO0
    P3V3 -->|3.3V power| W3V3
    PGND --- WGND
```

`EN` / `GPIO 0` are the standard ESP32 reset + boot-mode pins — same combo
the `esp_serial_flasher` uses on any ESP32 chip; on D1 Mini dev boards
they're already broken out to the `EN` and `D3` (= GPIO 0) headers
respectively.

The RST/BOOT lines let the main P4 firmware automatically re-flash the BT
agent over UART when their versions diverge — you only ever flash the D1 Mini
manually once.

---

## 🚀 Build & Flash

### Prerequisites

- **ESP-IDF v5.5 or newer** (tested with v5.5.3).
- Install both targets — `esp32p4` is mandatory, `esp32` only if you want
  the Android Auto bonus.

```bash
git clone --recursive https://github.com/espressif/esp-idf.git
cd esp-idf && ./install.sh esp32,esp32p4
. ./export.sh
```

### 1. Main firmware — ESP32-P4 (dashboard + AA)

```bash
cd esp32p4-android-auto
idf.py set-target esp32p4
idf.py -p /dev/cu.usbmodem* flash monitor
```

A plain `idf.py` build targets the **Waveshare 4.3"** board (the default).

> The ESP32-C6 WiFi co-firmware (`network_adapter.bin`) is bundled into the
> main binary via `EMBED_FILES` and pushed to the C6 over SDIO on boot when
> versions don't match — you don't flash the C6 separately.

#### Multiple boards

The firmware supports more than one ESP32-P4 head-unit board. Pick the board
with `scripts/build_board.sh <board> <idf.py args…>`, which uses a per-board
build directory and layers the matching `sdkconfig.defaults.<board>` overlay:

| Board | Slug | Flash | Notes |
|---|---|---|---|
| Waveshare ESP32-P4-WIFI6-Touch-LCD-4.3 | `waveshare` | 32 MB | default (also what plain `idf.py` builds) |
| Guition JC4880P443C_I_W | `jc4880` | 16 MB | ST7701S, smaller partition table (`partitions_16mb.csv`) |

```bash
scripts/build_board.sh                              # build firmware for ALL boards
scripts/build_board.sh waveshare flash monitor
scripts/build_board.sh jc4880 -p /dev/cu.usbmodem* flash monitor
```

Run with no board (or `all`) to build every board's image in one go.

What differs per board (everything else — WiFi/SDIO→C6, touch I2C, SD,
most of the I2S bus — is shared): MIPI-DSI panel timing + vendor init, LCD
backlight/reset pins, the BT-agent UART pins, the CAN RX/TX pins, flash size
and the partition table. The board is chosen by the Kconfig `BOARD_MODEL`
choice (`CONFIG_BOARD_WAVESHARE_43` / `CONFIG_BOARD_JC4880P443C`); see the
`#if CONFIG_BOARD_JC4880P443C` branches in the BSP and `main/bt_link.h`.

**JC4880P443C pin map** (free-header pins): BT-agent UART `TX=33 RX=31`,
`RST=30 IO0=29`; CAN `RX=52 TX=51`; LCD backlight `23`, reset `5`.
The 16 MB layout fits two 5 MB OTA slots + 1 MB storage + ~4.9 MB trip log —
the app image (~3.8 MB) has ~1.2 MB (24%) of headroom in each slot, so watch
the size as the firmware grows.

### 2. (Optional) BT agent firmware — D1 Mini ESP32

Only needed if you want the Android Auto bonus. Two ways to do it:

**A. Let the P4 flash it (best for a bare soldered WROOM).** Enable
`BT_AGENT_OTA_ENABLED=y` in `idf.py menuconfig` (under
*Project → BT Agent OTA*) and rebuild. On every boot the P4 checks the
agent's `BT-VER:` line over UART and reflashes it from the firmware blob
embedded in `components/bt_agent_fw/` if the version doesn't match. So a
bare soldered WROOM-32 comes up fully provisioned the first time you power
on. (Default is `n` — without it the BT path is a no-op even if a WROOM
is wired up.)

**B. Flash it yourself.** Plug a dev board with USB into your laptop:

```bash
cd tools/bt_agent
idf.py set-target esp32
idf.py -p /dev/cu.usbserial-* flash monitor
```

See [`tools/bt_agent/README.md`](tools/bt_agent/README.md) for the full agent
boot log walkthrough and what the SSP pairing dialogue looks like.

### 3. VESC LISP script — for cruise control + speed profiles

[`lisp/main.lisp`](lisp/main.lisp) runs on the **VESC controller** (not on
the P4). It adds cruise control via the PPM `RX` pin and three speed-profile
presets via `TX`, plus exposes the cruise state to the dashboard's LISP
poll channel — that's what drives the cruise indicator on screen.

Open *VESC Tool → VESC Packages → Lisp Scripting*, load `lisp/main.lisp`,
**Upload** → **Activate** → save to flash. See [`lisp/README.md`](lisp/README.md)
for details and how to customise the speed presets.

Without this script the dashboard still shows live telemetry — only the
cruise indicator and profile-switch beeps are lost.

### 4. OTA updates after the first flash

Once the head unit is on its SoftAP (default IP `192.168.4.1`), push new
firmware over HTTP from any laptop joined to the same AP:

```bash
scripts/ota_push.sh 192.168.4.1
```

You'll see a progress bar on the device screen, then it reboots into the new
slot.

---

## 📱 Using it

### Dashboard mode (default)

Power up. The dashboard comes up immediately on the screen showing whatever
the VESC is reporting over CAN — battery, speed, temps, etc. Tap into Settings
to change units, CAN bitrate, controller ID, or open the log viewer.

While the dashboard is running, **VESC Tool can connect over Bluetooth LE**
(NUS bridge) just like to any other VESC adapter.

### Switching between modes

**Three-finger tap** (any three fingers on the screen for ~100 ms) toggles
between the **VESC dashboard** and the **Android Auto projection**. The
gesture works in either direction, in both modes, and only fires once per
gesture — your fingers have to lift before it can fire again, so it won't
leak through to the phone as a stray touch.

### Android Auto bonus (after flashing the BT agent)

1. The screen also shows **"Waiting for phone"** in a corner with the SoftAP
   SSID / IP.
2. On the phone: *Settings → Bluetooth → Pair new device → **ESP32-P4 AA***.
3. Accept the SSP pairing prompt.
4. The phone joins the head unit's WiFi automatically and launches Android
   Auto. The screen flips to AA projection; the VESC dashboard runs on as
   an overlay (battery %, speed) on top of the AA video.

After the first pairing the head unit remembers the phone, and every
subsequent power-on auto-reconnects without prompts.

### Companion app (notifications, media, clock, WiFi flashing)

Optional Android app in [`flutter-application/`](flutter-application/). Build
and install:

```bash
cd flutter-application
flutter build apk --release
adb install -r build/app/outputs/flutter-apk/app-release.apk
```

- **Pair**: open the app → it scans for the head unit over BLE → tap to
  connect. Auto-reconnects after that.
- **Notifications / media**: grant the notification-access permission when
  prompted; phone notifications + now-playing info appear on the dashboard.
  The dashboard clock starts showing `HH:MM` once the app is connected.
- **Update firmware over WiFi**: *Home → Update head unit firmware*. The app
  reads the SoftAP credentials over BLE (or you scan / type them), joins that
  WiFi and uploads the firmware bundled in the APK. The head unit verifies and
  reboots onto the new version — keep it powered throughout. The bundled image
  is staged from `build/` by [`scripts/stage_firmware_asset.sh`](scripts/stage_firmware_asset.sh).

---

## 🗺️ Roadmap / Status

| Area | Status | Notes |
|---|---|---|
| VESC RT data over CAN | ✅ | Battery %, speed, voltage, current, temps, odometer |
| Trip statistics window + persistent trip log | ✅ | Per-trip metrics / charts + Reset trip; raw circular log on the `triplog` partition |
| On-device VESC Tool config menu + FOC detection | ✅ | Read / write controller config from the screen; param tables generated from the VESC Tool XML |
| VESC LISP poll | ✅ | Cruise indicator + custom stats (requires [`lisp/main.lisp`](lisp/main.lisp) on the controller) |
| BLE NUS bridge (VESC Tool over BLE) | ✅ | Works concurrently with AA |
| Companion app — notifications / media / clock bridge | ✅ | Flutter `aa_bridge` over BLE |
| Companion app — firmware update over WiFi | ✅ | Bundled image POSTed to the head unit's SoftAP OTA endpoint |
| Settings UI + PSRAM log viewer | ✅ | Logs survive resets, viewable on device |
| HTTP OTA + on-screen progress | ✅ | `scripts/ota_push.sh` |
| (Bonus) AA Wireless video (H.264) | ✅ | Native 800×480 @ ~10–15 fps; SW decode + RGB shuffle is the bottleneck |
| (Bonus) Touch input forwarding | ✅ | GT911 → AA `TouchEvent` protobuf |
| (Bonus) System audio channel | ✅ | UI beeps; required by Gearhead to project at all |
| (Bonus) Auto-reconnect to last phone | ✅ | Bonded list in NVS on the BT agent |
| Media / Speech audio channels | 🟡 | Dropped on purpose — no audio sink |
| Pure BT Classic on P4 (no D1 Mini) | ❌ | Not possible — ESP32-P4 has no BT Classic radio |

---

## 🖨️ 3D-printable enclosure

STL / STEP files for the case live in [`3d-model/`](3d-model/). Print
settings, recommended materials and assembly notes will be added there as the
design firms up.

<!-- TODO: render or photo of the printed enclosure -->

---

## 📁 Repository layout

```
.
├── main/                       # ESP32-P4 firmware — VESC, UI, AA stack, OTA, BLE
├── components/
│   ├── esp32_p4_wifi6_touch_lcd_4_3/  # Waveshare BSP (LVGL, ST7701 DSI, GT911 touch)
│   ├── vesc_can/               # CAN driver for VESC (RT data + LISP poll)
│   ├── vesc_ui/                # Dashboard UI — thin wrapper; real UI source is globbed from Super_VESC_Display/{generated,custom}/
│   ├── vesc_config/            # On-device VESC Tool config menu (param tables from XML, FOC detection)
│   ├── trip_log/               # Raw circular trip log on the dedicated triplog partition
│   ├── dev_settings/           # Settings screen + persisted prefs
│   ├── log_capture/            # PSRAM ring-buffer logger
│   ├── bt_agent_fw/            # Embeddable bt_agent firmware blob (AA bonus)
│   ├── c6_ota_partition/       # Embedded ESP32-C6 firmware (network_adapter.bin)
│   └── qr_info/                # QR code with WiFi creds for the phone
├── Super_VESC_Display/         # NXP GUI Guider project — the dashboard UI source (generated/ + custom/), compiled into the firmware via vesc_ui
├── tools/
│   ├── bt_agent/               # D1 Mini ESP32 firmware (Classic BT + SPP)
│   ├── c6_slave_fw/            # Sources of the bundled C6 firmware (gitignored, see CLAUDE.md)
│   └── c6_ota_flasher/         # Standalone fallback C6 flasher
├── scripts/                    # capture.sh (Wireshark), ota_push.sh, extract_yuv.py, release.sh
├── lisp/                       # VESC LISP script (cruise + speed profiles) — runs on the VESC, not the P4
├── 3d-model/                   # STL / STEP files for the printed enclosure
├── release/                    # Versioned release artifacts (P4 .bin + companion .apk)
├── docs/images/                # Photos / screenshots used by this README
├── research/                   # Reference upstream sources (gitignored)
├── partitions.csv              # Dual-OTA layout (both slots below 16 MB) + triplog partition
├── CLAUDE.md                   # Original architecture notes / design history
└── README.md
```

---

## 🙏 Credits & references

- The **[VESC project](https://vesc-project.com/)** by *Benjamin Vedder* —
  the open-source motor controller this dashboard is built around.
- [**aasdk**](https://github.com/f1xpl/aasdk) and [**openauto**](https://github.com/f1xpl/openauto) by *f1xpl* — the AA Wireless protocol reference for the bonus mode.
- [**headunit-revived**](https://github.com/andreknieriem/headunit-revived) by *andreknieriem* — wireless-mode reference.
- [**esp-h264**](https://github.com/espressif/esp-h264) and [**esp-hosted**](https://github.com/espressif/esp-hosted) by *Espressif*.
- [**Waveshare ESP32-P4-WIFI6-Touch-LCD-4.3 wiki**](https://github.com/waveshareteam/ESP32-P4-WIFI6-Touch-LCD-4.3) — board BSP and example code.

---

## 📜 License

Released under the **GNU General Public License v3.0** — see [`LICENSE`](LICENSE).
