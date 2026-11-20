; =============================================================================
;  main.adc-pas.lisp — ALTERNATIVE to main.lisp: the stock ADC app keeps the
;  throttle/brake, LISP only injects pedal-assist current when the rider is
;  NOT using them. Experimental — nothing flashes this file automatically.
; =============================================================================
;
;  WHY THIS SHAPE
;  The current main.lisp makes LISP the single motor arbiter: it reads the ADC
;  pots, applies its own curve and slew, and suppresses the stock app with a
;  rolling (app-disable-output 1500). That works, but LISP then owns the whole
;  throttle feel and every future feature has to be wedged into the arbiter.
;
;  Here the motor is owned by ONE of four parties at a time (motor-owner):
;
;    0  the stock ADC app        - default; throttle/brake are firmware-native
;    1  cruise (firmware speed PID) - only while cruise-active
;    2  pedal assist (LISP)      - only while the rider is not on throttle/brake
;    3  panel master switch off  - holds zero until the master is back on
;
;  PAS (2) is a strict subset of "rider is idle": throttle released, brake
;  released,
;  master on, cruise off, setpoint fresh. The moment any of those breaks, the
;  injected current is cut and the motor is handed straight back to the ADC app
;  within one tick (20 ms), so the rider never fights us for control.
;
;  FAIL-SAFE
;  Suppression is a ROLLING 200 ms window armed every tick, never -1. If this
;  script dies while owning the motor, the stock app is back in <= 200 ms:
;    - died during PAS  -> throttle/brake respond again, assist just stops;
;    - died with master off -> the bike becomes rideable again (deliberate);
;    - died during cruise -> stock app returns; speed PID is no longer asked
;      for a target, so the motor coasts on the command timeout.
;
;  WHAT IS DELIBERATELY GONE (vs main.lisp)
;    armed / out-rel / brk-rel / throttle-out / brake-out / monitor-traction —
;    no throttle curve, no brake slew, no second set-current caller.
;    Profiles still scale l-current-max (l-current-max-scale) instead of
;    overwriting the VESC Tool value, and that scale is what PAS is clamped to.
;    Cruise is the FIRMWARE speed PID again (set-rpm), not the LISP PI: this is
;    the pre-PAS behaviour, so engaging has the old "sag then pull" character
;    instead of the LISP bumpless transfer.
;
;  FIRMWARE FACTS THIS SCRIPT IS BUILT ON (vedderb/bldc)
;    apps/adc: `decoded_level = pwr` is stored BEFORE deadband/curve/ramping
;      and BEFORE the `app_is_output_disabled()` check, so get-adc-decoded is
;      always live and always the raw pot, never the ramped output.
;    apps/adc: while the output is disabled the thread `continue`s BEFORE the
;      ramp block, so pwr_ramp and last_time freeze. On hand-back the stale
;      last_time makes ramp_step enormous and utils_step_towards jumps to the
;      target in ONE cycle — i.e. the stock app has NO ramp on its first cycle
;      after a suppression. Hence: we only take the motor while the stock
;      output is already ~0 (pas-settle), and we never hand it back with a
;      throttle held (lazy master re-enable).
;    app.c: app_disable_output(0) re-enables now, (-1) disables forever, (ms)
;      disables for ms. We only ever use the (ms) form, refreshed every tick.
;    mc_interface/mcpwm_foc: set-rpm enters CONTROL_MODE_SPEED; with
;      s_pid_ramp_erpms_s > 0 (default 25000) the setpoint is seeded from the
;      CURRENT rpm and the command ramps to the target, so the rpm setpoint
;      itself engages bumplessly. The speed PI output starts from its stored
;      integrator, which is why the current sags briefly on the first engage.
;
;  NOT CHANGED
;    The 'VP' panel protocol (UI_DESC/STATE/DASH/ACTION/PAS/helper buttons),
;    beep volume in EEPROM — byte-identical to main.lisp.

