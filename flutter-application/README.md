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
- **Firmware update over WiFi** — bundles the P4 firmware image in the APK and
  flashes it to the head unit over its SoftAP:
  - reads the SoftAP SSID / password + running version over BLE
    (characteristic `…0006`), or lets you scan / type them manually,
  - joins that WiFi (Android `WifiNetworkSpecifier` + `bindProcessToNetwork`),
  - POSTs the image to `http://192.168.4.1/ota`; the head unit verifies and
    reboots onto the new version.
- **About** screen — app + bundled-firmware versions.

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
the freshly built `build/esp32p4_android_auto.bin` by `stage_firmware_asset.sh`.

## Platform notes

- **Android only** for the WiFi-flash + scan features (uses native
  `WifiManager` / `ConnectivityManager`). The BLE bridge works cross-platform,
  but the app is wired up and tested on Android.
- WiFi scan + "current network" read need **location enabled** + the location
  permission (Android requirement). Joining the SoftAP shows the system's own
  approval dialog.

## Layout

| Path | What |
|---|---|
| `lib/ble/` | BLE service, NotifBridge UUIDs, chunked PDU codec |
| `lib/firmware/` | `OtaInfo`, `FirmwareUpdater` (join → upload → reboot) |
| `lib/bridge/` | platform-channel wrappers (foreground service, WiFi) |
| `lib/ui/` | pairing, home, app filter, firmware update, about screens |
| `android/.../WifiBridge.kt` | native WiFi join / scan / current-SSID |
