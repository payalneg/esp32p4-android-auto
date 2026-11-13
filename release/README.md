# Firmware 1.3.40 / Android app 0.3.49

Built with ESP-IDF 5.5.6. The APK contains an OTA image for every profile below.

This build adds two extra dashboards on the 800x480 head units: **Cockpit
(Amber)**, the warm single-screen cockpit with DSEG7 numerals, and **Twin Arc**,
two 270-degree rings framing speed and power with battery/range between them,
a wide now-playing card under the rings and a cruise-control badge in the status
bar. Twin Arc uses a smaller 120 px speed face and sits the KM/H label clear of
the digits. Both are picked in Settings; the default Cockpit and Classic Max
dashboards are unchanged.

The JK-BMS pack remains available as a dashboard battery source (JK02 frames,
24S and 32S layouts, 16S packs on the 32S layout). Pack SOC and remaining Ah
feed the dashboard, AA HUD and range estimate; while the selected pack is
offline the percentage reads **N/A** instead of an unrelated controller value.
The dashboard's Ah field always shows **remaining capacity** — the pack's own
counter in BMS mode, the Smart tracker's remaining Ah, or the controller's
percentage applied to the configured capacity.

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
- `aa_bridge-0.3.49.apk`: one app for all profiles. It selects the firmware using
  the device's reported profile and blocks a detected revision/profile mismatch.
- `SHA256SUMS.txt`: checksums of the firmware images and APK.

For a P4 v3.2 board, choose **waveshare-rev3**. Do not use `--force` to install
a v1.x image. See [recovery details](../docs/esp32p4-rev3.md).

The APK uses the same signing certificate as 0.3.42, so it can update that app
without uninstalling it. Built using Flutter 3.29.3 / Dart 3.7.2 and Android
compile SDK 36.

Hardware validation is still required. Build and compatibility checks do not
establish that an individual board, its power supply, or its panel is healthy.
