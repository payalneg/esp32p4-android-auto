# VESC LISP script

## `main.adc-pas.lisp` v8: continuous ADC1 override (firmware 7.00)

This alternative script now commands only **virtual throttle ADC1**. It never
disables the ADC application, commands motor current directly, or commands
speed through `set-rpm`. Physical ADC2, its calibration and native braking
remain unchanged. The separate Lisp brake transition/ramp has been removed.

Priority is traction inhibit, brake, physical throttle, cruise, then PAS:

- Decoded ADC2 at or above 0.05 requests neutral ADC1 voltage and cancels
  cruise. Brake stays active until ADC2 falls to 0.02 or below, at which
  point an already armed throttle can resume. Stock ADC filtering,
  deadband and ramping still apply.
- Physical throttle has priority over both cruise and PAS. Its physical voltage
  is forwarded unchanged above the release threshold, preserving the native
  ADC curve and ramp. Cruise stays enabled while the throttle is held and
  resumes its original speed target when the throttle is released.
- PAS supplies virtual throttle only with the physical throttle released and
  cruise inactive. The input protocol is still motor amperes. Requests expire
  after 0.4 seconds; the existing sender lock is retained.
- Cruise uses a local ERPM **PI** controller with integral anti-windup.
  RX enables it or increases the
  target by 1 km/h; TX decreases the target by 1 km/h, down to 0.5 m/s without
  cancelling cruise. It supplies positive throttle only: overspeed reduces
  throttle to zero, never injects braking.
- Brake cancels cruise during normal riding. The master switch, an input or
  motor fault, VESC Tool output inhibition, or a control-loop stall also clear
  cruise. Startup and these inhibits require the physical throttle released
  before traction is armed. Braking remains available with the master off.

At startup `base-current-max` sets Motor Current Max to **100 A** in RAM;
the three existing profile scales give 50 / 67 / 100 A. Brake-current settings
are not changed. PAS and cruise requests are divided by the fixed 100 A
reference, clamped to 0..1, and mapped back through
the inverse throttle curve and deadband to calibrated voltage. Actual current
remains subject to firmware motor, battery, thermal and speed limits.

Before loading, select **ADC** or **ADC and UART**, ADC control type **Current
No Reverse Brake ADC** (8), and disable native ADC button functions. Calibrate
both controls normally in VESC Tool. The script checks app type, ADC1 calibration
and a nonzero motor timeout; incompatible configurations do not start override.
It does not automatically change the app selection or calibration.

These three constants at the top must match VESC Tool (7.00 does not expose
them through Lisp `conf-get`):

| Script constant | Matching ADC setting | Default |
| --- | --- | --- |
| `adc1-inverted` | ADC1 voltage inversion, 0 off / 1 on | 0 |
| `adc-curve-accel` | Acceleration throttle curve gain | 0.0 (linear) |
| `adc-curve-mode` | Curve mode: exponential / natural / polynomial | 0 |

Only PAS and cruise use inverse curve conversion. Manual throttle still goes
through the native curve unchanged. Calibration endpoints and deadband are
read live. ADC1 inversion is also needed to recognize a released
physical throttle and choose neutral voltage correctly.

`cruise-kp` and `cruise-ki` come from `main.lisp`. Error is in ERPM, time
in seconds and PI output in motor amps before conversion to ADC1 voltage.
Measured traction seeds the integral on engagement
and tracks it while the physical throttle overrides cruise.

Detached ADC uses the **global motor timeout**. Stopping override does not
reattach the physical throttle. Another command source that resets this timeout
can keep the last virtual input alive: do not use concurrent motor-command or
timeout-reset senders. Verify actual timeout behavior on the controller.

Host checks: `python scripts/test_adc_pas.py`. These execute the actual control
functions with a small subset evaluator and mocked hardware, including source
priority, brake hysteresis, PAS expiry/current scaling, inverse curves and
PI behavior. They do not validate VESC LispBM scheduling, heap/stack usage,
hardware filtering or vehicle stability. No controller flashing is performed.

The sections below describe the original `main.lisp` and the shared panel
protocol; they do not describe the v8 alternative's control algorithm.

`main.lisp` runs on the **VESC controller's** built-in LISP runtime (not on
the ESP32-P4). It does three things that the dashboard relies on:

1. **Cruise control** via the PPM `RX` pin button:
   - press once → lock current speed (switch into speed-control mode)
   - press throttle or brake → release cruise
   - the dashboard reads `cruise-active` over the LISP poll channel and
     lights the cruise indicator
2. **Speed profiles** — three presets (25 / 40 / 60 km/h, configurable in
   `apply-profile`), switchable two ways:
   - the PPM `TX` pin button cycles through them (`switch-profile`)
   - the on-screen panel picks one directly (`panel-set-profile`)
   Different beep tones confirm each profile change.