(def cruise-active 0)
(def cruise-rpm 0)
; Raw decoded pot levels from the previous tick, for the cruise-cancel edge
; detector. The old script cancelled cruise on a RISING edge of the throttle
; (so you can engage it while holding the throttle at speed) and on level for
; the brake (safety: the stock brake is suppressed while we own the motor).
(def old-thr 0.0)
(def old-brk 0.0)
(def rx-button-state 1)
(def tx-button-state 1)
(def current-profile 0)
(def num-profiles 3)
(def first-profile-init 1)
(def rpm-per-ms 0.0)
(def throttle-on 1)
; Panel throttle master. 0 = hold the motor at zero (owner 3) until switched
; back on AND the throttle has been seen released once.
(def pbuf (bufcreate 128))
(def pi 0)
(def beep-vol-addr 0)
(def beep-vol (let ((v (eeprom-read-i beep-vol-addr)))
                (if (and v (>= v 0) (<= v 50)) v 30)))
(def beep-vol-dirty 0)
; Pedal-assist setpoint from the head unit / BLE helper (COMM_CUSTOM_APP_DATA,
; msg 0x05): pas-amps is the requested motor current (A), pas-seen the (systime)
; of the last accepted frame, pas-src the CAN id of the sender we are locked
; onto (-1 = none) — see the 0x05 handler for why the lock exists.
(def pas-amps 0.0)
(def pas-seen 0)
(def pas-src -1)
; Injection state. pas-out is the SLEWED absolute current actually commanded,
; so a fading setpoint ramps out instead of stepping to 0.
(def pas-out 0.0)
; Who commands the motor right now: 0 ADC app, 1 cruise, 2 PAS, 3 master-off.
(def motor-owner 0)
; Supervisor tick — 100 Hz for the same reason the arbiter in main.lisp runs at
; 100 Hz: at 20 Hz a ramp advances in 12.5%-of-full-scale steps, and the FOC
; loop executes each step instantly, so it feels like a staircase rather than a
; ramp. Cheap here: one ADC read plus a couple of conf-get per tick.
(def ctl-dt 0.01)
(def own-window 200)      ; ms of rolling output suppression per tick

; @const-start flashes every definition below, freeing the cons heap. Without it
; all the defun bodies live in RAM and exhaust the heap — panel-event-loop then
; OOMs at runtime and the display goes blank while motor control keeps running.
; Everything mutable MUST stay above this line: setq'd scalars, and especially
; the pbuf buffer (a flashed buffer is read-only, bufset would fail/crash).
@const-start

(defun play-stop () {
    (sleep 0.1)
    (foc-play-stop)
})

