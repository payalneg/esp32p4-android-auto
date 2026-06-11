---
name: capture-logs
description: >
  Capture serial / debug logs from the ESP32-P4 head unit, the external
  BT-agent module, and/or the Android phone (logcat) — to debug a crash, boot
  loop, Android Auto handshake, Wi-Fi/TLS issue, or runtime behaviour. Use when
  asked to capture logs, get serial output, monitor the device, grab a boot log,
  read logcat, decode a dumped video frame, or push an OTA build.
---

# Capturing logs from the head unit

Helper scripts in `scripts/` write timestamped files to `logs/<ts>.log`. They
run pyserial from the ESP-IDF python env (auto-located), so no extra installs.
All take an optional duration (seconds) and `--flash` to flash first.

## P4 serial — the usual one

```bash
scripts/capture.sh                 # runs long (default 6000 s); Ctrl-C stops & keeps partial
scripts/capture.sh 120             # fixed 120 s window
scripts/capture.sh 60 --flash      # flash (idf.py -p PORT -b 460800 flash) then capture
PORT=/dev/cu.xxxx scripts/capture.sh
```

- **Resets the chip via DTR/RTS at start** so boot logs are always caught. This
  is the *opposite* of the device-screen bridge (which connects no-reset to keep
  the UI alive). If you must NOT reboot the device, don't use capture.sh — use
  the device-screen skill instead.
- Defaults: `PORT=/dev/cu.usbmodem5B5E0700881`, `BAUD=115200` (override via env).

## BT-agent module (external ESP32, e.g. D1 Mini / WROOM)

```bash
scripts/capture_bt.sh [s] [--flash]      # → logs/bt-<ts>.log
BT_PORT=/dev/cu.xxxx scripts/capture_bt.sh
```
`--flash` rebuilds + flashes `tools/bt_agent`. Default `BT_PORT=/dev/cu.usbserial-02BOJDQ2`.

## Android phone (logcat)

```bash
scripts/capture_phone.sh             # save all, show AA-tagged lines live
scripts/capture_phone.sh --filter    # save only AA-tagged (smaller)
scripts/capture_phone.sh --all-live  # save + print everything
```
Needs `adb` and an authorised device. Filters on AA tags
(`aap|gearhead|projection|androidauto|wireless.?helper|…`) → `logs/phone-<ts>.log`.

## Combined (shared timestamp so streams line up)

```bash
scripts/capture_both.sh [s] [--flash]   # P4 + phone
scripts/capture_all.sh  [s] [--flash]   # P4 + BT-agent + phone
```

## After capture

- Read / grep `logs/<ts>.log` (ANSI colour kept, non-printables stripped).
- **Decode a dumped video frame**: the firmware base64-dumps the 10th decoded
  I420 frame between `=== I420_DUMP_BEGIN … ===` / `=== I420_DUMP_END ===`
  (see `main/h264_pipe.c`). Extract it:
  `python3 scripts/extract_yuv.py logs/<ts>.log -o frame.yuv` (prints the ffplay
  line).

## Flashing / OTA (when you also need to update firmware)

```bash
scripts/build_board.sh waveshare -p <PORT> flash      # USB, per board
idf.py -p <PORT> flash                                # USB, default board
scripts/ota_push.sh [host]                            # Wi-Fi OTA → 192.168.4.1:80 /ota
BIN=build_jc4880/esp32p4_android_auto.bin scripts/ota_push.sh   # per-board OTA bin
```
(BLE OTA via the companion app also exists — see the head-unit skill.)

## Ports cheat-sheet

```bash
ls /dev/cu.usb*            # discover ports
# P4 serial   : $PORT      (default /dev/cu.usbmodem5B5E0700881)
# BT agent    : $BT_PORT   (default /dev/cu.usbserial-02BOJDQ2)
```
Only one process can own a port at a time — don't run `capture.sh` and
`uart_debug.py` (device-screen) on the same port simultaneously.

## See also

- **device-screen** — screenshot + touch injection (no-reset connect).
- **head-unit** — project overview and codebase map.
