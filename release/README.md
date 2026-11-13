# Firmware 1.3.42 / Android app 0.3.51

Built with ESP-IDF 5.5.6. The APK contains an OTA image for every profile below.

## What changed since 1.3.41

Bug-fix release; the 1.3.41 feature set is unchanged.

- **Fixed: the Twin Arc white screen** — the dashboard could freeze on a white
  screen right after the boot splash, most reliably on **Twin Arc**. The music
  info poller hides its surrounding `music_info` container while no track is
  playing (a trick for the GUI-Guider themes); Twin Arc parents its music tile
  directly on the screen, so the poller hid the **whole dashboard screen** —
  and LVGL drops every invalidation on a hidden object, so nothing could
  repaint. The toggle is now skipped when the tile's parent is the screen.
  Verified on a Waveshare 4.3" board: Twin Arc renders steadily (~10 fps)
  where 1.3.41 froze at 0 fps.
- **Android Auto auto-connect is now off by default** — the "Reconnect to last
  phone on power-on" setting (Settings → Connection) starts disabled on a
  fresh or erased device. A previously saved choice is kept. The manual
  **Connect** button always works.

Carried from 1.3.40/1.3.41:

- Cockpit (Amber) and Twin Arc on the 800×480 head units; Cockpit and Classic
  Max remain the defaults, and the 480×480 S3 build keeps those two.
- The JK-BMS pack as a dashboard battery source (JK02 frames, 24S/32S layouts,
  16S packs on the 32S layout). SOC and remaining Ah feed the dashboard, AA HUD
  and range estimate; an offline pack reads **N/A**.
- The screen-by-screen guide: [`docs/ui-guide.md`](https://github.com/payalneg/esp32p4-android-auto/blob/main/docs/ui-guide.md) /
  [`docs/ui-guide.ru.md`](https://github.com/payalneg/esp32p4-android-auto/blob/main/docs/ui-guide.ru.md).

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
- `aa_bridge-0.3.51.apk`: one app for all profiles. It selects the firmware using
  the device's reported profile and blocks a detected revision/profile mismatch.
- `SHA256SUMS.txt`: checksums of the firmware images and APK.

For a P4 v3.2 board, choose **waveshare-rev3**. Do not use `--force` to install
a v1.x image. See [recovery details](https://github.com/payalneg/esp32p4-android-auto/blob/main/docs/esp32p4-rev3.md).

The APK uses the same signing certificate as 0.3.42, so it can update that app
without uninstalling it. Built using Flutter 3.29.3 / Dart 3.7.2 and Android
compile SDK 36.

The white-screen fix itself was validated on real Waveshare hardware. The rest
of the release is build- and OTA-verified only; the 1.3.33 hardware caveats
still apply — flash with care and keep a USB recovery path available.
