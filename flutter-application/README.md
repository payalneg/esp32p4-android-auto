# aa_bridge

Companion Android app for the [ESP32-P4 VESC / Android Auto head unit](../README.md).
Talks to the head unit over Bluetooth Low Energy (the NotifBridge GATT
service), independently of the Android Auto link.

## What it does

- **Notification bridge** — mirrors phone notifications (title, text, app
  icon) onto the dashboard.
- **Media bridge** — now-playing title / artist / album + album art, with
  play / pause / next / prev controls echoed back from the dashboard.
- **Wall clock** — pushes local `HH:MM` over BLE every 15 s so the dashboard
  shows the time without an on-device RTC / coin cell. The head unit hides the
  clock again if updates stop for 30 s.
- **Bike battery** — reads the pack charge (SOC, pack voltage and remaining Ah)
  from the head unit over the NotifBridge battery characteristic, and caches
  the last reading on the phone. The app bar shows it even after the bike is
  switched off, with how long ago it was read.
- **Firmware update over WiFi** — bundles the P4 firmware image in the APK and
  flashes it to the head unit over its SoftAP:
  - reads the SoftAP SSID / password + running version over BLE
    (characteristic `…0006`), or lets you scan / type them manually,
  - joins that WiFi (Android `WifiNetworkSpecifier` + `bindProcessToNetwork`),
  - POSTs the image to `http://192.168.4.1/ota`; the head unit verifies and
    reboots onto the new version.
- **Firmware update over USB** — with a USB-C OTG adapter the phone is the USB
  host and writes the chip's flash directly (ESP ROM loader protocol over the
  native USB-Serial/JTAG port or the CH343 UART bridge). Works when the firmware
  no longer boots. Choose the image bundled in the APK (written as a full
  recovery image: bootloader + partition table + app) or any `.bin`: merged
  images go to 0x0, application-only images to an editable offset (0x20000 by
  default on this project's P4 boards). The chip and the P4 silicon revision
  are checked before erasing and the flash is MD5-verified. `usb_serial` is
  vendored in `packages/usb_serial` with CH343 support. See
  [`../docs/usb-flashing.md`](../docs/usb-flashing.md) for the step-by-step
  guide and troubleshooting.
- **Update prompt on the head unit** — when the firmware bundled in the APK is
  newer than the version the board reports, the head unit puts an
  Update / Later / Skip dialog over its dashboard (`…000E`). Update flashes the
  image over the same BLE link and the dialog shows progress; Skip remembers
  that version per board and stops offering it.
- **About** screen — app + bundled-firmware versions.

In **Map data → Download a region**, select **Małopolskie** to build the
styled offline map for Kraków. Download the region again if you have data
from an older build. The phone renders the water outlines, buildings, bridges,
cycleways and street labels from OpenStreetMap and sends the same 256 px PNG
tiles to the head unit over BLE.

## Build & run

Use Flutter 3.29.3 / Dart 3.7.2 with the checked-in lock file. Android builds
need compile SDK 36 and NDK 27.0.12077973. Firmware 1.3.42 bundles the
Waveshare P4 v1.x / v3.x, Guition JC4880 and ESP32-S3 images; build all
profiles before staging assets.

```bash
flutter pub get

# Stage the firmware image the OTA feature ships (run from the repo root after
# building the P4 firmware with idf.py build):
../scripts/stage_firmware_asset.sh

flutter build apk --release
adb install -r build/app/outputs/flutter-apk/app-release.apk
```

The OTA assets (`assets/firmware/`) are git-ignored — they're generated from
the freshly built `build/esp32p4_android_auto.bin` by `stage_firmware_asset.sh`
(together with `…-head.bin`, the first 0x20000 bytes of each board's merged
image, which the USB flasher combines with the application at flash time).
`dart run tool/check_usb_image.dart` prints the resulting USB image kind,
offset, chip and silicon-revision range.

## Platform notes

- **Android only** for the WiFi-flash + scan features (uses native
  `WifiManager` / `ConnectivityManager`). The BLE bridge works cross-platform,
  but the app is wired up and tested on Android.
- **USB flashing** needs an Android phone with USB host (USB-C OTG adapter).
  Android asks for permission to access the serial device the first time; no
  storage permission is involved. The head unit draws more than the phone's
  port usually supplies, so power the board properly while flashing.
- WiFi scan + "current network" read need **location enabled** + the location
  permission (Android requirement). Joining the SoftAP shows the system's own
  approval dialog.

## Layout

| Path | What |
|---|---|
| `lib/ble/` | BLE service, NotifBridge UUIDs, chunked PDU codec |
| `lib/firmware/` | `OtaInfo`, `FirmwareUpdater` (join → upload → reboot), `esptool/` + `usb_flasher.dart` (ROM loader over USB) |
| `lib/bridge/` | platform-channel wrappers (foreground service, WiFi) |
| `lib/ui/` | pairing, home, app filter, firmware update, about screens |
| `android/.../WifiBridge.kt` | native WiFi join / scan / current-SSID |