; Profiles scale the current limit instead of overwriting it: Motor Current Max
; in VESC Tool stays the master value (applied live, no LISP restart) and each
; profile is a fraction of it. Braking is never scaled — always full.
(defun apply-profile (profile-index) {
    (if (= profile-index 0) {
        (conf-set 'max-speed (/ 25.0 3.6))
        (conf-set 'l-current-max-scale 0.5)
        (print "Profile 0: Slow (25 km/h, 50% current)")
    } {
        (if (= profile-index 1) {
            (conf-set 'max-speed (/ 40.0 3.6))
            (conf-set 'l-current-max-scale 0.67)
            (print "Profile 1: Medium (40 km/h, 67% current)")
        } {
            (if (= profile-index 2) {
                (conf-set 'max-speed (/ 60.0 3.6))
                (conf-set 'l-current-max-scale 1.0)
                (print "Profile 2: Fast (60 km/h, 100% current)")
            })
        })
    })
    (if (= first-profile-init 0) {
        (let ((beep-freq (if (= profile-index 0) {
            500
        } {
            (if (= profile-index 1) {
                750
            } {
                1000
            })
        }))) {
            (foc-play-tone 0 beep-freq beep-vol)
            (spawn 150 play-stop)
        })
    } {
        (setq first-profile-init 0)
    })
})

(gpio-configure 'pin-rx 'pin-mode-in-pu)
(gpio-configure 'pin-tx 'pin-mode-in-pu)

(defun update-rpm-per-ms () {
    (loopwhile t {
        (if (= cruise-active 0) {
            (let ((current-rpm (get-rpm))) {
                (let ((current-speed-ms (get-speed))) {
                    (if (and (> (abs current-rpm) 10) (> (abs current-speed-ms) 0.1)) {
                        (setq rpm-per-ms (/ (abs current-rpm) (abs current-speed-ms)))
                    })
                })
            })
        })
        (sleep 0.2)
    })
})

; -----------------------------------------------------------------------------
;  Motor ownership helpers
; -----------------------------------------------------------------------------

; Take the motor: arm a fresh suppression window. Refreshed every tick while we
; own it, so a dead script gives the stock app back within own-window ms.
(defun own-motor (who) {
    (setq motor-owner who)
    (app-disable-output own-window)
})

; Hand the motor back: command zero while still suppressed, then release the
; suppression so the stock ADC app resumes from a known-zero state.
(defun release-motor () {
    (set-current 0)
    (app-disable-output 0)
    (setq motor-owner 0)
})

(defun clampf (v lo hi) (if (< v lo) lo (if (> v hi) hi v)))

; Live full-scale current for the active profile: VESC Tool Motor Current Max
; (master value, never written by us) x the profile scale. Read fresh each use
; so a VESC Tool write or a profile switch applies without restarting anything.
(defun cur-max () {
    (let ((v (conf-get 'l-current-max))) (if v v 60.0))
})
(defun cur-scale () {
    (let ((v (conf-get 'l-current-max-scale))) (if v v 1.0))
})
(defun motor-limit () {
    (let ((m (cur-max)) (s (cur-scale)))
        (if (and (> m 0.0) (> s 0.0)) (* m s) 0.0))
})

; Ramping times come live from the VESC Tool ADC app page — the same knobs that
; shape the stock throttle — so PAS ramps feel like the throttle and tuning
; stays in VESC Tool. Clamped away from 0 so a zero in the config can't divide
; by zero and kill the supervisor thread.
(defun ramp-pos () {
    (let ((v (conf-get 'adc-ramp-time-pos))) (clampf (if v v 0.3) 0.05 5.0))
})
(defun ramp-neg () {
    (let ((v (conf-get 'adc-ramp-time-neg))) (clampf (if v v 0.3) 0.05 5.0))
})

; Slew an ABSOLUTE current (A) toward target: a full-scale (limit) step takes
; pos-s when rising and neg-s when falling. limit <= 0 (no limit known) freezes
; the output instead of dividing by zero.
(defun slew-amps (v target pos-s neg-s limit) {
    (if (<= limit 0.0)
        0.0
        (if (> target v)
            (clampf (+ v (* limit (/ ctl-dt pos-s))) 0.0 target)
            (clampf (- v (* limit (/ ctl-dt neg-s))) target limit)))
})

; -----------------------------------------------------------------------------
;  Cruise — the FIRMWARE speed PID (set-rpm), as in the pre-PAS script: the
;  rider sets the speed with the throttle, presses RX to lock it, and the VESC
;  speed PI holds the ERPM. We only own the motor (suppress the ADC app) while
;  the PI drives it.
; -----------------------------------------------------------------------------
(defun activate-cruise-control () {
    (if (and (= cruise-active 0) (= throttle-on 1)) {
        (setq cruise-rpm (get-rpm))
        (if (> (abs cruise-rpm) 0) {
            (setq pas-out 0.0)             ; assist stops the moment cruise takes over
            (setq cruise-active 1)
            ; Take the motor and issue the setpoint in the SAME breath as the
            ; state flip: between the flip and the supervisor's next tick the
            ; ADC app would otherwise keep applying a held throttle.
            (own-motor 1)
            (set-rpm cruise-rpm)
            (print (str-merge "Cruise control activated at RPM: " (to-str cruise-rpm)))
        } {
            (print "Cannot activate cruise control: speed is zero")
        })
    })
})

; Drop the cruise STATE without touching motor ownership. Used where the caller
; is about to take ownership itself (master-off): going through
; deactivate-cruise-control would release the ADC app for a millisecond and let
; a held throttle through before the new owner re-arms the suppression.
(defun cruise-clear () {
    (setq cruise-active 0)
    (setq cruise-rpm 0)
    (setq rpm-per-ms 0.0)
})

(defun deactivate-cruise-control () {
    (if (= cruise-active 1) {
        (cruise-clear)
        (release-motor)
        (print "Cruise control deactivated")
    })
})

; Keep the firmware speed PID fed. Re-issuing set-rpm every tick also keeps the
; motor-command timeout alive (if this script dies, the timeout coasts the motor
; instead of holding the last setpoint).
(defun cruise-hold () {
    (own-motor 1)
    (set-rpm cruise-rpm)
})

(defun increase-cruise-speed () {
    (if (= cruise-active 1) {
        (if (> rpm-per-ms 0.0) {
            (let ((current-speed-ms (/ (abs cruise-rpm) rpm-per-ms))) {
                (let ((new-speed-ms (+ current-speed-ms (/ 1.0 3.6)))) {
                    (let ((new-rpm (* new-speed-ms rpm-per-ms))) {
                        (if (< cruise-rpm 0) {
                            (setq cruise-rpm (- new-rpm))
                        } {
                            (setq cruise-rpm new-rpm)
                        })
                        (print (str-merge "Cruise speed increased to RPM: " (to-str cruise-rpm)))
                    })
                })
            })
        } {
            (let ((rpm-increment 50)) {
                (if (< cruise-rpm 0) {
                    (setq cruise-rpm (- cruise-rpm rpm-increment))
                } {
                    (setq cruise-rpm (+ cruise-rpm rpm-increment))
                })
                (print (str-merge "Cruise speed increased to RPM: " (to-str cruise-rpm)))
            })
        })
    })
})

(defun decrease-cruise-speed () {
    (if (= cruise-active 1) {
        (if (> rpm-per-ms 0.0) {
            (let ((current-speed-ms (/ (abs cruise-rpm) rpm-per-ms))) {
                (let ((new-speed-ms (- current-speed-ms (/ 1.0 3.6)))) {
                    (if (> new-speed-ms 0.1) {
                        (let ((new-rpm (* new-speed-ms rpm-per-ms))) {
                            (if (< cruise-rpm 0) {
                                (setq cruise-rpm (- new-rpm))
                            } {
                                (setq cruise-rpm new-rpm)
                            })
                            (print (str-merge "Cruise speed decreased to RPM: " (to-str cruise-rpm)))
                        })
                    } {
                        (deactivate-cruise-control)
                        (print "Cruise control deactivated: speed too low")
                    })
                })
            })
        } {
            (deactivate-cruise-control)
            (print "Cruise control deactivated: no speed ratio available")
        })
    })
})

(defun monitor-rx-button () {
    (loopwhile t {
        (let ((current-button-state (gpio-read 'pin-rx))) {
            (if (and (= rx-button-state 1) (= current-button-state 0)) {
                (if (= cruise-active 1) {
                    (increase-cruise-speed)
                } {
                    (activate-cruise-control)
                })
            })
            (setq rx-button-state current-button-state)
        })
        (sleep 0.05)
    })
})

(defun monitor-tx-button () {
    (loopwhile t {
        (let ((current-button-state (gpio-read 'pin-tx))) {
            (if (and (= tx-button-state 1) (= current-button-state 0)) {
                (if (= cruise-active 1) {
                    (decrease-cruise-speed)
                } {
                    (switch-profile)
                })
            })
            (setq tx-button-state current-button-state)
        })
        (sleep 0.05)
    })
})

(defun switch-profile () {
    (setq current-profile (+ current-profile 1))
    (if (>= current-profile num-profiles) {
        (setq current-profile 0)
    })
    (apply-profile current-profile)
})

; Direct profile selection — what the on-screen panel buttons call (the TX pin
; button still cycles through them with switch-profile). Selecting the profile
; that is already active is a no-op: the panel's toggles are a radio group, so
; tapping the lit one just gets re-lit by the STATE echo instead of toggling off.
(defun panel-set-profile (idx) {
    (if (and (>= idx 0) (< idx num-profiles) (not (= idx current-profile))) {
        (setq current-profile idx)
        (apply-profile current-profile)
    })
})

; -----------------------------------------------------------------------------
;  THE supervisor — the only thing that decides who commands the motor. It does
;  NOT shape the throttle: whenever the rider touches throttle or brake, the
;  stock ADC app gets the motor back and drives it exactly as it does without
;  any script installed.
;
;  Priority: master-off > (hold until throttle released) > brake/throttle >
;  cruise > pedal assist > stock ADC app.
;
;  BRAKE — NO SMOOTHING IN THIS BUILD
;    Pressing the brake while we own the motor hands the motor straight back to
;    the stock app. Because a suppressed app has a frozen ramp, the stock brake
;    is then applied in one cycle: +drive -> full regen. That is the jerk we
;    tried to smooth and rolled back; the smoother variants broke cruise/PAS on
;    this controller, and a working bike beats a soft one. Known limitation #2:
;    with the panel master switch off the stock output is suppressed, so the
;    brake does not work until the switch is back on.
; -----------------------------------------------------------------------------
(defun pas-fresh () (and (> pas-amps 0.0) (< (secs-since pas-seen) 0.4)))

; Is the stock app done? Its output must be ~0 A before we take the motor,
; because a suppressed app freezes its ramp and jumps on hand-back (see the
; firmware notes at the top). Once we own the motor, skip the test — otherwise
; our own injected current would read as "still ramping" and cut itself.
(defun pas-settle () (or (= motor-owner 2) (< (abs (get-current)) 2.0)))

(defun motor-supervisor () {
    (loopwhile t {
        (let ((thr   (get-adc-decoded 0))
              (brake (get-adc-decoded 1))) {
            (let ((rider    (or (> thr 0.05) (> brake 0.05)))
                  (thr-edge (and (> thr 0.05) (<= old-thr 0.05)))
                  (brake-on (> brake 0.05))) {
                (cond
                    ; 1. Panel master off: hold the motor at zero.
                    ((= throttle-on 0) {
                        (setq pas-out 0.0)
                        (if (= cruise-active 1) (cruise-clear))
                        (own-motor 3)
                        (set-current 0)
                    })

                    ; 2. Master back on but the throttle is still held: keep
                    ;    holding. The ADC app's ramp state is frozen while
                    ;    suppressed, so its first cycle would apply the held
                    ;    throttle at full value in one step.
                    ((and (= motor-owner 3) (> thr 0.05)) {
                        (own-motor 3)
                        (set-current 0) })

                    ; 3. Cruise (firmware speed PID) owns the motor.
                    ((= cruise-active 1) {
                        (if (or brake-on thr-edge) {
                            (deactivate-cruise-control)
                            (setq pas-out 0.0)
                        } {
                            (cruise-hold)
                        }) })

                    ; 4. Nobody owns it: stock ADC app drives, PAS may add.
                    (t {
                        (let ((limit (motor-limit))) {
                            (if rider {
                                ; Rider takes over the throttle: cut instantly (no
                                ; ramp — the ADC app must see the pot at full
                                ; authority NOW) and give the motor back.
                                (setq pas-out 0.0)
                                (if (> motor-owner 0) (release-motor))
                            } {
                                ; Start injecting only once the stock output has
                                ; decayed: otherwise the takeover lands mid-ramp
                                ; and the hand-off dips (see pas-settle).
                                (let ((target (if (and (pas-fresh) (pas-settle))
                                                  (if (> pas-amps limit) limit pas-amps)
                                                  0.0))) {
                                    (setq pas-out (slew-amps pas-out target (ramp-pos) (ramp-neg) limit))
                                    (if (> pas-out 0.01) {
                                        (own-motor 2)
                                        ; Relative so the firmware's thermal
                                        ; derating and the live limit keep
                                        ; applying; the absolute form is the
                                        ; fallback when the limit is unknown.
                                        (if (> limit 1.0)
                                            (set-current-rel (clampf (/ pas-out limit) 0.0 1.0) 0.2)
                                            (set-current pas-out 0.2))
                                    } {
                                        ; Idle or fading out: once the ramp
                                        ; reaches zero, hand the bike back to the
                                        ; stock app.
                                        (if (> motor-owner 0) (release-motor))
                                        (setq pas-out 0.0)
                                    })
                                })
                            })
                        })
                    })
                )
                (setq old-thr thr)
                (setq old-brk brake)
            })
        })
        (sleep ctl-dt)
    })
})

; -----------------------------------------------------------------------------
;  Panel (protocol identical to main.lisp / vesc_lisp_panel.c)
; -----------------------------------------------------------------------------
(defun pu8  (v) { (bufset-u8  pbuf pi v) (setq pi (+ pi 1)) })
(defun pi32 (v) { (bufset-i32 pbuf pi (to-i32 v)) (setq pi (+ pi 4)) })
(defun pstr (s) { (bufcpy pbuf pi s 0 (buflen s)) (setq pi (+ pi (buflen s))) })

(defun panel-send-ui (reply-id) {
    (setq pi 0)
    (pu8 0x56) (pu8 0x50) (pu8 0x81) (pu8 1) (pu8 6)
    (pu8 1) (pu8 1) (pstr "Throttle") (pu8 (if (= throttle-on 1) 1 0))
    (pu8 4) (pu8 2) (pstr "Beep")
    (pu8 5) (pu8 3) (pstr "Beep Vol")
    (pi32 0) (pi32 50000) (pi32 5000) (pi32 (* beep-vol 1000)) (pstr "")
    ; Profile radio group (ids 10..12) — exactly one is lit, tapping a row
    ; selects that profile. Keep the labels in step with apply-profile.
    (pu8 10) (pu8 1) (pstr "Slow 25 km/h")   (pu8 (if (= current-profile 0) 1 0))
    (pu8 11) (pu8 1) (pstr "Medium 40 km/h") (pu8 (if (= current-profile 1) 1 0))
    (pu8 12) (pu8 1) (pstr "Fast 60 km/h")   (pu8 (if (= current-profile 2) 1 0))
    ; reply-id 255 means the request did NOT come in over CAN — it is a head
    ; unit talking through a VESC Express BLE adapter, which has no CAN id of
    ; its own. Answer on the interface the request arrived on and the reply
    ; rides back up that adapter's BLE link.
    (if (= reply-id 255) (send-data pbuf) (send-data pbuf 2 reply-id))
})

(defun panel-send-state (reply-id) {
    (setq pi 0)
    (pu8 0x56) (pu8 0x50) (pu8 0x82) (pu8 5)
    (pu8 1) (pi32 (* (if (= throttle-on 1) 1 0) 1000))
    (pu8 5) (pi32 (* beep-vol 1000))
    (pu8 10) (pi32 (* (if (= current-profile 0) 1 0) 1000))
    (pu8 11) (pi32 (* (if (= current-profile 1) 1 0) 1000))
    (pu8 12) (pi32 (* (if (= current-profile 2) 1 0) 1000))
    (if (= reply-id 255) (send-data pbuf) (send-data pbuf 2 reply-id))
})

(defun panel-send-dash (reply-id) {
    (setq pi 0)
    (pu8 0x56) (pu8 0x50) (pu8 0x84)
    (pi32 (* cruise-active 1000))
    (pi32 (* cruise-rpm 1000))
    (pi32 (* current-profile 1000))
    (pi32 (* rpm-per-ms 1000.0))
    (if (= reply-id 255) (send-data pbuf) (send-data pbuf 2 reply-id))
})

; Master enable. With the ADC app owning the motor, "off" cannot be just a flag:
; the supervisor has to keep the stock output suppressed. Arm that here so the
; switch takes effect immediately instead of on the next tick.
(defun panel-set-throttle (on) {
    (if (= on 0) {
        (setq throttle-on 0)
        (setq pas-out 0.0)
        ; cruise-clear, not deactivate-cruise-control: never hand the motor to
        ; the ADC app on the way to suppressing it (see cruise-clear).
        (if (= cruise-active 1) (cruise-clear))
        (own-motor 3)
        (set-current 0)
    } {
        ; Re-enabling is lazy on purpose: the supervisor hands the motor back
        ; only after the throttle has been seen released (see motor-supervisor).
        (setq throttle-on 1)
    })
})

(defun two-beeps () {
    (foc-play-tone 0 800 beep-vol)
    (sleep 0.1)
    (foc-play-stop)
    (sleep 0.06)
    (foc-play-tone 0 900 beep-vol)
    (sleep 0.1)
    (foc-play-stop)
    (sleep 0.3)
    (foc-play-tone 0 800 beep-vol)
    (sleep 0.1)
    (foc-play-stop)
    (sleep 0.06)
    (foc-play-tone 0 900 beep-vol)
    (sleep 0.1)
    (foc-play-stop)
})

(defun panel-beep () (spawn 150 two-beeps))

(defun panel-action (cid val) {
    (cond
        ((= cid 1) (panel-set-throttle (if (> val 0.5) 1 0)))
        ((= cid 4) (panel-beep))
        ((= cid 5) {
            (setq beep-vol (to-i32 (clampf val 0.0 50.0)))
            (setq beep-vol-dirty 1)
        })
        ; Profile radio group: the row identifies the profile, so val is ignored
        ; (tapping the already-lit row sends 0 and panel-set-profile no-ops).
        ((= cid 10) (panel-set-profile 0))
        ((= cid 11) (panel-set-profile 1))
        ((= cid 12) (panel-set-profile 2)))
})

(defun panel-handle (data) {
    (if (and (>= (buflen data) 4)
             (= (bufget-u8 data 0) 0x56)
             (= (bufget-u8 data 1) 0x50))
        (let ((msg (bufget-u8 data 2))
              (reply-id (bufget-u8 data 3))) {
            (cond
                ((= msg 0x01) (panel-send-ui reply-id))
                ((= msg 0x03) (panel-send-state reply-id))
                ((= msg 0x04) (panel-send-dash reply-id))
                ((and (= msg 0x05) (>= (buflen data) 8)) {
                    ; Pedal-assist setpoint (fire-and-forget, no reply). i32 mA at
                    ; byte 4 (after magic[0,1], msg[2], reply-id[3]).
                    ;
                    ; SOURCE LOCK, same rule as main.lisp: more than one node can
                    ; stream setpoints (the P4 display's on-device PAS idles at
                    ; 0 A, 20 Hz, forever on firmware older than 1.3.1). With two
                    ; senders the target would flip 3→0→3 A every 50 ms and the
                    ; injected current would vibrate at 10 Hz. So: lock onto
                    ; whoever sent the last NON-ZERO setpoint and ignore other
                    ; senders until the locked one goes silent/zero (0.4 s).
                    (let ((amps (/ (bufget-i32 data 4) 1000.0))) {
                        (if (or (= pas-src reply-id)
                                (= pas-src -1)
                                (> (secs-since pas-seen) 0.4))
                            {
                                (setq pas-amps amps)
                                (setq pas-seen (systime))
                                (setq pas-src (if (> amps 0.0) reply-id -1))
                            }
                            nil)
                    })
                })
                ((= msg 0x06) {
                    ; Atomic throttle toggle from the BLE helper: flip our own
                    ; state and answer with fresh STATE.
                    (panel-set-throttle (if (= throttle-on 1) 0 1))
                    (panel-send-state reply-id)
                })
                ((and (= msg 0x02) (>= (buflen data) 9))
                    (let ((cid (bufget-u8 data 4))
                          (val (/ (bufget-i32 data 5) 1000.0))) {
                        (panel-action cid val)
                        (panel-send-state reply-id)
                    })))
        }))
})

(defun panel-on-shutdown () {
    (if (= beep-vol-dirty 1) {
        (shutdown-hold t)
        (eeprom-store-i beep-vol-addr beep-vol)
        (setq beep-vol-dirty 0)
        (shutdown-hold nil)
    })
})

; Beep volume changes via the panel slider (many intermediate values per drag)
; and must survive a reboot. Persisting only in panel-on-shutdown is unreliable:
; event-shutdown fires only on a real power-off, not on a bench / USB / re-flash
; reboot. Flush the dirty volume on a slow timer instead — coalesces a drag into
; ~one flash write and does not depend on a clean shutdown.
(defun persist-volumes-loop () {
    (loopwhile t {
        (if (= beep-vol-dirty 1) {
            (eeprom-store-i beep-vol-addr beep-vol) (setq beep-vol-dirty 0) })
        (sleep 2)
    })
})

; Custom button frames from the BLE helper: plain standard-id CAN frames, id/data
; configured per button in the helper GUI. Command = the data bytes read as a
; big-endian u16 (single-byte frames work too):
;   CAN ID 0x123, data 00 01 -> toggle the throttle master switch
;   CAN ID 0x123, data 00 02 -> switch speed profile
(def helper-btn-id 0x123)

(defun proc-helper-btn (data) {
    (let ((cmd (if (>= (buflen data) 2)
                   (bufget-u16 data 0)
                   (bufget-u8 data 0)))) {
        (cond
            ((= cmd 1) {
                (panel-set-throttle (if (= throttle-on 1) 0 1))
                (print (str-merge "helper: throttle "
                                  (if (= throttle-on 1) "ON" "off")))
            })
            ((= cmd 2) {
                (switch-profile)
                (print (str-merge "helper: profile "
                                  (to-str current-profile)))
            })
            (t (print (str-merge "helper cmd " (to-str cmd)))))
    })
})

(defun panel-event-loop () {
    (loopwhile t {
        (recv ((event-data-rx . (? data)) (panel-handle data))
              ((event-can-sid (? id) . (? data))
                  (if (= id helper-btn-id) (proc-helper-btn data)))
              (event-shutdown               (panel-on-shutdown))
              (_ nil))
    })
})

; Spawn threads and enable events LAST — after every function they reach is
; bound. The supervisor is spawned last of all so its first tick sees finished
; state. These are plain expressions (not definitions), so @const-start does not
; flash them.
(apply-profile 0)
(print "adc-pas v5: ADC owns throttle, PAS injects, brake fade 0.35 s")
(event-register-handler (spawn 300 panel-event-loop))
(event-enable 'event-data-rx)
(event-enable 'event-shutdown)
(spawn 150 update-rpm-per-ms)
(spawn 150 monitor-rx-button)
(spawn 150 monitor-tx-button)
(spawn 150 persist-volumes-loop)
; 50 Hz supervisor: deeper nesting than the old monitors (cond -> let -> let ->
; slew-amps -> clampf), so give it more stack than the 150 used elsewhere.
(spawn 300 motor-supervisor)

@const-end
