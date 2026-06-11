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
   here. The starter set: **Throttle on/off**, a **Beep** button, a **Beep Vol**
   number, **Play Melody** (the motor plays a tune) and its **Melody Vol** — both
   volumes persisted on shutdown. A **Traction Control** toggle + sensitivity is
   included but commented out (single motor → native VESC TC is a no-op).

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

> Step-by-step with copy-paste templates is in **"Add a control — the simple
> version"** above. The current `main.lisp` panel is a live worked example:
> Throttle (toggle, id 1), Beep (button, id 4), Beep Vol (number, id 5),
> Play Melody (toggle, id 6), Melody Vol (number, id 7) — both volumes persisted
> on shutdown. Traction Control (ids 2/3) is there but commented out — a ready
> template to copy.

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
  "Add a control — the simple version" above).
- Melody — replace the `(def melody '(...))` list (each entry `(freq-hz dur-s)`,
  `0` = rest); volume follows the Beep Vol control.
- Traction-control template — uncomment the TC controls + `monitor-traction`
  spawn; tune the `limit` formula. (Multi-motor only; no-op on a single motor.)
