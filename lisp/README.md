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

## Quick-action panel — describing the menu in LISP

The drawer is **data-driven**: this script tells the P4 *what controls to draw*,
the P4 renders them and sends back interactions. Nothing about the menu is
hard-coded in the firmware, so you change the menu by editing LISP only — no
firmware rebuild.

### How the conversation works

```
        user swipes from the left edge of the dashboard
                          │
  P4 ─ REQ_UI ──────────► us        "describe your controls"
  us ─ UI_DESC ─────────► P4        list of controls (this is the menu)
            … drawer rendered, then every ~200 ms while open:
  P4 ─ REQ_STATE ───────► us
  us ─ STATE ───────────► P4        live values (refresh toggles/numbers/labels)
        user taps a control
  P4 ─ ACTION ──────────► us        (control-id, new value)
  us ─ STATE ───────────► P4        echo the applied state back
```

Transport is `COMM_CUSTOM_APP_DATA` (id 36): the firmware delivers inbound
frames to the `event-data-rx` event, and we answer with
`(send-data buf 2 reply-can-id)` — interface `2` is CAN, `reply-can-id` is the
P4's own controller id, which it puts in every request (needs VESC FW **6.05+**
for the explicit-interface form of `send-data`). The C side is
`components/vesc_can/vesc_lisp_panel.{c,h}`; everything below is its mirror.

### Conventions

- **Framing.** Every payload starts with the magic bytes `0x56 0x50` ('V' 'P'),
  then a 1-byte message type, then the body. The `COMM_CUSTOM_APP_DATA` byte
  itself is added/stripped by the firmware, so it never appears in `data` you
  receive or in the buffer you send.
- **Numbers are scaled integers.** Every float on the wire is
  `int32 = round(value * 1000)`, **big-endian**. So `0` → `0`, `100` → `100000`,
  `5` → `5000`, `tc-sens` → `(* tc-sens 1000.0)`. The P4 divides by 1000 again.
- **Strings are NUL-terminated.** A LISP string literal *is* a byte array that
  already ends in `0`, and `buflen` counts that terminator — so `(pstr "Beep")`
  emits `B e e p \0` and `(pstr "")` emits a single `\0` (used for "no suffix").

### Append helpers (already in `main.lisp`)

These write into the scratch buffer `pbuf` at the running index `pi`:

| helper        | writes                                         |
|---------------|------------------------------------------------|
| `(pu8 v)`     | one byte                                        |
| `(pi32 v)`    | `v` as big-endian int32 (you pre-multiply ×1000)|
| `(pstr s)`    | the string `s` **including** its NUL terminator |

`pbuf` is `(bufcreate 96)` — zero-filled and over-sized on purpose: the P4 reads
exactly `count` controls and ignores trailing zeros, so you only need to grow
the `96` if a *single* reply gets bigger than that.

### Message catalogue

**Inbound — what the P4 sends us** (`data`, magic already validated):

| msg                | bytes                                                        |
|--------------------|--------------------------------------------------------------|
| `REQ_UI`    `0x01` | `[0]=V [1]=P [2]=0x01 [3]=reply-id`                          |
| `ACTION`    `0x02` | `… [2]=0x02 [3]=reply-id [4]=ctrl-id [5..8]=i32 value*1000`  |
| `REQ_STATE` `0x03` | `… [2]=0x03 [3]=reply-id`                                    |

**Outbound — what we send back** (`send-data pbuf 2 reply-id`):

```
UI_DESC (0x81):  0x56 0x50 0x81  <ver=1> <count>   then <count> controls
STATE   (0x82):  0x56 0x50 0x82  <count>           then <count> (id, value)
```

A control inside `UI_DESC` is `<id:u8> <type:u8> <label:str>` followed by a
type-specific tail:

| control | `type` | tail after `id type label`                                   |
|---------|:------:|--------------------------------------------------------------|
| toggle  | `1`    | `<state:u8 0/1>`                                              |
| button  | `2`    | *(nothing)* — momentary                                       |
| number  | `3`    | `<min:i32> <max:i32> <step:i32> <value:i32> <suffix:str>`     |
| label   | `4`    | `<value:i32> <suffix:str>` (read-only readout)               |

An entry inside `STATE` is just `<id:u8> <value:i32>`. Send a value for every
control that can change (toggles, numbers, labels); buttons have no state, so
just leave them out of `STATE`.

**ACTION value semantics** (what arrives in `panel-action`): toggle → `0.0`/`1.0`,
button → `1.0` (a press), number → the new value in real units (e.g. `55.0`).

### Recipe: add a control

Touch the same three functions, all near the bottom of `main.lisp`:

1. **`panel-send-ui`** — bump the `count` byte and append the descriptor.
2. **`panel-send-state`** — bump its `count` and append `(pu8 id) (pi32 value*1000)`
   (skip for buttons).
3. **`panel-action`** — add a `((= cid <id>) …)` branch that does the work
   (skip for read-only labels).

Keep the `id`s unique and stable; the P4 matches `STATE`/`ACTION` to controls by
`id`, not by position.

The starter menu already ships four controls — Throttle (id 1), Traction
Control (id 2), TC Sens (id 3) and a Beep button (id 4). Here's how you'd add a
fifth: a read-only "Motor temp" readout.

```lisp
; --- in panel-send-ui: change (pu8 4) [count] to (pu8 5), then append: ---
; id=5 Motor temp (read-only label, suffix "C")
(pu8 5) (pu8 4) (pstr "Motor temp") (pi32 (* (get-temp-mot) 1000.0)) (pstr "C")

; --- in panel-send-state: change (pu8 3) [count] to (pu8 4), then append: ---
; (push its live value so the readout refreshes ~5x/s while the drawer is open)
(pu8 5) (pi32 (* (get-temp-mot) 1000.0))

; --- panel-action: nothing to add — a read-only label takes no interaction. ---
```

For a button instead (like the built-in Beep, id 4), the descriptor is just
`(pu8 <id>) (pu8 2) (pstr "<label>")` with no tail and no `STATE` entry, and you
add a `((= cid <id>) <do-something>)` branch to `panel-action`.

> The starter **throttle-disable** (`app-disable-output`) and **traction-control**
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
