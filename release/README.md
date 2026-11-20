# Firmware 1.3.48 / Android app 0.3.57

Built with ESP-IDF 5.5.6. The APK contains an OTA image for every profile below.

## What changed since 1.3.47 / 0.3.56

- **Smoother navigation arrow and position correction** — the arrow eases
  through course changes, including north, and the predicted position advances
  between phone updates instead of being pulled back towards a stationary fix.
- **Route colours by road type** — blue roads, green cycleways, orange
  pedestrian ways and purple other paths/tracks. The phone and head unit use
  the same geometry boundaries; older firmware receives the original route
  format through capability negotiation.
- **Short navigator activity messages** — Bluetooth map/route reception, SD
  reads, missing tiles and phone/app/GPS waits fit in one line.
- **Navigation controls no longer overlap** — FIND, AUTO/NORTH and zoom share
  a vertical column; the camera mode is inside its button.
- **VESC Tool bridge response filtering** — the bridge forwards answers to
  the phone's own requests instead of treating the display's partial telemetry
  polls as VESC Tool datasets. CAN phone requests share the poll request rail.

- **Windows release setup** — the batch script now initializes a partial
  ESP-IDF environment before configuring Wi-Fi Remote and reuses ccache.

## Validation and remaining limitations

- Navigation/BLE: 80 existing tests and 20 virtual trips on the real Kraków
  routing graph passed. The release test run passed serially with a two-minute
  timeout per test; a parallel run during full compilation hit timing timeouts.
  The longer model soak also recorded timing-sensitive failures, so it is not
  presented as a clean endurance run.
- JC4880: firmware boot, actual Bluetooth route/tile/position reception and
  the separate AUTO/NORTH controls were checked on the device.
- Rotated-map composition measured 55–70 ms under diagnostic load. The live
  overnight replay later stopped after three Bluetooth timeouts; this release
  does **not** claim a successful overnight hardware endurance run.
- Simulated arrival can retain a nonzero last speed. Repeated VIEW messages
  can hide missing GPS updates from the head unit timeout. Both remain open.
- Other boards and the CAN/NUS changes require hardware verification. Earlier
  S3 CAN limitations still apply.

## Features retained from 1.3.47 / 0.3.56

- **Offline vector maps on the head unit** — the navigator can now draw Kraków
  from a compact NVT1 package on the microSD card instead of forwarding every
  map PNG over BLE. The package carries roads, water, parks, buildings and
  place names; the head unit rasterizes it directly into the existing RGB565
  tile cache and keeps street names on a separate upright layer.
- **Heading-up or north-up from the panel** — the camera button toggles between `AUTO`
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
- `aa_bridge-0.3.57.apk`: one app for all profiles. It selects the firmware using
  the device's reported profile and blocks a detected revision/profile mismatch.
- `SHA256SUMS.txt`: checksums of the firmware images and APK.

For a P4 v3.2 board, choose **waveshare-rev3**. Do not use `--force` to install
a v1.x image. See [recovery details](https://github.com/payalneg/esp32p4-android-auto/blob/main/docs/esp32p4-rev3.md).

The APK uses the same signing certificate as 0.3.56, so it can update that app
without uninstalling it. Built using Flutter 3.29.3 / Dart 3.7.2 and Android
compile SDK 36.

The earlier white-screen fix was validated on Waveshare hardware. The JC4880
checks above cover navigation and layout, not the complete vehicle system;
the earlier hardware caveats still apply.
