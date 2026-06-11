---
name: device-screen
description: >
  Capture a screenshot of the ESP32-P4 head-unit's live screen and inject touch
  (tap/swipe) over the UART debug bridge, to visually verify a dashboard / LVGL
  UI change on real hardware. Use when asked to "see the device screen",
  "screenshot the device", "tap/swipe on the device", drive the UI, or verify a
  UI change on hardware. Includes a project reference (board, display geometry,
  touch/AA modes, build commands).
---

# Driving the ESP32-P4 head-unit screen over UART

This project is a wireless Android Auto head unit + VESC dashboard on an
ESP32-P4 with an 800×480 touch LCD. The firmware ships a **UART debug bridge**
(`main/debug_uart_bridge.c`, host tool `scripts/uart_debug.py`) that lets you
grab the live screen as an image and inject synthetic touch events. Use it to
*see* what's on the panel and to click around for UI testing — no eyes-on
hardware required.

## The core loop

1. **Screenshot** the device to a PNG.
2. **Read** the PNG (you can view images) to see the current screen.
3. Work out the pixel coords of the element you want.
4. **Tap / swipe** there.
5. Screenshot again to confirm the result.

```bash
# 1. find the serial port (macOS)
ls /dev/cu.usbserial-* /dev/cu.usbmodem* 2>/dev/null

# 2. one-time host deps
pip install pyserial Pillow

# 3. capture -> then Read the PNG with the Read tool
python3 scripts/uart_debug.py -p <PORT> screenshot /tmp/scr.png
#   add --fmt rgb565 for a lossless (slow, ~90 s) capture
#   add --swap-rb if colors look red/blue swapped

# 4. inject touch (coords are LVGL landscape, see geometry below)
python3 scripts/uart_debug.py -p <PORT> tap 400 240
python3 scripts/uart_debug.py -p <PORT> swipe 700 240 100 240 300
python3 scripts/uart_debug.py -p <PORT> touchdown 100 100   # low-level
python3 scripts/uart_debug.py -p <PORT> touchmove 400 100   # held drag
python3 scripts/uart_debug.py -p <PORT> touchup
```

Set `AA_PORT` in the env to skip `-p` every time.

## Prerequisite: build with the bridge enabled

The bridge is gated behind `CONFIG_DEBUG_UART_BRIDGE` (default **off**). Enable
it and flash before the commands above work:

```bash
. "$IDF_PATH/export.sh"
# menuconfig: Android Auto Head Unit -> "UART0 debug bridge" = y
scripts/build_board.sh waveshare menuconfig    # or jc4880
scripts/build_board.sh waveshare -p <PORT> flash
```

Or build straight into a dir with a one-line overlay:
`SDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.defaults.waveshare;<file with CONFIG_DEBUG_UART_BRIDGE=y>"`.
Keep it **off** for production / the tight jc4880 OTA slot.

## Screen geometry — touch coordinate system

- LVGL logical space is **800×480 landscape**. `(0,0)` = top-left,
  **x → right (0..799)**, **y → down (0..479)**.
- The panel is physically 480×800 portrait, rotated 90° by the BSP — the bridge
  already gives you the upright landscape image and accepts landscape coords, so
  you don't deal with the rotation.
- A `tap` presses ~80 ms then releases (registers an LVGL click). `swipe`
  interpolates from start to end over `ms` (default 300).

## Important caveats

- **Captures the LVGL layer only** — the VESC dashboard, menus, settings,
  toasts. It does **NOT** capture the Android Auto video layer (AA video is
  drawn straight to the panel via `esp_lcd_panel_draw_bitmap` in
  `display_video.c`, bypassing LVGL). A screenshot during AA projection shows
  the LVGL background, not the phone's video.
- **No-reset connect**: the host opens the port with DTR/RTS held so connecting
  does **not** reboot the device (unlike `idf.py monitor`). Pass `--allow-reset`
  to opt out. Only one process can own the port — close `idf.py monitor` first.
- **Colors**: the HW JPEG encoder has no channel-order option; if a screenshot
  comes back with red/blue swapped, add `--swap-rb` (fixed host-side, no
  reflash). Raw `--fmt rgb565` is unaffected.
- **Baud**: host default is 115200 (matches the existing `build_*` dirs).
  `sdkconfig.defaults` says 921600, so a freshly-created build dir may be
  921600 — pass `-b 921600` then.
- **Wire format**: `screenshot` mutes logs and streams a base64 frame
  (`SCR-BEGIN … SCR-DATA … SCR-END`, CRC32-checked). Touch injection only
  affects `TOUCH_MODE_LVGL`; it auto-expires so a dead host never wedges the
  panel.

## Project reference (what you're looking at)

- **Board(s)**: Waveshare ESP32-P4-WIFI6-Touch-LCD-4.3 (default) and Guition
  JC4880P443C (16 MB). Build per board with `scripts/build_board.sh <board>`;
  plain `idf.py build` = waveshare. See `CLAUDE.md` for the full board matrix.
- **Display**: 800×480 ST7701 MIPI-DSI, GT911 capacitive touch, RGB565, LVGL
  v8.4 via `esp_lvgl_adapter` (DOUBLE_FULL, ROTATE_90). Init in
  `main/display_init.c`; BSP in `components/esp32_p4_wifi6_touch_lcd_4_3/`.
- **Two screen modes**, multiplexed by `main/touch_input.c`
  (`touch_input_set_mode`): `TOUCH_MODE_LVGL` (VESC dashboard, the default at
  boot) and `TOUCH_MODE_AA` (Android Auto projection). A 3-finger gesture
  toggles between them (`ui_mode.c`, `ui_mode_toggle`).
- **The dashboard** is a GUI-Guider project under `Super_VESC_Display/`
  (generated into the `vesc_ui` component) with a switchable themes framework.
  VESC realtime data arrives over CAN; the UI updates via `vesc_ui_updater.c`.
- **Key UI files**: `main/idle_screen.c` (connect screen), `main/ota_screen.c`,
  `main/notif_toast.c` / `music_info_view.c` (notifications/media tile),
  `main/splash_screen.c` (boot splash).
- **Touch injection point**: `touch_input_inject()` / `touch_input_inject_clear()`
  in `main/touch_input.c` (override over the GT911 poll atomics, LVGL mode only).

## When the device isn't reachable

If `screenshot` times out or the port won't open: confirm the firmware was
flashed with `CONFIG_DEBUG_UART_BRIDGE=y`, the right `-p <PORT>`, no other
process holds the port, and the baud matches (`-b`). For pure layout work
without hardware, the GUI-Guider simulator under `Super_VESC_Display/` renders
the same dashboard on the desktop.

## See also

- **head-unit** — project overview and codebase map.
- **build-flash** — build with `CONFIG_DEBUG_UART_BRIDGE=y` and flash.
- **capture-logs** — serial logs (resets the chip; use that, not this, for boot logs).
- **dashboard-ui** — editing the LVGL UI you're screenshotting.