3. **Quick-action panel** — the swipe-out drawer on the P4 dashboard
   (left-edge swipe). This script is the *server* for it: the P4 asks
   "describe your controls" and we answer with a list the firmware renders
   (toggles / buttons / numbers / read-only values); interactions come back
   here. The starter set: **Throttle on/off**, a **Beep** button, a **Beep Vol**
   number (persisted) and the three **profile buttons** — a radio group, exactly
   one lit. A **Traction Control** toggle + sensitivity is included but commented
   out (single motor → native VESC TC is a no-op).

Without this script the dashboard still shows live telemetry (battery,
speed, voltage, temps over standard VESC CAN), but the cruise indicator,
profile switching and the quick-action panel won't work.

## Quick-action panel — describing the menu in LISP

The drawer is **data-driven**: this script tells the P4 *what controls to draw*,
the P4 renders them and sends back interactions. Nothing about the menu is
hard-coded in the firmware, so you change the menu by editing LISP only — no
firmware rebuild.

> **New here? Read "Add a control — the simple version" just below.** The
> byte-level sections after it are reference — you rarely need them.

### Add a control — the simple version

A control = one row in the drawer. You describe it in LISP, the P4 draws it.
Four kinds: **toggle** (on/off), **button** (press once), **number**
(−/value/+), **label** (read-only readout).

To add one, edit **3 spots** near the bottom of this file, all keyed by a
unique **id** number:

1. `panel-send-ui`    — bump the count, add one "describe" line.
2. `panel-send-state` — add its live value (skip for buttons).
3. `panel-action`     — say what happens when tapped (skip for labels).

Copy-paste templates — replace `ID`, `"Label"`, `myvar`, and the action:

```lisp
; TOGGLE  (on/off; myvar is 0 or 1)
;  ui:     (pu8 ID) (pu8 1) (pstr "Label") (pu8 myvar)
;  state:  (pu8 ID) (pi32 (* myvar 1000))
;  action: ((= cid ID) (setq myvar (if (> val 0.5) 1 0)))

; BUTTON  (momentary)
;  ui:     (pu8 ID) (pu8 2) (pstr "Label")
;  state:  — none —
;  action: ((= cid ID) (do-something))

; NUMBER  (−/value/+; this one is 0..100 step 5)
;  ui:     (pu8 ID) (pu8 3) (pstr "Label")
;          (pi32 0) (pi32 100000) (pi32 5000) (pi32 (* myvar 1000)) (pstr "")
;          ;        ^min×1000     ^max×1000   ^step×1000 ^value×1000 ^unit
;  state:  (pu8 ID) (pi32 (* myvar 1000))
;  action: ((= cid ID) (setq myvar (to-i32 val)))

; LABEL   (read-only; e.g. a live temperature)
;  ui:     (pu8 ID) (pu8 4) (pstr "Label") (pi32 (* (get-temp-mot) 1000)) (pstr "C")
;  state:  (pu8 ID) (pi32 (* (get-temp-mot) 1000))
;  action: — none —
```

Rules of thumb:
- **count** is the byte right after `0x81` (in `panel-send-ui`) / `0x82` (in
  `panel-send-state`). It must equal how many controls you actually list — add
  one, bump both.
- **id** is just a label; keep each unique, order doesn't matter.
- Numbers travel ×1000 (so `50` → `(pi32 (* 50 1000))`) — that's how decimals
  survive as whole numbers; the P4 divides back. Strings: `(pstr "...")`, empty
  unit = `(pstr "")`.

### Добавить контрол — по-простому

Контрол = одна строка в выезжающей панели. Ты описываешь её в LISP, а P4 рисует.
Четыре вида: **toggle** (вкл/выкл), **button** (нажал один раз), **number**
(−/значение/+), **label** (просто показать, без нажатия).

Чтобы добавить, правишь **3 места** внизу файла, всё завязано на уникальный
номер **id**:

1. `panel-send-ui`    — увеличь счётчик и добавь одну строку-«описание».
2. `panel-send-state` — добавь живое значение (для кнопки — не нужно).
3. `panel-action`     — что делать при нажатии (для label — не нужно).

Шаблоны — подставь `ID`, `"Название"`, `myvar` и действие:

```lisp
; TOGGLE  (вкл/выкл; myvar = 0 или 1)
;  ui:     (pu8 ID) (pu8 1) (pstr "Название") (pu8 myvar)
;  state:  (pu8 ID) (pi32 (* myvar 1000))
;  action: ((= cid ID) (setq myvar (if (> val 0.5) 1 0)))

; BUTTON  (разовое нажатие)
;  ui:     (pu8 ID) (pu8 2) (pstr "Название")
;  state:  — нет —
;  action: ((= cid ID) (что-то-сделать))

; NUMBER  (−/значение/+; тут 0..100 шаг 5)
;  ui:     (pu8 ID) (pu8 3) (pstr "Название")
;          (pi32 0) (pi32 100000) (pi32 5000) (pi32 (* myvar 1000)) (pstr "")
;          ;        ^мин×1000     ^макс×1000  ^шаг×1000  ^знач×1000  ^единица
;  state:  (pu8 ID) (pi32 (* myvar 1000))
;  action: ((= cid ID) (setq myvar (to-i32 val)))

; LABEL   (только показать; напр. температуру)
;  ui:     (pu8 ID) (pu8 4) (pstr "Название") (pi32 (* (get-temp-mot) 1000)) (pstr "C")
;  state:  (pu8 ID) (pi32 (* (get-temp-mot) 1000))
;  action: — нет —
```

