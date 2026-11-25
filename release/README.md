# Firmware 1.3.55 / Android app 0.3.68

ESP-IDF 5.5.6; Flutter 3.29.3 / Dart 3.7.2; Android build 132, compile SDK 36.
The APK includes the three P4 OTA images below. S3 remains paused and is not
included in this release or the app bundle.

## Changes

Firmware-only fix; the app is the same code as 0.3.67, rebuilt with a version
bump.

- **VESC Tool through the display works again** — the link connected, the phone
  received one dataset, and then neither reads nor writes worked while the
  dashboard kept showing good data. Two causes, both in this device:
  - the dashboard's own polls (10 Hz setup-values, 5 Hz dashboard stats, the
    ADC/PPM viewer) share one request/reply rail with the phone's requests, so
    VESC Tool's requests could queue behind them and time out;
  - the bridge sent each phone request from the GATT write callback on the
    NimBLE host task and waited up to 60 ms for the answer there, stalling the
    whole BLE stack once per request.
  Every phone request now marks the bridge active and the dashboard's polls are
  capped at 1 Hz for two seconds after the last one; its fast fields come from
  the phone's answers, which already pass through the shared fan-out
  (`COMM_GET_VALUES_SELECTIVE` and `COMM_GET_VALUES` are now parsed too).
  Requests leave from a dedicated `nus_bridge` task; the host task only copies
  the payload, and the queue is drained on disconnect and on an NUS ownership
  switch so a stale `SET_*` cannot be applied later.
- **The reply tag matches the echoed mask**, not only the command byte. The
  display's own SELECTIVE polls — and on a BLE link the second head's
  temperature-only poll — carry the same command byte as VESC Tool's realtime
  request; one of them could claim the tag, so the phone got a partial dataset
  and its own answer was dropped.
- `scripts/build_board.bat` passes the dependency-lock path to CMake with
  forward slashes; a backslashed `C:\Users\...` aborted the configure with
  `Invalid character escape '\U'`.

## Validation

All three P4 images were rebuilt in this release; the version string embedded
in each image is 1.3.55. The APK was built from the same app sources as 0.3.67
(build number 132) and carries the three images. The merged USB images were
generated from each board's `flasher_args.json`. `release/SHA256SUMS.txt` was
regenerated from the seven published artifacts.

The bridge fix is not yet validated against VESC Tool on hardware: it is built
for all three boards and the behaviour is exercised only by the code paths
described above. Please report whether reads, the realtime page and a
configuration write all work through the display.

## Files and hardware

| Filename profile | Hardware |
| --- | --- |
| `waveshare` | Waveshare P4 4.3, silicon v1.x |
| `waveshare-rev3` | Waveshare P4 4.3, silicon v3.x / v3.2 |
| `jc4880` | Guition JC4880P443C, P4 v1.x |

- `*-1.3.55.bin`: application image for OTA.
- `*-1.3.55-merged.bin`: complete USB image, written at **0x0**.
- `aa_bridge-0.3.68.apk`: normal companion app, with no automatic test drive
  and no embedded LLM API key. Firmware is selected by reported board profile.
- `SHA256SUMS.txt`: SHA-256 checksums of the seven binary artifacts.

For USB recovery use `flash.bat` (Windows) or `flash.command` (macOS), with
`flash.py` alongside the images. Stable esptool 4.12+ or 5.3+ is required.
Choose **waveshare-rev3** for P4 v3.2 and do not force a v1.x image onto it.
See [v3.2 recovery instructions](https://github.com/payalneg/esp32p4-android-auto/blob/v1.3.55/docs/esp32p4-rev3.md). Use the application
image for ordinary OTA updates; the merged image is the full USB recovery set.

APK signing certificate SHA-256:
`9ce204fe0b7f092f9732744031f231a622f098389c0ee35c2e460d3129204845`.
Android requires the same certificate for an in-place update. The older APK on
the bench Samsung used a different debug certificate; its app and data were
preserved by installing a separate test copy. No signing keys are distributed.
