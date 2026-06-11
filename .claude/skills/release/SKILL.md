---
name: release
description: >
  Cut a release of the ESP32-P4 head-unit firmware + Flutter companion app —
  bump versions, build every board, bundle firmwares into the APK, and stage
  versioned artifacts. Use when asked to make a release, bump the version, build
  the APK, produce per-board release binaries, or understand how firmware/app/
  BT-agent versions stay in lockstep.
---

# Cutting a release

`scripts/release.sh` is the one-shot release helper. It bumps versions, builds
the P4 firmware for **every** board, bundles them into the Flutter app, builds
the APK, and stages versioned artifacts.

```bash
. "$IDF_PATH/export.sh"
scripts/release.sh                 # patch-bump P4 fw + app
scripts/release.sh 1.3.0 0.2.0     # explicit  <fw_version> <app_version>
```

What it does, in order:

1. **Bump versions** — `version.txt` (P4 firmware) and
   `flutter-application/pubspec.yaml` (app `version: X.Y.Z+build`).
2. **Build each board** — `scripts/build_board.sh <board> reconfigure build`
   (the `reconfigure` is essential so CMake picks up `PROJECT_VER` from the new
   `version.txt`). Boards: `waveshare`, `jc4880`.
3. **Stage firmwares into the app** — `scripts/stage_firmware_asset.sh` copies
   `build_<board>/esp32p4_android_auto.bin` →
   `flutter-application/assets/firmware/esp32p4_android_auto-<board>.bin` plus
   `version.txt`. The app picks the matching image at OTA time from the board
   model the head unit reports over BLE / `GET /info`
   (`lib/firmware/firmware_updater.dart`).
4. **Build the APK** — `flutter build apk --release` (the APK bundles all
   per-board firmwares for over-Wi-Fi OTA).
5. **Stage release artifacts** — `release/esp32p4_android_auto-<board>-<ver>.bin`
   per board + the APK.

## It does NOT commit

After it finishes, review and commit yourself: `version.txt`,
`flutter-application/pubspec.yaml`, `release/*`, the bundled
`flutter-application/assets/firmware/*`, and your code changes.

## Three independent version trains

- **P4 firmware** — `version.txt` (→ `PROJECT_VER`, needs `reconfigure`).
- **Companion app** — `flutter-application/pubspec.yaml`.
- **BT-agent firmware** — versioned **separately**, NOT touched by `release.sh`.
  Bump it only when `tools/bt_agent/` changes, via `tools/pack_fw_blobs.sh`
  (3-way lockstep between agent source, the embedded blob in
  `components/bt_agent_fw`, and the host firmware that flashes it). The C6
  Wi-Fi blob is shared across all boards.

## Before a release — sanity

- Build sizes fit, especially the **jc4880 16 MB** board (~5 MB OTA slot, ~1 MB
  headroom): `scripts/build_board.sh jc4880 size`. Keep debug-only options
  (e.g. `CONFIG_DEBUG_UART_BRIDGE`) **off** in release builds.
- The C6 firmware blob is current (`components/c6_ota_partition/`) — bump only
  when the C6 slave fw changed (see `CLAUDE.md`).

## See also

- **build-flash** — per-board builds, flashing, OTA, flash budget.
- **head-unit** — project overview and codebase map.