Памятка:
- **count** — это байт сразу после `0x81` (в `panel-send-ui`) / `0x82` (в
  `panel-send-state`). Он должен равняться числу реально перечисленных контролов:
  добавил один — увеличь оба.
- **id** — просто метка; держи уникальным, порядок не важен.
- Числа едут ×1000 (`50` → `(pi32 (* 50 1000))`) — так дроби переживают пересылку
  целыми, P4 делит обратно. Строки: `(pstr "...")`, пустая единица = `(pstr "")`.

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

> **Three copies of these rules exist and must stay in step.** This file is the
> human one. The in-app AI assistant is told the same protocol in
> `flutter-application/lib/agent/lisp_reference.dart` (byte layout) and
> `agent_prompt.dart` (the fail-silently rules), and
> `flutter-application/lib/agent/lisp_lint.dart` enforces it before letting the
> assistant flash. Change the wire format and you change all three, plus
> `components/vesc_can/`.

Transport is `COMM_CUSTOM_APP_DATA` (id 36): the firmware delivers inbound
frames to the `event-data-rx` event, and we answer with
`(send-data buf 2 reply-can-id)` — interface `2` is CAN, `reply-can-id` is the
P4's own controller id, which it puts in every request (needs VESC FW **6.05+**
for the explicit-interface form of `send-data`).

A reply-id of **255** means the request did not arrive over CAN at all: the
head unit is connected through a VESC Express BLE adapter and has no CAN id of
its own. The script then answers with plain `(send-data buf)` — the interface
the request came in on — and the reply rides back up the adapter's BLE link.
255 is the CAN broadcast address, so no real node can ever claim it. The C side is
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

`pbuf` is `(bufcreate 128)` — zero-filled and over-sized on purpose: the P4 reads
exactly `count` controls and ignores trailing zeros, so you only need to grow
the `128` if a *single* reply gets bigger than that. The current `UI_DESC` is
102 B, so there is room for roughly one more short control before you must bump
it (the whole buffer goes on the wire every reply, so don't over-grow it).

### Message catalogue

**Inbound — what the P4 sends us** (`data`, magic already validated):

| msg                | bytes                                                        |
|--------------------|--------------------------------------------------------------|
| `REQ_UI`    `0x01` | `[0]=V [1]=P [2]=0x01 [3]=reply-id`                          |
| `ACTION`    `0x02` | `… [2]=0x02 [3]=reply-id [4]=ctrl-id [5..8]=i32 value*1000`  |
| `REQ_STATE` `0x03` | `… [2]=0x03 [3]=reply-id`                                    |

**Outbound — what we send back** (`send-data pbuf 2 reply-id`, or plain
`send-data pbuf` when reply-id is 255 — see above):

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

> Step-by-step with copy-paste templates is in **"Add a control — the simple
> version"** above. The current `main.lisp` panel is a live worked example:
> Throttle (toggle, id 1), Beep (button, id 4), Beep Vol (number, id 5, persisted)
> and the profile radio group — Slow / Medium / Fast (toggles, ids 10/11/12).
> Traction Control (ids 2/3) is there but commented out — a ready template to
> copy.
>
> **Radio group pattern** (the profiles): N toggles whose `state` is
> `(if (= sel K) 1 0)`, and each action sets `sel` to *its own* index while
> ignoring the incoming value. Tapping the lit row therefore sends `0` and does
> nothing — the `STATE` echo lights it straight back up.

## How to flash

Open **VESC Tool → VESC Packages → Lisp Scripting**, load
`lisp/main.lisp`, then **Upload** and **Activate**. Save to flash so it
survives reboot.

## Customising

- Speed presets — edit `apply-profile` (the three `conf-set 'max-speed`
  lines near the top of the file). Adding/removing one means: `num-profiles`,
  the `beep-freq` table, and the panel rows (ids 10..12 in `panel-send-ui` /
  `panel-send-state` / `panel-action`) — keep the labels in step with the
  speeds, the P4 only draws what this script says.
- Throttle/brake deadzones — `monitor-throttle-brake`.
- Quick-action panel controls — the `panel-*` functions at the bottom (see
  "Add a control — the simple version" above).
- Traction-control template — uncomment the TC controls + `monitor-traction`
  spawn; tune the `limit` formula. (Multi-motor only; no-op on a single motor.)
