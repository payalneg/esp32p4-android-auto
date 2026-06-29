(def cruise-active 0)
(def cruise-rpm 0)
(def rx-button-state 1)
(def tx-button-state 1)
(def old-throttle 0.0)
(def old-brake 0.0)
(def app-output-disabled 0)
(def adc-detached 0)
(def current-profile 0)
(def num-profiles 3)
(def first-profile-init 1)
(def rpm-per-ms 0.0)
(def throttle-on 1)
(def tc-on 0)
(def tc-sens 50.0)
(def pbuf (bufcreate 128))
(def pi 0)
(def beep-vol-addr 0)
(def beep-vol (let ((v (eeprom-read-i beep-vol-addr)))
                (if (and v (>= v 0) (<= v 50)) v 30)))
(def beep-vol-dirty 0)
(def melody-vol-addr 1)
(def melody-vol (let ((v (eeprom-read-i melody-vol-addr)))
                  (if (and v (>= v 0) (<= v 50)) v 40)))
(def melody-vol-dirty 0)
(def playing-idx -1)
; Pedal-assist setpoint from the head unit (over COMM_CUSTOM_APP_DATA, msg 0x05).
; pas-amps is the requested motor current (A); pas-seen is the (systime) of the
; last frame, for a staleness check. setq'd at runtime → MUST stay above @const.
(def pas-amps 0.0)
(def pas-seen 0)
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
(defun apply-profile (profile-index) {
    (if (= profile-index 0) {
        (conf-set 'max-speed (/ 25.0 3.6))
        (conf-set 'l-current-max 45.0)
        (print "Profile 0: Slow (25 km/h, 45 A)")
    } {
        (if (= profile-index 1) {
            (conf-set 'max-speed (/ 40.0 3.6))
            (conf-set 'l-current-max 60.0)
            (print "Profile 1: Medium (40 km/h, 60 A)")
        } {
            (if (= profile-index 2) {
                (conf-set 'max-speed (/ 60.0 3.6))
                (conf-set 'l-current-max 90.0)
                (print "Profile 2: Fast (60 km/h, 90 A)")
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
            (foc-play-tone 0 beep-freq 10)
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
(defun activate-cruise-control () {
    (if (and (= cruise-active 0) (= throttle-on 1)) {
        (setq cruise-rpm (get-rpm))
        (if (> (abs cruise-rpm) 0) {
            (app-disable-output -1)
            (setq app-output-disabled 1)
            (setq cruise-active 1)
            (print (str-merge "Cruise control activated at RPM: " (to-str cruise-rpm)))
            (set-rpm cruise-rpm)
        } {
            (print "Cannot activate cruise control: speed is zero")
        })
    })
})
(defun deactivate-cruise-control () {
    (if (= cruise-active 1) {
        (setq cruise-active 0)
        (setq cruise-rpm 0)
        (setq rpm-per-ms 0.0)
        (if (= app-output-disabled 1) {
            (if (= adc-detached 0) {
                (app-adc-detach 1 1)
                (setq adc-detached 1)
            })
            (app-adc-detach 1 0)
            (set-current 0)
            (sleep 0.05)
            (app-disable-output 0)
            (setq app-output-disabled 0)
            (app-adc-detach 1 0)
            (setq adc-detached 0)
        })
        (print "Cruise control deactivated")
    })
})
(defun monitor-throttle-brake () {
    (loopwhile t {
        (if (= cruise-active 1) {
            (let ((throttle-value (get-adc-decoded 0))) {
                (if (and (> throttle-value 0.05) (<= old-throttle 0.05)) {
                    (deactivate-cruise-control)
                })
                (setq old-throttle throttle-value)
            })
            (let ((brake-value (get-adc-decoded 1))) {
                (if (and (> brake-value 0.05) (<= old-brake 0.05)) {
                    (deactivate-cruise-control)
                })
                (setq old-brake brake-value)
            })
            (if (= cruise-active 1) {
                (set-rpm cruise-rpm)
            })
        } {
            (let ((throttle-value (get-adc-decoded 0))) {
                (setq old-throttle throttle-value)
            })
            (let ((brake-value (get-adc-decoded 1))) {
                (setq old-brake brake-value)
            })
        })
        (sleep 0.05)
    })
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
                        (set-rpm cruise-rpm)
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
                (set-rpm cruise-rpm)
                (print (str-merge "Cruise speed increased to RPM: " (to-str cruise-rpm)))
            })
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
(defun switch-profile () {
    (setq current-profile (+ current-profile 1))
    (if (>= current-profile num-profiles) {
        (setq current-profile 0)
    })
    (apply-profile current-profile)
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
                            (set-rpm cruise-rpm)
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
(apply-profile 0)
(spawn 150 update-rpm-per-ms)
(spawn 150 monitor-rx-button)
(spawn 150 monitor-tx-button)
(spawn 150 monitor-throttle-brake)
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
    (pu8 6) (pu8 1) (pstr "Polish Cow") (pu8 (if (= playing-idx 0) 1 0))
    (pu8 8) (pu8 1) (pstr "Free Bird") (pu8 (if (= playing-idx 1) 1 0))
    (pu8 7) (pu8 3) (pstr "Melody Vol")
    (pi32 0) (pi32 50000) (pi32 5000) (pi32 (* melody-vol 1000)) (pstr "")
    (send-data pbuf 2 reply-id)
})
(defun panel-send-state (reply-id) {
    (setq pi 0)
    (pu8 0x56) (pu8 0x50) (pu8 0x82) (pu8 5)
    (pu8 1) (pi32 (* (if (= throttle-on 1) 1 0) 1000))
    (pu8 5) (pi32 (* beep-vol 1000))
    (pu8 6) (pi32 (* (if (= playing-idx 0) 1 0) 1000))
    (pu8 8) (pi32 (* (if (= playing-idx 1) 1 0) 1000))
    (pu8 7) (pi32 (* melody-vol 1000))
    (send-data pbuf 2 reply-id)
})
(defun panel-send-dash (reply-id) {
    (setq pi 0)
    (pu8 0x56) (pu8 0x50) (pu8 0x84)
    (pi32 (* cruise-active 1000))
    (pi32 (* cruise-rpm 1000))
    (pi32 (* current-profile 1000))
    (pi32 (* rpm-per-ms 1000.0))
    (send-data pbuf 2 reply-id)
})
(defun panel-set-throttle (on) {
    (if (= on 0) {
        (if (= cruise-active 1) (deactivate-cruise-control))
        (app-disable-output -1)
        (setq throttle-on 0)
    } {
        (app-disable-output 0)
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
; Run the sequence in its own thread so the sleeps don't block panel-event-loop.
(defun panel-beep () (spawn 150 two-beeps))
(defun play-list (idx lst) {
    (setq playing-idx idx)
    (let ((n (length lst))) {
        (loopwhile (= playing-idx idx) {
            (let ((i 0)) {
                (loopwhile (and (< i n) (= playing-idx idx)) {
                    (let ((note (ix lst i))) {
                        (let ((f (ix note 0)) (d (ix note 1))) {
                            ; Carve a short silence out of the END of each tone so
                            ; consecutive notes are articulated instead of slurring
                            ; together. Gap is taken from d, so the tempo is unchanged.
                            (if (> f 0) {
                                (foc-play-tone 0 f melody-vol)
                                (let ((gap (if (< d 0.09) (/ d 3.0) 0.03))) {
                                    (sleep (- d gap))
                                    (foc-play-stop)
                                    (sleep gap)
                                })
                            } {
                                (foc-play-stop)
                                (sleep d)
                            })
                        })
                    })
                    (setq i (+ i 1))
                })
            })
            (if (= playing-idx idx) (sleep 0.3))
        })
    })
    (if (= playing-idx -1) (foc-play-stop))
})
(defun panel-action (cid val) {
    (cond
        ((= cid 1) (panel-set-throttle (if (> val 0.5) 1 0)))
        ((= cid 4) (panel-beep))
        ((= cid 5) {
            (setq beep-vol (to-i32 val))
            (setq beep-vol-dirty 1)
        })
        ((= cid 6)
            (if (> val 0.5)
                (if (not (= playing-idx 0)) { (setq playing-idx 0) (spawn 200 play-list 0 melody) })
                (if (= playing-idx 0) (setq playing-idx -1))))
        ((= cid 8)
            (if (> val 0.5)
                (if (not (= playing-idx 1)) { (setq playing-idx 1) (spawn 200 play-list 1 melody2) })
                (if (= playing-idx 1) (setq playing-idx -1))))
        ((= cid 7) {
            (setq melody-vol (to-i32 val))
            (setq melody-vol-dirty 1)
        }))
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
                ((= msg 0x05) {
                    ; Pedal-assist setpoint (fire-and-forget, no reply). i32 mA at
                    ; byte 4 (after magic[0,1], msg[2], reply-id[3]).
                    (setq pas-amps (/ (bufget-i32 data 4) 1000.0))
                    (setq pas-seen (systime))
                })
                ((= msg 0x02)
                    (let ((cid (bufget-u8 data 4))
                          (val (/ (bufget-i32 data 5) 1000.0))) {
                        (panel-action cid val)
                        (panel-send-state reply-id)
                    })))
        }))
})
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
            (sleep 0.02)
        })
    })
})
; Pedal-assist arbiter. Runs only when cruise is OFF and the panel throttle
; master is ON: cruise (set-rpm) owns the motor while active and pedaling must
; not disturb it; throttle-on=0 means the user disabled output via the panel, so
; leave it. Throttle-override: a touched throttle hands the motor straight back
; to the native ADC app (with its throttle curve); on a released throttle while
; pedaling we suppress the ADC app (so its loop can't fight us) and inject the
; PAS current, clamped to the active profile's l-current-max. A stale setpoint
; (sensor dropped) backs off — the P4 watchdog also sends 0.
(defun pas-control-loop () {
    (loopwhile t {
        (if (and (= cruise-active 0) (= throttle-on 1))
            (let ((thr (get-adc-decoded 0))
                  (stale (> (secs-since pas-seen) 0.4))
                  (lim (conf-get 'l-current-max)))
                (if (> thr 0.05)
                    (app-disable-output 0)
                    (if (and (not stale) (> pas-amps 0.0))
                        { (app-disable-output 100)
                          (set-current (if (> pas-amps lim) lim pas-amps) 0.1) }
                        (app-disable-output 0)))))
        (sleep 0.05)
    })
})
(defun panel-on-shutdown () {
    (if (or (= beep-vol-dirty 1) (= melody-vol-dirty 1)) {
        (shutdown-hold t)
        (if (= beep-vol-dirty 1) {
            (eeprom-store-i beep-vol-addr beep-vol) (setq beep-vol-dirty 0) })
        (if (= melody-vol-dirty 1) {
            (eeprom-store-i melody-vol-addr melody-vol) (setq melody-vol-dirty 0) })
        (shutdown-hold nil)
    })
})
; Volumes change via the panel slider (many intermediate values per drag) and
; must survive a reboot. Persisting only in panel-on-shutdown was unreliable:
; event-shutdown fires only on a real power-off, not on a bench / USB / re-flash
; reboot, so eeprom-store-i never ran. Flush the dirty volumes on a slow timer
; instead — coalesces a drag into ~one flash write and does not depend on a
; clean shutdown. panel-on-shutdown stays as a final flush.
(defun persist-volumes-loop () {
    (loopwhile t {
        (if (= beep-vol-dirty 1) {
            (eeprom-store-i beep-vol-addr beep-vol) (setq beep-vol-dirty 0) })
        (if (= melody-vol-dirty 1) {
            (eeprom-store-i melody-vol-addr melody-vol) (setq melody-vol-dirty 0) })
        (sleep 2)
    })
})
(defun panel-event-loop () {
    (loopwhile t {
        (recv ((event-data-rx . (? data)) (panel-handle data))
              (event-shutdown               (panel-on-shutdown))
              (_ nil))
    })
})
(def melody '(
  (330 0.124) (0 0.124) (330 0.124) (0 0.124) (440 0.124) (0 0.124)
  (440 0.124) (0 0.124) (330 0.124) (0 0.124) (330 0.124) (0 0.124)
  (262 0.124) (0 0.620) (330 0.124) (0 0.372) (330 0.124) (0 0.124)
  (330 0.124) (0 0.372) (330 0.124) (0 0.620) (294 0.124) (0 0.372)
  (294 0.124) (0 0.124) (294 0.124) (0 0.372) (294 0.124) (0 0.372)
  (330 0.124) (0 0.124) (330 0.124) (0 0.124) (440 0.124) (0 0.124)
  (440 0.124) (0 0.124) (330 0.124) (0 0.124) (330 0.124) (0 0.124)
  (262 0.124) (0 0.372) (330 0.124) (0 0.124) (330 0.124) (0 0.124)
  (440 0.124) (0 0.124) (440 0.124) (0 0.124) (330 0.124) (0 0.124)
  (330 0.124) (0 0.124) (262 0.124) (0 0.620) (330 0.124) (0 0.372)
  (330 0.124) (0 0.124) (330 0.124) (0 0.372) (330 0.124) (0 0.620)
  (294 0.124) (0 0.372) (294 0.124) (0 0.124) (294 0.124) (0 0.372)
  (294 0.124) (0 0.372) (330 0.124) (0 0.124) (330 0.124) (0 0.124)
  (440 0.124) (0 0.124) (440 0.124) (0 0.124) (330 0.124) (0 0.124)
  (330 0.124) (0 0.124) (262 0.124) (0 0.372) (330 0.124) (0 0.124)
  (330 0.124) (0 0.124) (440 0.124) (0 0.124) (440 0.124) (0 0.124)
  (330 0.124) (0 0.124) (330 0.124) (0 0.124) (262 0.124) (0 0.620)
  (330 0.124) (0 0.372) (330 0.124) (0 0.124) (330 0.124) (0 0.372)
  (330 0.124) (0 0.620) (294 0.124) (0 0.372) (294 0.124) (0 0.124)
  (294 0.124) (0 0.372) (294 0.124) (0 0.372) (330 0.124) (0 0.124)
  (330 0.124) (0 0.124) (440 0.124) (0 0.124) (440 0.124) (0 0.124)
  (330 0.124) (0 0.124) (330 0.124) (0 0.124) (262 0.124) (0 0.372)
  (330 0.124) (0 0.124) (330 0.124) (0 0.124) (440 0.124) (0 0.124)
  (440 0.124) (0 0.124) (330 0.124) (0 0.124) (330 0.124) (0 0.124)
  (262 0.124) (0 0.620) (330 0.124) (0 0.372) (330 0.124) (0 0.124)
  (330 0.124) (0 0.372) (330 0.124) (0 0.620) (294 0.124) (0 0.372)
  (294 0.124) (0 0.124) (294 0.124) (0 0.372) (294 0.124) (0 0.372)
  (330 0.124) (0 0.124) (330 0.124) (0 0.124) (440 0.124) (0 0.124)
  (440 0.124) (0 0.124) (330 0.124) (0 0.124) (330 0.124) (0 0.124)
  (262 0.124) (0 0.372) (330 0.124)
))
; Free Bird guitar solo (lead line) — from "Free Bird - Lynyrd Skynyrd.mid":
; track 9, one of the twin harmonized lead guitars (clean monophonic line, no
; vamp), sped up 20% (durations x0.8). The play-list articulation gap separates
; the notes. TRIMMED to the opening third (~170 notes) — the full ~470-note
; version pushed the load-time cons heap over the ceiling (the reader builds the
; whole quoted list in the heap before @const flashes it), which killed the
; panel spawn at the end of the file → "no buttons". Keep this short; if you
; re-extend it, watch for the panel/melodies dying at upload.
(def melody2 '(
  (659 0.207) (880 0.207) (659 0.207) (880 1.242) (784 0.207) (659 0.207)
  (784 0.207) (880 0.207) (784 0.207) (659 0.207) (880 0.414) (784 0.207)
  (659 0.207) (880 0.621) (1047 0.207) (880 0.414) (1047 0.207) (880 0.414)
  (1047 0.207) (880 0.621) (659 0.207) (880 0.207) (659 0.207) (880 1.242)
  (784 0.207) (659 0.207) (784 0.207) (880 0.207) (784 0.207) (659 0.207)
  (880 0.414) (784 0.207) (659 0.207) (880 0.621) (1047 0.207) (880 0.414)
  (1047 0.207) (880 0.414) (1047 0.207) (880 0.310) (1047 0.103) (880 0.310)
  (1047 0.103) (880 0.310) (1047 0.103) (880 0.310) (1047 0.103) (880 0.310)
  (1047 0.103) (880 0.310) (1047 0.103) (880 0.310) (1047 0.103) (880 0.310)
  (1047 0.103) (880 0.310) (1047 0.103) (880 0.310) (1047 0.103) (880 0.310)
  (1047 0.103) (880 0.310) (1047 0.103) (880 0.310) (1047 0.103) (880 0.310)
  (1047 0.103) (880 0.310) (1047 0.103) (880 0.310) (1047 0.103) (880 0.310)
  (1047 0.103) (880 0.310) (1047 0.103) (880 0.310) (1047 0.103) (784 0.069)
  (880 0.069) (784 0.069) (659 0.069) (784 0.069) (880 0.069) (784 0.069)
  (659 0.069) (784 0.069) (880 0.069) (784 0.069) (659 0.069) (784 0.069)
  (880 0.069) (784 0.069) (659 0.069) (784 0.069) (880 0.069) (784 0.069)
  (659 0.069) (784 0.069) (880 0.069) (784 0.069) (659 0.069) (784 0.069)
  (880 0.069) (784 0.069) (659 0.069) (784 0.069) (880 0.069) (784 0.069)
  (659 0.069) (784 0.069) (880 0.069) (784 0.069) (659 0.069) (784 0.069)
  (880 0.069) (784 0.069) (659 0.069) (784 0.069) (880 0.069) (784 0.069)
  (880 0.069) (784 0.069) (659 0.069) (784 0.069) (880 0.069) (784 0.069)
  (659 0.069) (784 0.069) (880 0.069) (784 0.069) (659 0.069) (784 0.069)
  (880 0.069) (784 0.069) (659 0.069) (784 0.069) (880 0.069) (784 0.069)
  (880 0.069) (784 0.069) (659 0.069) (784 0.069) (880 0.069) (784 0.069)
  (659 0.069) (784 0.069) (880 0.069) (784 0.069) (659 0.069) (784 0.069)
  (880 0.069) (784 0.069) (659 0.069) (784 0.069) (880 0.069) (784 0.069)
  (659 0.069) (784 0.069) (880 0.069) (784 0.069) (659 0.069) (784 0.069)
  (880 0.069) (784 0.069) (880 0.207) (784 0.207) (659 0.207) (880 0.827)
))

; Spawn threads and enable events LAST — after melody/melody2 (defined just above)
; are bound. panel-event-loop calls panel-action, which references melody2 (cid 8);
; spawned earlier, an incoming panel command during load would hit melody2 while
; still unbound → the handler thread dies → panel/melodies dead.
; These are plain expressions (not definitions), so @const-start does not flash
; them — they just execute here, which is exactly what we want.
(event-register-handler (spawn panel-event-loop))
(event-enable 'event-data-rx)
(event-enable 'event-shutdown)
(spawn 150 persist-volumes-loop)
(spawn 150 pas-control-loop)
@const-end
