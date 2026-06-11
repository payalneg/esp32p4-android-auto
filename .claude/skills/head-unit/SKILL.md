---
name: head-unit
description: >
  Orientation and architecture map for THIS repo — a wireless Android Auto head
  unit + VESC dashboard on ESP32-P4 with an 800×480 touch LCD. Use when starting
  work here, understanding how the system fits together (connection modes,
  boards, on-board C6, external BT agent, AA protocol, the two UI modes), finding
  where a component lives, or how to build / flash / debug. Indexes the helper
  skills (device-screen, capture-logs) and the canonical docs.
---

# ESP32-P4 Android Auto head unit — project map

A wireless **Android Auto** head unit running on an ESP32-P4 with an 800×480
touch LCD, that doubles as a **VESC** e-vehicle dashboard. The phone projects
over Wi-Fi; when not projecting, the panel shows a VESC dashboard fed by CAN.

`CLAUDE.md` is the canonical, detailed spec (board matrix, both modes verbatim,
reference repos, how to reproduce git-ignored artifacts, versioning/release).
This skill is the quick map; reach for `CLAUDE.md` for depth.

## What talks to what

```
phone ──BT handshake (Mode A) / mDNS (Mode B)──▶ joins Wi-Fi
      ──TCP :5277 + TLS──▶ AA protocol ──H.264──▶ panel 800×480
on-board ESP32-C6 = Wi-Fi6 transport over SDIO (ESP-Hosted)
VESC ──CAN──▶ dashboard (when not projecting)
```

- **Two connection modes**, chosen by `CONNECTION_MODE` in `main/config.h`:
  - **Mode A** `MODE_BT_CLASSIC` — an external ESP32 BT-agent module does the
    Classic-BT handshake and hands Wi-Fi creds to the P4 over UART (acts like a
    real car stereo, no phone app).
  - **Mode B** `MODE_WIRELESS_HELPER` — no extra hardware; the Wireless Helper
    APK finds the unit by mDNS and starts AA.
- **Boards**: Waveshare ESP32-P4-WIFI6-Touch-LCD-4.3 (default, 32 MB) and
  Guition JC4880P443C (16 MB). Display: ST7701 800×480 MIPI-DSI, GT911 touch.

## Build / flash / debug

```bash
. "$IDF_PATH/export.sh"
idf.py build                              # = Waveshare (default, back-compat)
scripts/build_board.sh waveshare -p <PORT> flash monitor
scripts/build_board.sh jc4880   -p <PORT> flash      # 16 MB board
scripts/build_board.sh                               # build ALL boards (release imgs)
scripts/ota_push.sh                                  # OTA over Wi-Fi to 192.168.4.1
```

- The **C6 Wi-Fi firmware** is embedded in the P4 binary and auto-OTA'd at boot
  on version mismatch (`main/c6_ota.c`, blob in `components/c6_ota_partition/`).
- Multi-board mechanics (Kconfig `BOARD_MODEL`, per-board `sdkconfig.defaults.*`,
  16 MB partition table): see `CLAUDE.md` → "Поддержка нескольких девайсов".

## Codebase map (`main/` unless noted)

- **Wi-Fi / transport**: `wifi_manager.c`, `tcp_server.c`, `mdns_advertise.c`, `c6_ota.c`
- **AA protocol** (port of aasdk): `aa_frame/tls/handshake/proto/service.c`, `aa_x509_patch.c` (mbedTLS UTCTime wrap), `aa_overclock.c`
- **Video / display**: `h264_pipe.c` (SW H.264 → I420), `display_video.c` (direct-to-panel blit), `display_init.c`, `aa_overlay.c`; BSP in `components/esp32_p4_wifi6_touch_lcd_4_3/`
- **Touch / mode**: `touch_input.c` (single GT911 reader, `TOUCH_MODE_AA` vs `TOUCH_MODE_LVGL`), `ui_mode.c` (3-finger gesture toggles the two modes)
- **VESC dashboard**: `vesc_ui` component (GUI-Guider project under `Super_VESC_Display/`, switchable **themes** framework), `vesc_ui_updater.c`, plus `vesc_can` / `vesc_config` components; trip logging in `trip_log` component
- **BLE + phone bridge**: `ble_host/ble_nus/ble_ota/ble_files.c`, `notif_bridge.c` / `notif_toast.c`, `music_info_view.c` (album art via HW JPEG decode)
- **Other UI**: `idle_screen.c`, `ota_screen.c`, `files_screen.c`, `splash_screen.c` (boot GIF)
- **External BT agent fw**: `tools/bt_agent` (git-ignored, reproducible) embedded via `components/bt_agent_fw`; P4↔agent UART link in `bt_link.c`
- **Debug bridge**: `debug_uart_bridge.c` (screenshot + touch injection — see device-screen skill)
- **Config**: `main/config.h` (mode), `main/Kconfig.projbuild` (board, Wi-Fi role, debug bridge), `main/lv_conf.h` (LVGL, `LV_CONF_SKIP=n`)

## LVGL note

LVGL **v8.4**, RGB565, single source of truth = `main/lv_conf.h` (the LVGL
component's own Kconfig `CONFIG_LV_*` symbols are **ignored** because
`CONFIG_LV_CONF_SKIP=n`). Native panel is 480×800 portrait, rotated 90° → 800×480
landscape logical space.

## Helper skills

- **device-screen** — screenshot the live panel + inject tap/swipe over the UART
  debug bridge (UI testing / visual verification on hardware; LVGL layer only).
- **capture-logs** — capture serial logs from the P4, the BT-agent module, and/or
  the phone (logcat), plus OTA push.
- **build-flash** — build per board, flash (USB / Wi-Fi / BLE OTA), build-time
  options, the embedded C6 firmware.
- **dashboard-ui** — work on the LVGL VESC dashboard (themes, fonts, GUI-Guider,
  and the board's LVGL pitfalls).
- **release** — version bump, multi-board build, APK bundling, artifacts.

## Canonical docs & memory

- `CLAUDE.md` — full spec, board matrix, both modes, reference repos, artifact
  reproduction, versioning/release flow.
- `README.md` — user-facing feature list (file browser, BLE file manager, boot
  splash, etc.).
- The agent's `project_*` / `feedback_*` memory files record non-obvious gotchas
  (e.g. H.264 IRAM must stay off, NVS-in-LVGL-thread freezes, dashboard themes).
