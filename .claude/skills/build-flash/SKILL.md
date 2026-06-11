---
name: build-flash
description: >
  Build, flash, and release the ESP32-P4 head-unit firmware across its boards
  (Waveshare 4.3" and Guition JC4880). Use when asked to build, compile, flash
  (USB or OTA), select a board, bump the version, cut a release, or enable a
  build-time option (e.g. the UART debug bridge). Covers build_board.sh, idf.py,
  Wi-Fi/BLE OTA, the embedded C6 firmware, and release.sh.
---

# Building & flashing the head-unit firmware

Multi-board project: a plain `idf.py build` targets **Waveshare** (default, for
back-compat); other boards use `scripts/build_board.sh`, which builds into a
separate `build_<board>/` with the per-board `sdkconfig.defaults.<board>` overlay
layered on the common `sdkconfig.defaults`.

```bash
. "$IDF_PATH/export.sh"     # always source IDF first (scripts try to, but do it)

idf.py build                                   # Waveshare (default dir: build/)
scripts/build_board.sh waveshare build         # → build_waveshare/
scripts/build_board.sh jc4880   build          # → build_jc4880/  (16 MB board)
scripts/build_board.sh                          # build EVERY board
scripts/build_board.sh waveshare menuconfig    # change Kconfig for one board
scripts/build_board.sh waveshare size          # partition / size report
```

Boards live in the `BOARDS=(waveshare jc4880)` list in `build_board.sh`; board
selection is Kconfig `CHOICE BOARD_MODEL` (read by the BSP + `bt_link.h`).

## Flash

```bash
# USB (per board — picks the right build_<board>/ image)
scripts/build_board.sh waveshare -p <PORT> flash monitor
scripts/build_board.sh jc4880    -p <PORT> flash
idf.py -p <PORT> flash                          # default board

# Wi-Fi OTA (laptop joined the head unit's SoftAP)
scripts/ota_push.sh                             # build/esp32p4_android_auto.bin → 192.168.4.1:80 /ota
BIN=build_jc4880/esp32p4_android_auto.bin scripts/ota_push.sh

# BLE OTA: via the Flutter companion app (PSRAM staging + worker-task flash);
# the very first flash must be USB or Wi-Fi (chicken-and-egg).
```
Find ports: `ls /dev/cu.usb*`.

## The on-board C6 (Wi-Fi) firmware

`network_adapter.bin` for the ESP32-C6 is embedded into the P4 binary
(`components/c6_ota_partition/`). At boot `main/c6_ota.c` compares versions and,
on mismatch, OTAs the C6 over SDIO then reboots. You normally don't flash the C6
separately — rebuilding the P4 with an updated blob is enough. (Rebuilding the
blob itself: see `CLAUDE.md` → "Воспроизведение игнорируемых артефактов".)

## Enabling build-time options

Project options are under `menuconfig` → **"Android Auto Head Unit"** (see
`main/Kconfig.projbuild`): board model, Wi-Fi role (SoftAP vs STA), and the
**UART debug bridge** (`CONFIG_DEBUG_UART_BRIDGE`, default off — turn on for the
device-screen skill; keep off for jc4880's tight OTA slot). For a throwaway
build, add a one-line overlay file (`CONFIG_FOO=y`) as the last entry of
`-D SDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.defaults.<board>;<overlay>"`
into a scratch `-B build_dir` so you don't mutate the real board sdkconfig.

## Flash budget watch (jc4880 = 16 MB)

The 16 MB board has **5 MB OTA slots** (`partitions_16mb.csv`) with only ~1 MB
headroom over the ~3.8 MB image. Watch size on jc4880 when adding fonts/features
(`scripts/build_board.sh jc4880 size`); keep optional/debug features off there.

## Release

Cutting a versioned release (bump versions → build all boards → bundle into the
APK → stage artifacts) is its own workflow: `scripts/release.sh`. See the
**release** skill for the full step-by-step and the three version trains (P4
fw / app / BT-agent).

## See also

- **release** — version bump + multi-board build + APK + artifacts.
- **head-unit** — architecture & codebase map.
- **capture-logs** — flashing also available via the capture scripts' `--flash`.
- **device-screen** — needs `CONFIG_DEBUG_UART_BRIDGE=y`.
