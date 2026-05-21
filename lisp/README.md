# VESC LISP script

`main.lisp` runs on the **VESC controller's** built-in LISP runtime (not on
the ESP32-P4). It does two things that the dashboard relies on:

1. **Cruise control** via the PPM `RX` pin button:
   - press once → lock current speed (switch into speed-control mode)
   - press throttle or brake → release cruise
   - the dashboard reads `cruise-active` over the LISP poll channel and
     lights the cruise indicator
2. **Speed profiles** via the PPM `TX` pin button — three presets
   (25 / 40 / 60 km/h, configurable in `apply-profile`). Different beep
   tones confirm each profile change.

Without this script the dashboard still shows live telemetry (battery,
speed, voltage, temps over standard VESC CAN), but the cruise indicator
and profile switching won't work.

## How to flash

Open **VESC Tool → VESC Packages → Lisp Scripting**, load
`lisp/main.lisp`, then **Upload** and **Activate**. Save to flash so it
survives reboot.

## Customising

- Speed presets — edit `apply-profile` (the three `conf-set 'max-speed`
  lines near the top of the file).
- Beep frequencies / profile count — `num-profiles` plus the `beep-freq`
  table.
- Throttle/brake deadzones — `monitor-throttle-brake`.
