; =============================================================================
; main.adc-pas.lisp v8 - continuous ADC1 override, VESC firmware 7.00.
; Only virtual throttle is commanded. ADC2/braking, curves, ramps and motor
; limits remain in the stock ADC app. No set-current, set-rpm or app suppression.
;
; Priority: brake > master/arming/fault inhibit > physical throttle > cruise > PAS.
; A physical throttle overrides cruise without cancelling it. Brake cancels it.
; PAS is accepted only with the physical throttle released, and expires in 0.4 s.
; Cruise is a local speed PID whose output is a positive current fraction,
; converted through the inverse ADC curve/deadband into virtual throttle volts.
;
; REQUIRED: APP_ADC or APP_ADC_UART, Current No Reverse Brake ADC (type 8).
; Disable native ADC button functions; RX/TX are handled by this script.
; adc1-inverted / adc-curve-accel / adc-curve-mode below MUST match VESC Tool.
; FW 7.00 does not expose those three ADC settings through conf-get.
; Calibration endpoints, fault range, deadband and current limits are read live.
;
; Detached ADC relies on the GLOBAL motor timeout: no override -> timeout stop,
; NOT automatic restoration of the physical throttle. Another command source
; resetting that timeout can keep stale override alive; do not run another motor
; command/timeout-reset source. Test timeout on the actual controller.
; The PID defaults are starting values, not validated gains for this bicycle.
; =============================================================================

(def cruise-active 0)
(def cruise-rpm 0.0)
(def cruise-i 0.0)
(def cruise-d 0.0)
(def cruise-last-speed 0.0)
; PID output is a 0..1 current fraction, error in m/s, time in seconds.
(def cruise-kp 0.06)
(def cruise-ki 0.03)
(def cruise-kd 0.01)
(def cruise-d-tau 0.2)
(def adc1-inverted 0)
(def adc-curve-accel 0.0)
(def adc-curve-mode 0)
(def input-threshold 0.02)
(def brake-threshold 0.001)
(def throttle-armed 0)
(def adc1-command 0.0)
(def ctl-dt 0.01)
(def ctl-time 0)
(def rx-button-state 1)
(def tx-button-state 1)
(def base-current-max 100.0) ; motor amperes, before profile scaling
(def current-profile 0)
(def num-profiles 3)
(def first-profile-init 1)
(def rpm-per-ms 0.0)
(def throttle-on 1)
(def pbuf (bufcreate 128))
(def pi 0)
(def beep-vol-addr 0)
(def beep-vol (let ((v (eeprom-read-i beep-vol-addr)))
                (if (and v (>= v 0) (<= v 50)) v 30)))
(def beep-vol-dirty 0)
; External PAS protocol remains motor amperes; source lock is unchanged.
(def pas-amps 0.0)
(def pas-seen 0)
(def pas-src -1)

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
; ADC calibration and inverse mapping. Manual input bypasses this mapping and
; passes physical voltage unchanged. Only PAS/PID need inverse curve conversion.
; -----------------------------------------------------------------------------
(defun clampf (v lo hi) (if (< v lo) lo (if (> v hi) hi v)))

(defun motor-limit ()
    (* (conf-get 'l-current-max) (conf-get 'l-current-max-scale)))

