; Cruise control implementation for RX button
; When RX button is pressed:
; 1. Read current speed
; 2. Switch to speed control mode
; 3. Set current speed
; 4. Disable cruise control when brake or throttle is pressed

; Cruise control state variables
(def cruise-active 0)
(def cruise-rpm 0)
(def rx-button-state 1)  ; Previous button state (1 = not pressed with pull-up)
(def tx-button-state 1)  ; Previous TX button state (1 = not pressed with pull-up)
(def old-throttle 0.0)  ; Previous throttle value to detect changes (ADC1 decoded 0.0-1.0)
(def old-brake 0.0)  ; Previous brake value to detect changes (ADC2 decoded 0.0-1.0)
(def app-output-disabled 0)  ; Track if we disabled app output
(def adc-detached 0)  ; Track if ADC is detached
(def current-profile 0)  ; Current profile index (0, 1, 2, ...)
(def num-profiles 3)  ; Number of profiles
(def first-profile-init 1)  ; Flag to skip beep on first initialization
(def rpm-per-ms 0.0)  ; RPM per m/s ratio (cached to avoid recalculation)

; ---- Quick-action panel state (rendered by the ESP32-P4 dashboard) ----
; The P4 opens a swipe-out drawer and asks this script to describe its controls
; over COMM_CUSTOM_APP_DATA; we answer with (send-data ... 2 reply-can-id).
; Protocol mirror of components/vesc_can/vesc_lisp_panel.h. See bottom of file.
(def throttle-on 1)   ; 1 = motor responds to the throttle, 0 = output disabled
(def tc-on 0)         ; 1 = traction-control slip limiter active
(def tc-sens 50.0)    ; traction-control sensitivity 0..100 (higher = earlier cut)
(def pbuf (bufcreate 96))  ; scratch buffer for panel replies (zero-filled)
(def pi 0)                 ; running write index into pbuf

; Function to stop tone playback after delay
(defun play-stop () {
    (sleep 0.1)
    (foc-play-stop)
})

