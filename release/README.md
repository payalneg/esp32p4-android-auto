# Firmware 1.3.41 / Android app 0.3.50

Built with ESP-IDF 5.5.6. The APK contains an OTA image for every profile below.

## What changed since 1.3.40

This is a **documentation and tooling release**. Firmware behaviour is unchanged
from 1.3.40; every board is rebuilt with the new version string.

- **New screen-by-screen guide** — [`docs/ui-guide.md`](../docs/ui-guide.md) and
  [`docs/ui-guide.ru.md`](../docs/ui-guide.ru.md) now document every screen with
  a picture of the firmware's own UI: all four dashboard themes (**Cockpit**,
  **Classic Max**, **Cockpit (Amber)**, **Twin Arc**), the JK-BMS pack screen and
  its Bluetooth pairing, all four Settings tabs, the odometer dialog, trip log
  and chart, VESC Config, PAS, speed sensor, Quick actions, LISP editor,
  Realtime, QR codes and the log viewer.
- **Refreshed screenshots** — every dashboard and Settings picture was
  regenerated from the current simulator, including the two wide themes and the
  connected JK-BMS state. The Twin Arc demo GIF is linked from the guide.
- **Simulator parity** — `menu bms` was added to the headless driver so the
  JK-BMS screen can be rendered and audited like the other menus; the LISP
  editor now also builds on Windows/MinGW hosts.
- **README / changelog** — both READMEs point at 1.3.41 / 0.3.50, the feature
  list covers the themes and the JK-BMS source, and the 1.3.40 release notes are
  recorded in [`CHANGELOG.md`](../CHANGELOG.md).

The 1.3.40 feature set is unchanged and still present:

- Cockpit (Amber) and Twin Arc on the 800×480 head units; Cockpit and Classic
  Max remain the defaults, and the 480×480 S3 build keeps those two.
- The JK-BMS pack as a dashboard battery source (JK02 frames, 24S/32S layouts,
  16S packs on the 32S layout). SOC and remaining Ah feed the dashboard, AA HUD
  and range estimate; an offline pack reads **N/A**.

| Profile in filename / app | Hardware |
| --- | --- |
| `waveshare` | Waveshare P4 4.3, silicon v1.x |
| `waveshare-rev3` | Waveshare P4 4.3, silicon v3.x (including v3.2) |
| `jc4880` | Guition JC4880P443C, P4 v1.x |
| `s3touch4` | Waveshare ESP32-S3-Touch-LCD-4 |

For USB recovery, use `flash.bat` (Windows) or `flash.command` (macOS).
The helper checks the P4 revision before writing and requires stable esptool
4.12+ or 5.3+. Use the USB-UART connector and the default 115200 baud for recovery.

- `*-merged.bin`: complete USB image, written at **0x0**.
- The `.bin` without `-merged`: application image for OTA.
- `aa_bridge-0.3.50.apk`: one app for all profiles. It selects the firmware using
  the device's reported profile and blocks a detected revision/profile mismatch.
- `SHA256SUMS.txt`: checksums of the firmware images and APK.

For a P4 v3.2 board, choose **waveshare-rev3**. Do not use `--force` to install
a v1.x image. See [recovery details](../docs/esp32p4-rev3.md).

The APK uses the same signing certificate as 0.3.42, so it can update that app
without uninstalling it. Built using Flutter 3.29.3 / Dart 3.7.2 and Android
compile SDK 36.

Hardware validation is still required. Build and compatibility checks do not
establish that an individual board, its power supply, or its panel is healthy.