/// VESC LispBM API reference handed to the model.
///
/// Signatures come from the upstream reference
/// (github.com/vedderb/bldc → lispBM/README.md, mirrored at lispbm.com), not
/// from guesswork: a made-up builtin costs a whole flash-and-verify cycle to
/// discover, because the script dies with `variable_not_bound` and takes its
/// thread with it. Where this project has a house rule that contradicts the
/// generic advice (which call to prefer, where a `def` must live), the rule is
/// stated next to the call.
///
/// `opt…` arguments are optional.
///
/// Kept in its own file so the system prompt stays one stable, cacheable
/// string — see agent_prompt.dart.
library;

const String kLispReference = r'''
# VESC LispBM reference

Upstream docs: github.com/vedderb/bldc → lispBM/README.md. If you need a
builtin that is not listed here, say so instead of guessing — a wrong name
kills the thread that calls it, silently.

## Language

```
(def name value)              ; global. If you ever setq it, it MUST be above @const-start
(defun f (a b) body)
(defun g () { (a) (b) })      ; { ... } is progn sugar, used everywhere in this script
(setq name value)             ; assign to an existing binding
(let ((a 1) (b 2)) body)
(if cond then else)           ; else optional
(cond ((t1) b1) ((t2) b2) (t fallback))
(and a b) (or a b) (not a)
(loopwhile cond { ... })  (loopfor i 0 (< i n) (+ i 1) { ... })  (looprange i 0 n { ... })
(spawn stack-words fn arg…)   ; e.g. (spawn 150 my-loop)
(atomic expr…)                ; run without being preempted
(recv (pattern body) …)       ; block on messages/events
(sleep seconds)               ; float
```

Numbers/lists: `+ - * / mod = > < >= <=`, `abs`, `length`, `ix lst i`, `list`,
`append`, `map`, `foldl`. Strings: `str-merge`, `to-str`, `str-len`.
Conversion: `to-i32`, `to-float`, `to-i`, `to-byte`.
Time: `(systime)` ticks, `(secs-since timestamp)` — the idiomatic staleness test.
Output: `(print arg1 … argN)`, `(puts str)`.

## Motor — read

```
(get-rpm)              ; electrical RPM, signed   (get-rpm-fast) (get-rpm-faster)
(get-speed)            ; m/s                       (get-speed-set)
(get-current optFilter) (get-current-in optFilter) (get-current-dir)
(get-duty) (get-duty-abs)
(get-vin)              ; battery volts
(get-batt)             ; state of charge 0..1
(get-temp-fet optFet) (get-temp-mot)
(get-fault)            ; fault code, 0 = none
(get-dist) (get-dist-abs) (get-ah) (get-wh) (get-ah-chg) (get-wh-chg)
(get-adc ch) (get-adc-decoded ch)   ; ch 0 = throttle, 1 = brake; decoded is 0..1
(get-ppm) (get-ppm-age)
(get-pos) (get-encoder)
```

## Motor — command

Every motor command feeds the firmware's command timeout: keep sending or the
motor stops after ~1 s. That is a safety feature — do not defeat it.
`(timeout-reset)` exists but is almost never the right answer.

```
(set-current amps optOffDelay)
(set-current-rel rel optOffDelay)   ; 0..1 of the LIVE limit — PREFER THIS: it follows
                                    ; l-current-max, the profile scale and thermal derating
(set-brake amps) (set-brake-rel rel)
(set-handbrake amps) (set-handbrake-rel rel)
(set-duty d)        ; -1..1
(set-rpm rpm) (set-pos degrees)
(set-kill-sw state)
(foc-openloop current rpm)
(throttle-curve value accel brake mode)  ; mode 0 exp, 1 natural, 2 polynomial
```

## App / ADC control

```
(app-disable-output ms)     ; suppress the stock ADC/PPM app output for ms.
                            ; Re-arm it every tick: when the script dies the
                            ; stock throttle comes back and the bike stays rideable
(app-is-output-disabled)
(app-adc-detach mode state) (app-adc-override mode value)
(app-ppm-detach state) (app-ppm-override value)
(app-pas-get-rpm)           ; cadence from the firmware's own PAS app
```

## Configuration

```
(conf-get 'param optDefault)    ; live VESC Tool value
(conf-set 'param value)         ; applies immediately, RAM only
(conf-store)                    ; persist to flash — slow, wears flash, avoid in loops
(conf-restore)
```

Parameters used here: `'l-current-max`, `'l-current-max-scale`, `'max-speed`
(m/s), `'adc-ramp-time-pos`, `'adc-ramp-time-neg`. Others include `'l-max-duty`,
`'si-motor-poles`, `'foc-motor-r`, `'app-to-use`, `'adc-ctrl-type`.

House rule: scale a limit (`l-current-max-scale`) rather than overwrite it, so
the rider's own VESC Tool numbers stay the master value.

## CAN

```
(can-scan) (can-list-devs) (can-ping id) (can-local-id)
(can-send-sid id data) (can-send-eid id data)     ; data is a byte buffer
(can-recv-sid optTimeout) (can-recv-eid optTimeout)
(can-msg-age id msg)                              ; freshness of a received status frame
(canset-current id amps optOffDelay) (canset-current-rel id rel optOffDelay)
(canset-brake id amps) (canset-brake-rel id rel)
(canset-duty id duty) (canset-rpm id rpm) (canset-pos id pos)
(canget-current id) (canget-rpm id) (canget-duty id) (canget-vin id)
(canget-temp-fet id) (canget-temp-motor id) (canget-speed id) (canget-adc id ch)
```

## Buffers

```
(bufcreate n)              ; MUST be above @const-start — a flashed buffer is read-only
(buflen b)
(bufget-u8 b i) (bufget-u16 b i) (bufget-i32 b i) (bufget-f32 b i)
(bufset-u8 b i v) (bufset-u16 b i v) (bufset-i32 b i v) (bufset-f32 b i v)
(bufcpy dst di src si len) (bufclear b)
(crc16 buf optLen) (crc32 buf init optLen)
(send-data buf 2 can-id)   ; interface 2 = CAN with an explicit destination id
```

## GPIO, sound, persistence

```
(gpio-configure 'pin-rx 'pin-mode-in-pu)   ; also 'pin-mode-out, 'pin-mode-in
(gpio-read 'pin-rx) (gpio-write 'pin-tx 1)
(foc-play-tone channel freq voltage) (foc-play-stop) (foc-beep freq time voltage)
(eeprom-store-i addr int) (eeprom-read-i addr)      ; addr is a small slot index; nil if unset
(eeprom-store-f addr float) (eeprom-read-f addr)
(shutdown-hold t) … (shutdown-hold nil)             ; keep power up across a write
```

Flash wear is real: never `eeprom-store-i` or `conf-store` from a fast loop.
This script coalesces writes behind a dirty flag plus a 2 s flush loop.

## Events

```
(event-register-handler (spawn my-loop))
(event-enable 'event-data-rx)     ; COMM_CUSTOM_APP_DATA arrived
(event-enable 'event-can-sid)     ; raw standard-id CAN frame  ('event-can-eid for extended)
(event-enable 'event-shutdown)
(defun my-loop () {
    (loopwhile t {
        (recv ((event-data-rx . (? data))        (handle data))
              ((event-can-sid (? id) . (? data)) (on-can id data))
              (event-shutdown                    (on-shutdown))
              (_ nil))
    })
})
```

`event-shutdown` fires on a real power-off ONLY — not on a bench reboot, a USB
reset or a re-flash. Anything that must survive needs a timer flush as well.

## This project's conventions

* The head unit talks to the script over `COMM_CUSTOM_APP_DATA`: magic
  `0x56 0x50` ('V' 'P'), a 1-byte message id, then a reply-id (the sender's CAN
  id). Replies are built in `pbuf` at index `pi` with `(pu8 v)`, `(pi32 v)`,
  `(pstr s)` and sent with `(send-data pbuf 2 reply-id)`. Full byte layout in
  "Quick-action panel protocol" below.
* ONE loop commands the motor (`motor-control-loop`), with a fixed priority:
  master-off > brake > throttle > cruise > pedal assist > coast. Add a new
  source as a branch there; never call `set-current` from a second thread.

### The motor arbiter, and why it is shaped like that

Each of these was paid for on hardware. Do not simplify one away.

* 100 Hz tick (`ctl-dt 0.01`). At 20 Hz the ramp advanced in 12.5 %-of-max
  steps, which FOC executes instantly — the rider felt jerks, not a ramp.
* `(app-disable-output 1500)` is refreshed EVERY tick deliberately. The stock
  ADC app stays configured (`get-adc-decoded` only works while it runs); only
  its output is suppressed. If this script dies the motor stops on the command
  timeout and the stock throttle returns ~1.5 s later, so the bike stays
  rideable. Never make it a one-shot, never detach the ADC app.
* Safe start: no output until the throttle has been seen released once
  (`armed`) — a held throttle at script start must not launch the bike.
* A branch stays selected while its slew tails off (`out-rel` / `brk-rel` above
  zero), otherwise releasing below the 0.05 threshold steps current to 0.
* Cruise is a PI controller with a CURRENT output, not `set-rpm` — the mode
  switch is what jerked on engage; the integrator is seeded with
  `(get-current)` for a bumpless transfer.
* Ramp times come live from `(conf-get 'adc-ramp-time-pos)` / `-neg`, clamped
  away from zero (a zero would divide-by-zero and kill the thread). Tuning
  stays in VESC Tool — do not hardcode ramps.

## Quick-action panel protocol

The drawer is a menu this script describes at runtime, so it changes with LISP
alone — no firmware rebuild. `panel-handle` runs on the `event-data-rx` thread:
nothing it calls may sleep or block; spawn instead.

Inbound — `data`, magic already checked:

```
[0]=0x56 [1]=0x50 [2]=msg [3]=reply-id
  0x01 REQ_UI     -> reply UI_DESC
  0x03 REQ_STATE  -> reply STATE    (~5 Hz while the drawer is open)
  0x04 REQ_DASH   -> reply DASH     (~5 Hz always, drawer open or not)
  0x02 ACTION     [4]=ctrl-id [5..8]=i32 value*1000 -> apply, then reply STATE
  0x05 PAS_SET    [4..7]=i32 amps*1000 — fire-and-forget, send NO reply
```

Outbound — built in `pbuf`, always starting with `(setq pi 0)`. Skip that reset
and the writes run off the end of the buffer; the evaluation error kills
`panel-event-loop` and takes the shutdown handler with it.

```
UI_DESC  0x56 0x50 0x81 <ver=1:u8> <count:u8>  then <count> controls
STATE    0x56 0x50 0x82 <count:u8>             then <count> x <id:u8> <val:i32>
DASH     0x56 0x50 0x84 <cruise-active:i32> <cruise-rpm:i32>
                        <current-profile:i32> <rpm-per-ms:i32>
```

Only UI_DESC carries a version byte. DASH carries neither version nor count —
its four fields are a fixed struct on the head unit.

A control inside UI_DESC is `<id:u8> <type:u8> <label:str>` — that order —
followed by a tail chosen by the type:

```
1 toggle : <state:u8, 0 or 1>     <- a plain byte here, NOT scaled
2 button : (nothing)
3 number : <min:i32> <max:i32> <step:i32> <value:i32> <suffix:str>
4 label  : <value:i32> <suffix:str>
```

Every i32 here is `round(value * 1000)`, big-endian — including a toggle's 0/1
inside STATE, which travels as 0 or 1000. An ACTION value arrives already
divided, in real units: toggle 0.0 or 1.0, button 1.0, number e.g. 55.0.

Strings are NUL-terminated and `buflen` counts the terminator, so `(pstr "")`
still writes one byte. A label may be at most 39 bytes and a unit suffix 11: a
longer one leaves the P4 decoder stopped mid-string and everything after it in
the frame is misread. Sizes, with L and S the label and suffix lengths:

```
toggle L+4    button L+3    number L+S+20    label L+S+8
UI_DESC = 5 + those;  STATE = 4 + 5 per entry;  pbuf is (bufcreate 128)
```

`pbuf` is never cleared between replies — only `pi` is reset — which is why a
count byte larger than what you wrote makes the P4 read leftovers from the
last, longer reply. Grow the 128 only when one reply no longer fits, and keep
the `bufcreate` above `@const-start`.

# Worked examples

## An observable debug counter

The reliable way to prove a loop runs — it works even when the print console
does not. Note where the `def` goes and where the `spawn` goes.

```lisp
(def dbg-tick 0)            ; ABOVE @const-start: it is setq'd
@const-start
(defun dbg-loop () {
    (loopwhile t {
        (setq dbg-tick (+ dbg-tick 1))
        (sleep 1)
    })
})
(spawn 150 dbg-loop)        ; AFTER the defun it references
@const-end
```

Flash it with `moving_globals: ["dbg-tick"]` and verification fails if the
value never moves.

## Add a control to the quick-action panel

Three edits keyed by one unused id, plus the count byte in both senders.
Replace ID, the label and `myvar`:

```lisp
; TOGGLE  (myvar is 0 or 1)
;  ui:     (pu8 ID) (pu8 1) (pstr "Label") (pu8 myvar)
;  state:  (pu8 ID) (pi32 (* myvar 1000))
;  action: ((= cid ID) (setq myvar (if (> val 0.5) 1 0)))
; BUTTON  (momentary — no state entry)
;  ui:     (pu8 ID) (pu8 2) (pstr "Label")
;  action: ((= cid ID) (do-something))
; NUMBER  (-/value/+, here 0..100 step 5)
;  ui:     (pu8 ID) (pu8 3) (pstr "Label")
;          (pi32 0) (pi32 100000) (pi32 5000) (pi32 (* myvar 1000)) (pstr "")
;          ;        ^min*1000     ^max*1000   ^step*1000 ^value*1000 ^unit
;  state:  (pu8 ID) (pi32 (* myvar 1000))
;  action: ((= cid ID) (setq myvar (to-i32 val)))
; LABEL   (read-only — no action branch)
;  ui:     (pu8 ID) (pu8 4) (pstr "Label") (pi32 (* (get-temp-mot) 1000)) (pstr "C")
;  state:  (pu8 ID) (pi32 (* (get-temp-mot) 1000))
```

The dispatch parameter is named `cid` by convention and the linter keys on it.

Choosing one of N — a profile, a mode — is N toggles over one variable: a radio
group with exactly one lit. There is no list control and no type for it.

```lisp
; panel-send-ui   (the count byte covers all three rows)
(pu8 10) (pu8 1) (pstr "Slow")   (pu8 (if (= current-profile 0) 1 0))
(pu8 11) (pu8 1) (pstr "Medium") (pu8 (if (= current-profile 1) 1 0))
(pu8 12) (pu8 1) (pstr "Fast")   (pu8 (if (= current-profile 2) 1 0))
; panel-send-state
(pu8 10) (pi32 (* (if (= current-profile 0) 1 0) 1000))
(pu8 11) (pi32 (* (if (= current-profile 1) 1 0) 1000))
(pu8 12) (pi32 (* (if (= current-profile 2) 1 0) 1000))
; panel-action — the row identifies the choice, so val is ignored
((= cid 10) (panel-set-profile 0))
((= cid 11) (panel-set-profile 1))
((= cid 12) (panel-set-profile 2))
```

Tapping the row that is already lit sends 0.0; the selector no-ops and the
STATE echo lights it straight back up. That is intended — do not "fix" it by
toggling the value off.

## React to a CAN button frame

```lisp
(def helper-btn-id 0x123)
(defun proc-helper-btn (data) {
    (let ((cmd (if (>= (buflen data) 2)
                   (bufget-u16 data 0)
                   (bufget-u8 data 0))))
        (cond
            ((= cmd 1) (panel-set-throttle (if (= throttle-on 1) 0 1)))
            ((= cmd 2) (switch-profile))
            (t (print (str-merge "unknown cmd " (to-str cmd))))))
})
```

## Slew-limit a value so the motor never steps

```lisp
(defun clampf (v lo hi) (if (< v lo) lo (if (> v hi) hi v)))
(defun slew (v target pos-s neg-s)
    (if (> target v)
        (clampf (+ v (/ ctl-dt pos-s)) 0.0 target)
        (clampf (- v (/ ctl-dt neg-s)) target 1.0)))
```

## Beep without stalling the caller

`foc-play-tone` + `sleep` inside an event handler would block that handler
thread, so give the sequence its own.

```lisp
(defun two-beeps () {
    (foc-play-tone 0 800 beep-vol) (sleep 0.1) (foc-play-stop) (sleep 0.06)
    (foc-play-tone 0 900 beep-vol) (sleep 0.1) (foc-play-stop)
})
(defun panel-beep () (spawn 150 two-beeps))
```

## Read a limit and act on a fraction of it

```lisp
(let ((imax (* (conf-get 'l-current-max) (conf-get 'l-current-max-scale))))
    (set-current (clampf wanted 0.0 imax) 0.2))
```
''';
