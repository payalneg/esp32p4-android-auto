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

* The head unit talks to the script over `COMM_CUSTOM_APP_DATA`: payload starts
  with magic `0x56 0x50` ('V' 'P'), then a 1-byte message id, then a reply-id
  (the sender's CAN id). Floats on the wire are `int32 = round(value * 1000)`,
  big-endian. Strings are NUL-terminated and `buflen` counts the terminator.
* Replies are built in `pbuf` at index `pi` with `(pu8 v)`, `(pi32 v)`,
  `(pstr s)`, then sent with `(send-data pbuf 2 reply-id)`.
* ONE loop commands the motor (`motor-control-loop`), with a fixed priority:
  master-off > brake > throttle > cruise > pedal assist > coast. Add a new
  source as a branch there; never call `set-current` from a second thread.

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
