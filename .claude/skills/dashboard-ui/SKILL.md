---
name: dashboard-ui
description: >
  Work on the VESC dashboard / LVGL UI of the ESP32-P4 head unit — screens,
  widgets, themes, fonts, layout. Use when editing the dashboard, GUI-Guider
  output, the themes framework, fonts, or any LVGL UI under Super_VESC_Display /
  main/. Encodes the hard-won LVGL pitfalls on this board (thread-safety, display
  mode, fonts, ellipsis, keyboard) so they aren't re-hit. LVGL v8.4, RGB565.
---

# Working on the VESC dashboard (LVGL UI)

The dashboard is a **GUI-Guider** project in `Super_VESC_Display/`, compiled into
the firmware via the `vesc_ui` component. There's also a desktop **SDL
simulator** for layout work without hardware. LVGL is **v8.4**, RGB565, logical
screen **800×480 landscape**.

## The #1 rule: `generated/` vs `custom/`

- `Super_VESC_Display/generated/` is **overwritten** every time the `.guiguider`
  project is re-exported from GUI-Guider. **Never put logic you want to keep
  there.**
- `Super_VESC_Display/custom/` is hand-written and survives regeneration — themes,
  on-device VESC-Tool config menu, dashboard logic live here
  (`dashboard_theme.c`, `theme_generic.c`, `theme_*.c`, etc.).
- Firmware-side UI glue lives in `main/` (`vesc_ui_updater.c`, `ui_mode.c`, the
  `*_screen.c` / `*_view.c` files).

## Themes framework

Switchable dashboard layouts in `custom/dashboard_theme.c` + `theme_<name>.c`
(e.g. `theme_dashboard_amber.c`, `theme_generic.c`):
- The generated `update_*()` functions are **dispatchers** → they call the active
  theme's ops. The active dashboard screen sits in the `guider_ui.dashboard`
  slot, so the generated navigation keeps working untouched.
- Persisted in NVS key **`dash_theme`**; a Settings dropdown switches it live.
- Adding a theme: implement the ops table in a new `theme_<name>.c`, register it
  in `dashboard_theme.c`, wire it into the Settings dropdown.

## Fonts — watch the flash budget

- The display font is **Antonio** (`import/font/Antonio-Regular.ttf`); the
  firmware build **auto-subsets** it into the build dir (GUI-Guider can't be
  allowed to bloat it). A full-ASCII Antonio is ~1.4 MB and **blows the jc4880
  16 MB board's ~6 MB OTA slot** — keep the subset tight.
- The **simulator** does NOT auto-subset — regenerate its fonts manually if you
  add glyphs and want them visible in the sim.
- Default Montserrat is **ASCII-only**: a literal `…` (U+2026) renders as a tofu
  box. **Use `...`** in strings, never the ellipsis character.

## LVGL pitfalls on this board (don't re-hit these)

- **No NVS work in the LVGL thread.** An `nvs_commit()` / erase on the LVGL task
  freezes the screen. Debounce setters (e.g. spinboxes) with a volatile RAM
  cache + deferred persist off the LVGL thread. `CONFIG_SPI_FLASH_AUTO_SUSPEND=y`
  keeps DSI rendering alive during a NVS erase (DSI DMA never blanks, only
  rendering would stall).
- **Display mode = DOUBLE_FULL + ROTATE_90**, with the Espressif PPA patch
  applied in IDF. Do **not** switch to TRIPLE_PARTIAL with ROTATE_90 — it hits a
  PPA/DMA2D ISR-loss bug on the P4 and the UI freezes. (`main/display_init.c`,
  `esp_lvgl_adapter`.)
- **Keyboard**: every keyboard show/hide MUST be paired with a content-container
  resize, or the focused textarea hides behind the keyboard / a black square
  appears below it. `LV_USE_KEYBOARD` is enabled (for the LISP editor).
- **Indev API is v8.4** (`lv_indev_drv_init` / `lv_indev_drv_register`), not v9.
  Touch comes through `main/touch_input.c` → the custom indev in `main.c`.
- LVGL config single source of truth = `main/lv_conf.h` (`CONFIG_LV_CONF_SKIP=n`;
  the LVGL component's own `CONFIG_LV_*` Kconfig symbols are ignored).

## Simulator (layout iteration without hardware)

`Super_VESC_Display/lvgl-simulator/` (+ `ports/`) is an SDL desktop build of the
same UI. It has its **own** `lv_conf` and font set (manual regen). Good for fast
layout work; for anything touching real timing / NVS / CAN, test on hardware.

## After a UI change — verify on hardware

Build + flash (see **build-flash**), then use the **device-screen** skill to
screenshot the panel and tap around — confirm the layout and that touch targets
land where expected. The screenshot captures the LVGL layer (exactly this UI).

## See also

- **device-screen** — screenshot + touch the running UI.
- **build-flash** — build/flash, font budget on jc4880.
- **head-unit** — project overview.
