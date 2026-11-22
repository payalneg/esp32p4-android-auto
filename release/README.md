# Firmware 1.3.53 / Android app 0.3.64

ESP-IDF 5.5.6; Flutter 3.29.3 / Dart 3.7.2; Android build 128, compile SDK 36.
The APK includes the three P4 OTA images below. S3 remains paused and is not
included in this release or the app bundle.

## Changes

- AA video: only the HUD strip of the panel framebuffer is invalidated before
  the overlay is blended, instead of the whole 768 KB frame (12k cache lines)
  per video frame. One full invalidate per AA session still clears lines left
  behind by LVGL.
- AA HUD: the 1-px outline is composited per destination pixel and drawn in one
  pass instead of four glyph stamps, cutting the overlay from five passes to
  two and the PSRAM read-modify-writes per halo pixel from four to one.
- Settings: NVS commits run on a low-priority writer task instead of the LVGL
  thread. Toggling an option no longer stalls the UI for ~100 ms; bursts of
  changes coalesce into one commit.
- LISP web editor: `GET /lisp/api/code` copies the source under the lock and
  sends it after releasing it, so a large response no longer blocks the CAN
  transfer worker.
- BMS cell tab: the whole cell tile is hidden/shown (not just its label) and
  each cell now prints its voltage with a `V` suffix.
- `lisp/main.adc-pas.lisp` v8: continuous virtual ADC1 throttle for PAS and a
  local-speed PID cruise, with brake/throttle priority and startup interlocks.
  See `lisp/README.md` for the required ADC configuration.
- All P4 images in this release are built with `-O2`
  (`CONFIG_COMPILER_OPTIMIZATION_PERF`), including JC4880.

## Validation

All three P4 images were rebuilt from a clean sdkconfig in this release. The
APK was verified to carry exactly those images and firmware version 1.3.53, its
signing certificate matches the previous releases
(`9ce204fe…204845`), and the merged USB images were checked against each
board's `flasher_args.json`. `release/SHA256SUMS.txt` was regenerated from the
seven published artifacts. JC4880 uses 4 733 088 of its 5 MiB OTA slot
(0x7c760 free, 10 %).

No hardware was flashed for this build. The on-device checklist for the AA HUD,
settings writer and LISP editor is in
`docs/optimization-batch-test-plan.md`; AA output and the settings commit path
still need a real-device run.

## Files and hardware

| Filename profile | Hardware |
| --- | --- |
| `waveshare` | Waveshare P4 4.3, silicon v1.x |
| `waveshare-rev3` | Waveshare P4 4.3, silicon v3.x / v3.2 |
| `jc4880` | Guition JC4880P443C, P4 v1.x |

- `*-1.3.53.bin`: application image for OTA.
- `*-1.3.53-merged.bin`: complete USB image, written at **0x0**.
- `aa_bridge-0.3.64.apk`: normal companion app, with no automatic test drive
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