; Profile configurations
; Profile 0: Slow (25 km/h)
; Profile 1: Medium (35 km/h)
; Profile 2: Fast (50 km/h)
(defun apply-profile (profile-index) {
    (if (= profile-index 0) {
        ; Profile 0: Slow
        (conf-set 'max-speed (/ 25.0 3.6))  ; 25 km/h in m/s
        (print "Profile 0: Slow (25 km/h)")
    } {
        (if (= profile-index 1) {
            ; Profile 1: Medium
            (conf-set 'max-speed (/ 40.0 3.6))  ; 35 km/h in m/s
            (print "Profile 1: Medium (35 km/h)")
        } {
            (if (= profile-index 2) {
                ; Profile 2: Fast
                (conf-set 'max-speed (/ 60.0 3.6))  ; 50 km/h in m/s
                (print "Profile 2: Fast (50 km/h)")
            })
        })
    })
    ; Play tone to indicate profile switch (skip on first initialization)
    ; Frequency depends on profile: Slow=400Hz, Medium=800Hz, Fast=1200Hz
    (if (= first-profile-init 0) {
        (let ((beep-freq (if (= profile-index 0) {
            500  ; Profile 0: Slow - low frequency
        } {
            (if (= profile-index 1) {
                750  ; Profile 1: Medium - medium frequency
            } {
                1000  ; Profile 2: Fast - high frequency
            })
        }))) {
            (foc-play-tone 0 beep-freq 10)
            (spawn 150 play-stop)
        })
    } {
        ; First initialization - skip beep
        (setq first-profile-init 0)
    })
})

; Configure RX pin as input with pull-up for button detection
(gpio-configure 'pin-rx 'pin-mode-in-pu)
; Configure TX pin as input with pull-up for profile switching button
(gpio-configure 'pin-tx 'pin-mode-in-pu)

; Function to calculate and update rpm-per-ms ratio
(defun update-rpm-per-ms () {
    (loopwhile t {
        ; Only calculate when moving (not in cruise control mode)
        (if (= cruise-active 0) {
            (let ((current-rpm (get-rpm))) {
                (let ((current-speed-ms (get-speed))) {
                    ; Calculate rpm-per-ms if both RPM and speed are valid
                    (if (and (> (abs current-rpm) 10) (> (abs current-speed-ms) 0.1)) {
                        (setq rpm-per-ms (/ (abs current-rpm) (abs current-speed-ms)))
                    })
                })
            })
        })
        (sleep 0.2)  ; Update every 200ms
    })
})

; Function to activate cruise control
(defun activate-cruise-control () {
    ; Don't engage cruise while the panel has the throttle disabled — they both
    ; drive app-disable-output, so honouring throttle-on keeps the two coherent.
    (if (and (= cruise-active 0) (= throttle-on 1)) {
        ; Read current RPM
        (setq cruise-rpm (get-rpm))
        ; Only activate if speed is above zero
        (if (> (abs cruise-rpm) 0) {
            ; Disable app output to allow set-rpm to work without conflicts
            ; -1 means disable forever (until we enable it back)
            (app-disable-output -1)
            (setq app-output-disabled 1)
            (setq cruise-active 1)
            (print (str-merge "Cruise control activated at RPM: " (to-str cruise-rpm)))
            ; Set RPM control mode - this switches to speed control mode
            (set-rpm cruise-rpm)
        } {
            (print "Cannot activate cruise control: speed is zero")
        })
    })
})

; Function to deactivate cruise control
(defun deactivate-cruise-control () {
    (if (= cruise-active 1) {
        (setq cruise-active 0)
        (setq cruise-rpm 0)
        (setq rpm-per-ms 0.0)
        ; Stop RPM control (set to 0 to release control)
        ;(set-rpm 0)
        ; Re-enable app output if we disabled it
        (if (= app-output-disabled 1) {

            ; Detach ADC to take control
            (if (= adc-detached 0) {
                (app-adc-detach 1 1)  ; Detach both ADC1 and ADC2
                (setq adc-detached 1)
            })
            ; Set ADC values to zero before re-enabling app output
            ;(app-adc-override 0 0.0)  ; ADC1 (throttle) = 0
            ;(app-adc-override 1 0.0)  ; ADC2 (brake) = 0
            ; Small delay to ensure values are set
            ;(set-kill-sw 1)
            ; Re-attach ADC to return control
            (app-adc-detach 1 0)  ; Attach ADC back
            (set-current 0) 
            (sleep 0.05)
            ;(set-kill-sw 0)
            ; Re-enable app output
            (app-disable-output 0)  ; 0 means enable now
            (setq app-output-disabled 0)
            ; Re-attach ADC to return control
            (app-adc-detach 1 0)  ; Attach ADC back
            (setq adc-detached 0)

        })
        (print "Cruise control deactivated")
    })
})

; Function to monitor throttle and brake, disable cruise control if used
(defun monitor-throttle-brake () {
    (loopwhile t {
        (if (= cruise-active 1) {
            ; Check throttle via ADC1 (channel 0)
            ; get-adc-decoded returns remapped value 0.0-1.0 according to ADC app configuration
            ; Note: ADC app must be running for this function to work
            (let ((throttle-value (get-adc-decoded 0))) {
                ; Only deactivate cruise control when throttle transitions from not pressed to pressed
                ; This prevents deactivation if throttle is already held when cruise is activated
                (if (and (> throttle-value 0.05) (<= old-throttle 0.05)) {
                    (deactivate-cruise-control)
                })
                (setq old-throttle throttle-value)
            })
            ; Check brake via ADC2 (channel 1)
            ; get-adc-decoded returns remapped value 0.0-1.0 according to ADC app configuration
            (let ((brake-value (get-adc-decoded 1))) {
                ; Only deactivate cruise control when brake transitions from not pressed to pressed
                ; This prevents deactivation if brake is already held when cruise is activated
                (if (and (> brake-value 0.05) (<= old-brake 0.05)) {
                    (deactivate-cruise-control)
                })
                (setq old-brake brake-value)
            })
            ; Maintain cruise control speed
            (if (= cruise-active 1) {
                (set-rpm cruise-rpm)
            })
        } {
            ; When cruise control is not active, still update old values to track state
            (let ((throttle-value (get-adc-decoded 0))) {
                (setq old-throttle throttle-value)
            })
            (let ((brake-value (get-adc-decoded 1))) {
                (setq old-brake brake-value)
            })
        })
        (sleep 0.05)  ; Check every 50ms
    })
})

; Function to increase cruise control speed by 1 km/h
(defun increase-cruise-speed () {
    (if (= cruise-active 1) {
        (if (> rpm-per-ms 0.0) {
            ; Calculate target speed from cruise-rpm (not actual speed)
            (let ((current-speed-ms (/ (abs cruise-rpm) rpm-per-ms))) {
                (let ((new-speed-ms (+ current-speed-ms (/ 1.0 3.6)))) {
                    (let ((new-rpm (* new-speed-ms rpm-per-ms))) {
                        ; Preserve direction (sign)
                        (if (< cruise-rpm 0) {
                            (setq cruise-rpm (- new-rpm))
                        } {
                            (setq cruise-rpm new-rpm)
                        })
                        (set-rpm cruise-rpm)
                        (print (str-merge "Cruise speed increased to RPM: " (to-str cruise-rpm)))
                    })
                })
            })
        } {
            ; Fallback: use small RPM increment if ratio not yet available
            (let ((rpm-increment 50)) {
                (if (< cruise-rpm 0) {
                    (setq cruise-rpm (- cruise-rpm rpm-increment))
                } {
                    (setq cruise-rpm (+ cruise-rpm rpm-increment))
                })
                (set-rpm cruise-rpm)
                (print (str-merge "Cruise speed increased to RPM: " (to-str cruise-rpm)))
            })
        })
    })
})

; Function to monitor RX button and handle cruise control
(defun monitor-rx-button () {
    (loopwhile t {
        (let ((current-button-state (gpio-read 'pin-rx))) {
            ; Detect button press (edge: 1 -> 0, button pulls pin low)
            (if (and (= rx-button-state 1) (= current-button-state 0)) {
                ; Button was just pressed
                (if (= cruise-active 1) {
                    ; If cruise control is active, increase speed by 1 km/h
                    (increase-cruise-speed)
                } {
                    ; If cruise control is not active, activate it
                    (activate-cruise-control)
                })
            })
            (setq rx-button-state current-button-state)
        })
        (sleep 0.05)  ; Check button every 50ms
    })
})

; Function to switch to next profile
(defun switch-profile () {
    ; Increment profile index
    (setq current-profile (+ current-profile 1))
    ; Wrap around if exceeds number of profiles
    (if (>= current-profile num-profiles) {
        (setq current-profile 0)
    })
    ; Apply the new profile
    (apply-profile current-profile)
})

; Function to decrease cruise control speed by 1 km/h
(defun decrease-cruise-speed () {
    (if (= cruise-active 1) {
        (if (> rpm-per-ms 0.0) {
            ; Calculate target speed from cruise-rpm (not actual speed)
            (let ((current-speed-ms (/ (abs cruise-rpm) rpm-per-ms))) {
                (let ((new-speed-ms (- current-speed-ms (/ 1.0 3.6)))) {
                    (if (> new-speed-ms 0.1) {
                        (let ((new-rpm (* new-speed-ms rpm-per-ms))) {
                            ; Preserve direction (sign)
                            (if (< cruise-rpm 0) {
                                (setq cruise-rpm (- new-rpm))
                            } {
                                (setq cruise-rpm new-rpm)
                            })
                            (set-rpm cruise-rpm)
                            (print (str-merge "Cruise speed decreased to RPM: " (to-str cruise-rpm)))
                        })
                    } {
                        ; Target speed too low, deactivate cruise control
                        (deactivate-cruise-control)
                        (print "Cruise control deactivated: speed too low")
                    })
                })
            })
        } {
            ; No ratio available, can't safely decrease
            (deactivate-cruise-control)
            (print "Cruise control deactivated: no speed ratio available")
        })
    })
})

; Function to monitor TX button and handle profile switching or speed adjustment
(defun monitor-tx-button () {
    (loopwhile t {
        (let ((current-button-state (gpio-read 'pin-tx))) {
            ; Detect button press (edge: 1 -> 0, button pulls pin low)
            (if (and (= tx-button-state 1) (= current-button-state 0)) {
                ; Button was just pressed
                (if (= cruise-active 1) {
                    ; If cruise control is active, decrease speed by 1 km/h
                    (decrease-cruise-speed)
                } {
                    ; If cruise control is not active, switch profile
                    (switch-profile)
                })
            })
            (setq tx-button-state current-button-state)
        })
        (sleep 0.05)  ; Check button every 50ms
    })
})

; Initialize with profile 0
(apply-profile 0)

; Start rpm-per-ms calculation in separate thread
(spawn 150 update-rpm-per-ms)
; Start monitoring RX button in separate thread
(spawn 150 monitor-rx-button)
; Start monitoring TX button in separate thread
(spawn 150 monitor-tx-button)
; Start monitoring throttle and brake in separate thread
(spawn 150 monitor-throttle-brake)

; =====================================================================
;  Quick-action panel server (talks to the ESP32-P4 swipe-out drawer)
; ---------------------------------------------------------------------
;  Wire protocol (app payload; the COMM_CUSTOM_APP_DATA byte is added/
;  stripped by the firmware on both ends — see vesc_lisp_panel.h):
;
;    P4 -> us : [0x56 0x50][msg][reply-can-id] (+ [ctrl-id][i32 val] for ACTION)
;    us -> P4 : [0x56 0x50][msg][...]   via (send-data buf 2 reply-can-id)
;
;  All floats travel as int32 = round(value * 1000), big-endian.
;  This is a working TEMPLATE — the throttle-disable and traction-control
;  mechanics below are deliberately simple and should be tuned to the
;  actual app (ADC / PPM / UART) and vehicle.
; =====================================================================

; --- little-endian-free append helpers over pbuf/pi (big-endian, scale 1000) ---
(defun pu8  (v) { (bufset-u8  pbuf pi v) (setq pi (+ pi 1)) })
(defun pi32 (v) { (bufset-i32 pbuf pi (to-i32 v)) (setq pi (+ pi 4)) })
(defun pstr (s) { (bufcpy pbuf pi s 0 (buflen s)) (setq pi (+ pi (buflen s))) })

; Describe the panel: 2 toggles + 1 number. ver=1, count=3.
(defun panel-send-ui (reply-id) {
    (setq pi 0)
    (pu8 0x56) (pu8 0x50) (pu8 0x81) (pu8 1) (pu8 3)
    ; id=1 Throttle (toggle)
    (pu8 1) (pu8 1) (pstr "Throttle") (pu8 (if (= throttle-on 1) 1 0))
    ; id=2 Traction Control (toggle)
    (pu8 2) (pu8 1) (pstr "Traction Control") (pu8 (if (= tc-on 1) 1 0))
    ; id=3 TC Sens (number, 0..100, step 5, no suffix)
    (pu8 3) (pu8 3) (pstr "TC Sens")
    (pi32 0) (pi32 100000) (pi32 5000) (pi32 (* tc-sens 1000.0)) (pstr "")
    (send-data pbuf 2 reply-id)
})

; Send the live state of every control (id + i32 value*1000).
(defun panel-send-state (reply-id) {
    (setq pi 0)
    (pu8 0x56) (pu8 0x50) (pu8 0x82) (pu8 3)
    (pu8 1) (pi32 (* (if (= throttle-on 1) 1 0) 1000))
    (pu8 2) (pi32 (* (if (= tc-on 1) 1 0) 1000))
    (pu8 3) (pi32 (* tc-sens 1000.0))
    (send-data pbuf 2 reply-id)
})

; Throttle master switch — off disables app output (motor ignores the grip),
; on returns control. Mirrors what cruise control does with app-disable-output.
(defun panel-set-throttle (on) {
    (if (= on 0) {
        ; Cleanly drop cruise first so its re-attach can't race our disable.
        (if (= cruise-active 1) (deactivate-cruise-control))
        (app-disable-output -1)  ; -1 = disable until re-enabled
        (setq throttle-on 0)
    } {
        (app-disable-output 0)   ; 0 = enable now
        (setq throttle-on 1)
    })
})

; Apply one control interaction from the panel.
(defun panel-action (cid val) {
    (cond
        ((= cid 1) (panel-set-throttle (if (> val 0.5) 1 0)))
        ((= cid 2) (setq tc-on (if (> val 0.5) 1 0)))
        ((= cid 3) (setq tc-sens val)))
})

; Parse one inbound frame from the P4.
(defun panel-handle (data) {
    (if (and (>= (buflen data) 4)
             (= (bufget-u8 data 0) 0x56)
             (= (bufget-u8 data 1) 0x50))
        (let ((msg (bufget-u8 data 2))
              (reply-id (bufget-u8 data 3))) {
            (cond
                ((= msg 0x01) (panel-send-ui reply-id))      ; REQ_UI
                ((= msg 0x03) (panel-send-state reply-id))   ; REQ_STATE
                ((= msg 0x02)                                ; ACTION
                    (let ((cid (bufget-u8 data 4))
                          (val (/ (bufget-i32 data 5) 1000.0))) {
                        (panel-action cid val)
                        (panel-send-state reply-id)          ; echo new state
                    })))
        }))
})

; Traction control — TEMPLATE slip limiter. Watches ERPM acceleration; if the
; wheel spins up faster than `limit` (derived from tc-sens) it momentarily cuts
; torque. Real TC needs proper slip estimation; tune for your vehicle.
(defun monitor-traction () {
    (let ((last-erpm 0.0)) {
        (loopwhile t {
            (if (= tc-on 1) {
                (let ((erpm (get-rpm))) {
                    (let ((accel (- (abs erpm) (abs last-erpm)))
                          (limit (- 5000.0 (* tc-sens 40.0)))) {
                        (if (> accel limit) (set-current 0))
                    })
                    (setq last-erpm erpm)
                })
            } {
                (setq last-erpm (get-rpm))
            })
            (sleep 0.02)  ; 50 Hz
        })
    })
})

; Event loop: deliver every COMM_CUSTOM_APP_DATA frame to panel-handle.
(defun panel-event-loop () {
    (loopwhile t {
        (recv ((event-data-rx (? data)) (panel-handle data))
              (_ nil))
    })
})

(event-register-handler (spawn panel-event-loop))
(event-enable 'event-data-rx)
(spawn 150 monitor-traction)

