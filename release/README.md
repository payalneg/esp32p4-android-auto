# Firmware 1.3.47 / Android app 0.3.56

Built with ESP-IDF 5.5.6. The APK contains an OTA image for every profile below.

## What changed since 1.3.46 / 0.3.55

- **Offline vector maps on the head unit** — the navigator can now draw Kraków
  from a compact NVT1 package on the microSD card instead of forwarding every
  map PNG over BLE. The package carries roads, water, parks, buildings and
  place names; the head unit rasterizes it directly into the existing RGB565
  tile cache and keeps street names on a separate upright layer.
- **Heading-up or north-up from the panel** — `ROT` now toggles between `AUTO`
  (the map turns with the rider's course) and `NORTH` (the original north-up
  map). The old 30-degree bench steps are gone.
- **No more zoom badge** — the `z16` / `z17` plate over the map was removed;
  the `+` / `−` buttons still change the level.
- **Kraków offline layer in the companion app** — when the Małopolskie region
  is built, the app also stores the local vector geometry and serves it to the
  navigator without a tile download.
- **Twin Arc redrawn** — mirror-image rings, Antonio hero readouts, a correct
  `MAX` scale, a ride-time cell and the cruise badge moved above the speed ring.

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
- `aa_bridge-0.3.56.apk`: one app for all profiles. It selects the firmware using
  the device's reported profile and blocks a detected revision/profile mismatch.
- `SHA256SUMS.txt`: checksums of the firmware images and APK.

For a P4 v3.2 board, choose **waveshare-rev3**. Do not use `--force` to install
a v1.x image. See [recovery details](https://github.com/payalneg/esp32p4-android-auto/blob/main/docs/esp32p4-rev3.md).

The APK uses the same signing certificate as 0.3.55, so it can update that app
without uninstalling it. Built using Flutter 3.29.3 / Dart 3.7.2 and Android
compile SDK 36.

The white-screen fix itself was validated on real Waveshare hardware. The rest
of the release is build- and OTA-verified only; the 1.3.33 hardware caveats
still apply — flash with care and keep a USB recovery path available.
