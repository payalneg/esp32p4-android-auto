# VESC LISP script

`main.lisp` runs on the **VESC controller's** built-in LISP runtime (not on
the ESP32-P4). It does three things that the dashboard relies on:

1. **Cruise control** via the PPM `RX` pin button:
   - press once → lock current speed (switch into speed-control mode)
   - press throttle or brake → release cruise
   - the dashboard reads `cruise-active` over the LISP poll channel and
     lights the cruise indicator
2. **Speed profiles** via the PPM `TX` pin button — three presets
   (25 / 40 / 60 km/h, configurable in `apply-profile`). Different beep
   tones confirm each profile change.
3. **Quick-action panel** — the swipe-out drawer on the P4 dashboard
   (left-edge swipe). This script is the *server* for it: the P4 asks
   "describe your controls" and we answer with a list the firmware renders
   (toggles / buttons / numbers / read-only values); interactions come back
   here. The starter set is **Throttle on/off** and **Traction Control**
   (toggle + a sensitivity number).

Without this script the dashboard still shows live telemetry (battery,
speed, voltage, temps over standard VESC CAN), but the cruise indicator,
profile switching and the quick-action panel won't work.

## Quick-action panel protocol

Transport is `COMM_CUSTOM_APP_DATA` (id 36): the firmware delivers inbound
frames to the `event-data-rx` event, and we reply with
`(send-data buf 2 reply-can-id)` — interface `2` is CAN, `reply-can-id` is the
P4's own controller id, which it puts in every request (needs VESC FW **6.05+**
for the explicit-interface `send-data`). The app payload is framed
`[0x56 0x50 ('V''P')][msg][...]`; all floats are `int32 = round(value*1000)`,
big-endian. The C side of the protocol is
`components/vesc_can/vesc_lisp_panel.{c,h}`.

The panel logic lives at the **bottom of `main.lisp`** (`panel-*` functions,
`monitor-traction`, the `event-data-rx` loop). To add a control: bump the
`count` in `panel-send-ui`, append its descriptor there, add its live value to
`panel-send-state`, and handle its id in `panel-action`.

> The **throttle-disable** (`app-disable-output`) and **traction-control**
> (`monitor-traction`) mechanics are a working *template* — tune them to your
> app type (ADC / PPM / UART) and vehicle.

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
- Quick-action panel controls — the `panel-*` functions at the bottom (see
  "Quick-action panel protocol" above). Traction-control aggressiveness —
  the `limit` formula in `monitor-traction`.
