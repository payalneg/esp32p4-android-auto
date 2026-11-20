# Firmware 1.3.52 / Android app 0.3.63

ESP-IDF 5.5.6; Flutter 3.29.3 / Dart 3.7.2; Android build 127, compile SDK 36.
The APK includes the three P4 OTA images below. S3 remains paused and is not
included in this release or the app bundle.

## Changes

- Smoother navigation prediction: repeated GPS anchors no longer reset the
  advancing target; fractional steps and Mercator precision are retained.
- One background GPS producer per BLE channel. Repeated connection events no
  longer leave an orphan feed competing with the foreground navigator, and UI
  ownership / stream settings survive connection setup and reconnection.
- Simulated arrival publishes zero speed before cancelling its source.
- Stop repeatedly aligning an unchanged guide row every 25 ms in LVGL.
- Current position messages retain priority during map tile transfers.
- Boot-health/watchdog recovery and an Android recovery screen for supported
  recovery-capable firmware; compatibility with installed bootloaders retained.
- P4 performance work: -O2 builds, fewer AA video copies, map scratch reuse,
  fewer redundant HUD/label updates and faster trip-log scanning.
- Route colours, short English navigation activity captions and separated
  navigation controls are retained from the preceding work.

## Validation

A Samsung Galaxy A23 5G supplied simulated GeoFix events through the normal
app screen, service IPC and real Bluetooth to a JC4880. At 5 m/s, 1 Hz GPS and
zoom 17, matching instrumented intervals improved from **4.68 to 6.68 new map
frames/s**. Backward camera steps fell from **11/180 to 0/182**. Simulated
arrival and physical head restart/BLE reconnection passed without a competing
GPS source. **62 targeted Flutter tests** passed and all three P4 images built.

See the [phone test report](../docs/navigation-phone-test-report.md) for the
fixture, thresholds, figure, source exclusivity checks and rejected attempts.
Map-frame counters are not measured LCD scanout FPS. The final hardware test
uses a 120-second route prefix and a separate arrival injection; it is not a
complete vehicle or overnight endurance test. Other board hardware, background
simulation handoff and forced panic/hang recovery still require validation.
Rendering remains limited to single-digit new map-frame rates in this bench
case. Repeated VIEW messages can still mask stale GPS in the activity caption,
although motion prediction is bounded to 1.5 seconds without a distinct fix.

## Files and hardware

| Filename profile | Hardware |
| --- | --- |
| `waveshare` | Waveshare P4 4.3, silicon v1.x |
| `waveshare-rev3` | Waveshare P4 4.3, silicon v3.x / v3.2 |
| `jc4880` | Guition JC4880P443C, P4 v1.x |

- `*-1.3.52.bin`: application image for OTA.
- `*-1.3.52-merged.bin`: complete USB image, written at **0x0**.
- `aa_bridge-0.3.63.apk`: normal companion app, with no automatic test drive
  and no embedded LLM API key. Firmware is selected by reported board profile.
- `SHA256SUMS.txt`: SHA-256 checksums of the seven binary artifacts.

For USB recovery use `flash.bat` (Windows) or `flash.command` (macOS), with
`flash.py` alongside the images. Stable esptool 4.12+ or 5.3+ is required.
Choose **waveshare-rev3** for P4 v3.2 and do not force a v1.x image onto it.
See [v3.2 recovery instructions](../docs/esp32p4-rev3.md). Use the application
image for ordinary OTA updates; the merged image is the full USB recovery set.

APK signing certificate SHA-256:
`9ce204fe0b7f092f9732744031f231a622f098389c0ee35c2e460d3129204845`.
Android requires the same certificate for an in-place update. The older APK on
the bench Samsung used a different debug certificate; its app and data were
preserved by installing a separate test copy. No signing keys are distributed.