(defun adc-config-ok () {
    (let ((app (conf-get 'app-to-use))
          (v0 (conf-get 'adc-v1-start)) (v1 (conf-get 'adc-v1-end))) {
        (and (or (= app 2) (= app 5)) (= (conf-get 'adc-ctrl-type) 8)
             (>= v0 0.0) (<= v0 3.3) (>= v1 0.0) (<= v1 3.3)
             (> (abs (- v1 v0)) 0.05)
             (> (conf-get 'adc-v1-max) (conf-get 'adc-v1-min))
             (>= v0 (conf-get 'adc-v1-min))
             (<= v0 (conf-get 'adc-v1-max))
             (>= v1 (conf-get 'adc-v1-min))
             (<= v1 (conf-get 'adc-v1-max))
             (>= (conf-get 'adc-thr-hyst) 0.0)
             (< (conf-get 'adc-thr-hyst) 0.95)
             (> (conf-get 'timeout-msec) 0))
    })
})

(defun adc1-volts (level) {
    (let ((v0 (conf-get 'adc-v1-start)) (v1 (conf-get 'adc-v1-end))) {
        (+ v0 (* (- v1 v0) (if (= adc1-inverted 1) (- 1.0 level) level)))
    })
})

(defun physical-throttle (volts) {
    (let ((v0 (conf-get 'adc-v1-start)) (v1 (conf-get 'adc-v1-end))) {
        (let ((level (clampf (/ (- volts v0) (- v1 v0)) 0.0 1.0)))
            (if (= adc1-inverted 1) (- 1.0 level) level))
    })
})

(defun throttle-released (level)
    (<= level (if (> (conf-get 'adc-thr-hyst) input-threshold)
                   (conf-get 'adc-thr-hyst) input-threshold)))

; Binary inversion uses the firmware's own curve primitive, including all three
; modes and negative curve constants. At zero use genuine neutral, not deadband.
(defun current-to-volts (fraction) {
    (if (<= fraction 0.0) (adc1-volts 0.0) {
        (let ((lo 0.0) (hi 1.0) (n 0)
              (target (clampf fraction 0.0 1.0))) {
            (loopwhile (< n 16) {
                (let ((mid (* 0.5 (+ lo hi)))) {
                    (if (< (throttle-curve mid adc-curve-accel 0.0 adc-curve-mode) target)
                        (setq lo mid) (setq hi mid))
                })
                (setq n (+ n 1))
            })
            (let ((h (conf-get 'adc-thr-hyst)))
                (adc1-volts (+ h (* (- 1.0 h) (* 0.5 (+ lo hi))))))
        })
    })
})

; PID derivative is on measured speed (no setpoint kick), low-pass filtered.
; Conditional integration prevents windup at 0 and 1. While the physical
; throttle wins, track measured traction so release does not restore an old
; saturated integrator. Firmware ADC ramping is always active downstream.
(defun cruise-output (dt manual) {
    (let ((speed (get-speed))
          (target (/ cruise-rpm rpm-per-ms))) {
        (let ((error (- target speed))) {
            (setq cruise-d (+ cruise-d
                (* (/ dt (+ cruise-d-tau dt))
                   (- (/ (- speed cruise-last-speed) dt) cruise-d))))
            (setq cruise-last-speed speed)
            (let ((pd (- (* cruise-kp error) (* cruise-kd cruise-d)))) {
                (if manual {
                    (let ((limit (motor-limit))) {
                        (setq cruise-i (clampf (- (if (> limit 0.0)
                            (clampf (/ (get-current) limit) 0.0 1.0) 0.0) pd) 0.0 1.0))
                    })
                } {
                    (let ((candidate (clampf (+ cruise-i (* cruise-ki error dt)) 0.0 1.0))) {
                        (if (or (and (>= (+ pd candidate) 0.0) (<= (+ pd candidate) 1.0))
                                (and (> (+ pd candidate) 1.0) (< error 0.0))
                                (and (< (+ pd candidate) 0.0) (> error 0.0)))
                            (setq cruise-i candidate))
                    })
                })
                (clampf (+ pd cruise-i) 0.0 1.0)
            })
        })
    })
})

(defun activate-cruise-control () {
    (if (and (= cruise-active 0) (= throttle-on 1) (= throttle-armed 1)
             (= (get-fault) 0)
             (adc-config-ok) (> rpm-per-ms 0.0) (> (get-speed) 0.5)
             (<= (get-adc-decoded 1) brake-threshold)
             (not (app-is-output-disabled))) {
        (setq cruise-rpm (* (get-speed) rpm-per-ms))
        (setq cruise-last-speed (get-speed))
        (setq cruise-d 0.0)
        ; Seed from actual traction after the ADC ramp, rather than a raw held
        ; throttle that may still be far above actual current during acceleration.
        (let ((limit (motor-limit)))
            (setq cruise-i (if (> limit 0.0)
                (clampf (/ (get-current) limit) 0.0 1.0) 0.0)))
        (setq cruise-active 1)
        (print (str-merge "Cruise PID activated at RPM: " (to-str cruise-rpm)))
    })
})

(defun cruise-clear () {
    (setq cruise-active 0)
    (setq cruise-rpm 0.0)
    (setq cruise-i 0.0)
    (setq cruise-d 0.0)
})

(defun deactivate-cruise-control () (cruise-clear))

(defun increase-cruise-speed () {
    (if (and (= cruise-active 1) (> rpm-per-ms 0.0))
        (setq cruise-rpm (* rpm-per-ms
            (clampf (+ (/ cruise-rpm rpm-per-ms) (/ 1.0 3.6)) 0.5
                    (conf-get 'max-speed)))))
})

(defun decrease-cruise-speed () {
    (if (and (= cruise-active 1) (> rpm-per-ms 0.0))
        (setq cruise-rpm (* rpm-per-ms
            (clampf (- (/ cruise-rpm rpm-per-ms) (/ 1.0 3.6)) 0.5 100.0))))
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
; Only writer of virtual ADC1 during normal operation. No ADC2 override or
; direct motor command anywhere. Brake neutralizes ADC1 BEFORE choosing a
; source, so full throttle cannot cancel full brake in firmware's pwr -= brake.
; -----------------------------------------------------------------------------
(defun pas-fresh () (and (> pas-amps 0.0) (< (secs-since pas-seen) 0.4)))

(defun motor-tick (dt) {
    (if (adc-config-ok) {
        (let ((raw (get-adc 0)) (brake (get-adc-decoded 1))) {
            (let ((thr (physical-throttle raw))
                  (valid (and (>= raw (conf-get 'adc-v1-min))
                              (<= raw (conf-get 'adc-v1-max))))) {
                (setq adc1-command (adc1-volts 0.0))
                (cond
                    ((> brake brake-threshold) {
                        (cruise-clear)
                        (setq pas-amps 0.0)
                        ; Re-arm after release of BOTH controls; PAS also needs
                        ; a newly received setpoint after braking ends.
                        (setq throttle-armed 0)
                    })
                    ((or (not valid) (= throttle-on 0)
                         (app-is-output-disabled) (not (= (get-fault) 0)) (> dt 0.2)) {
                        (cruise-clear)
                        (setq throttle-armed 0)
                        (setq pas-amps 0.0)
                    })
                    ((= throttle-armed 0) {
                        (if (throttle-released thr) (setq throttle-armed 1))
                    })
                    (t {
                        (let ((manual (not (throttle-released thr))) (request 0.0)) {
                            (if (= cruise-active 1) {
                                (if (> rpm-per-ms 0.0)
                                    (setq request (cruise-output (clampf dt 0.001 0.05) manual))
                                    (cruise-clear))
                            })
                            (cond
                                (manual (setq adc1-command raw))
                                ((= cruise-active 1)
                                    (setq adc1-command (current-to-volts request)))
                                ((pas-fresh) {
                                    (let ((limit (motor-limit))) {
                                        (if (> limit 0.0)
                                            (setq adc1-command (current-to-volts
                                                (clampf (/ pas-amps limit) 0.0 1.0))))
                                    })
                                })
                            )
                        })
                    })
                )
                (app-adc-override 0 adc1-command)
            })
        })
    } {
        ; No keepalive when the live app/calibration changes incompatibly.
        ; Detached ADC will stop on the configured global timeout.
        (cruise-clear)
        (setq throttle-armed 0)
    })
})

(defun motor-supervisor () {
    (setq ctl-time (systime))
    (loopwhile t {
        (let ((dt (secs-since ctl-time))) {
            (setq ctl-time (systime))
            (motor-tick dt)
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

; Master disables traction only. Native braking remains available. The loop
; owns ADC1 writes; turning back on requires a released physical throttle.
(defun panel-set-throttle (on) {
    (setq throttle-on on)
    (setq throttle-armed 0)
    (setq pas-amps 0.0)
    (cruise-clear)
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
(conf-set 'l-current-max base-current-max)
(apply-profile 0)
; Seed neutral BEFORE detaching. State 2 detaches ADC1 only; ADC2 stays live.
; Mode 3 also detaches native buttons, preventing their cruise/reverse actions
; from competing with the custom RX/TX button handlers. Neither ADC2 nor its
; calibration, filtering, curve or ramp is changed.
(if (adc-config-ok) {
    (setq adc1-command (adc1-volts 0.0))
    (app-adc-override 0 adc1-command)
    (app-adc-detach 3 2)
    (app-adc-override 2 0) ; reverse button released
    (app-adc-override 3 0) ; native cruise button released
    (print "adc-pas v8: ADC1 override, manual > cruise PID > PAS; native brake")
} {
    (print "ADC1 override NOT started: select ADC/ADC+UART, Current No Reverse Brake ADC; check calibration")
})
(event-register-handler (spawn 300 panel-event-loop))
(event-enable 'event-data-rx)
(event-enable 'event-shutdown)
(spawn 150 update-rpm-per-ms)
(spawn 150 monitor-rx-button)
(spawn 150 monitor-tx-button)
(spawn 150 persist-volumes-loop)
; Do not start the override loop unless initialization actually detached ADC1.
(if (adc-config-ok) (spawn 500 motor-supervisor))

@const-end
