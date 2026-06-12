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
(defun play-stop () {
    (sleep 0.1)
    (foc-play-stop)
})
(defun apply-profile (profile-index) {
    (if (= profile-index 0) {
        (conf-set 'max-speed (/ 25.0 3.6))
        (print "Profile 0: Slow (25 km/h)")
    } {
        (if (= profile-index 1) {
            (conf-set 'max-speed (/ 40.0 3.6))
            (print "Profile 1: Medium (35 km/h)")
        } {
            (if (= profile-index 2) {
                (conf-set 'max-speed (/ 60.0 3.6))
                (print "Profile 2: Fast (50 km/h)")
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
(defun panel-beep () {
    (foc-play-tone 0 800 beep-vol)
    (spawn 150 play-stop)
})
(defun play-list (idx lst) {
    (setq playing-idx idx)
    (let ((n (length lst))) {
        (loopwhile (= playing-idx idx) {
            (let ((i 0)) {
                (loopwhile (and (< i n) (= playing-idx idx)) {
                    (let ((note (ix lst i))) {
                        (let ((f (ix note 0)) (d (ix note 1))) {
                            (if (> f 0) (foc-play-tone 0 f melody-vol) (foc-play-stop))
                            (sleep d)
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
(defun panel-event-loop () {
    (loopwhile t {
        (recv ((event-data-rx . (? data)) (panel-handle data))
              (event-shutdown               (panel-on-shutdown))
              (_ nil))
    })
})
(event-register-handler (spawn panel-event-loop))
(event-enable 'event-data-rx)
(event-enable 'event-shutdown)
@const-start
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
(def melody2 '(
  (587 0.174) (698 0.304) (587 0.043) (698 0.174) (784 0.478) (0 0.043)
  (784 0.304) (0 0.043) (698 0.304) (587 0.043) (698 0.174) (784 0.304)
  (698 0.174) (587 0.043) (784 0.304) (0 0.043) (698 0.304) (587 0.043)
  (784 0.478) (0 0.043) (932 0.304) (784 0.174) (0 0.043) (932 0.304)
  (784 0.174) (0 0.043) (932 0.304) (784 0.174) (0 0.043) (784 0.304)
  (587 0.043) (784 0.304) (587 0.043) (698 0.174) (784 0.478) (0 0.043)
  (784 0.304) (0 0.043) (698 0.304) (587 0.043) (698 0.174) (784 0.304)
  (698 0.174) (587 0.043) (784 0.304) (0 0.043) (698 0.304) (587 0.043)
  (784 0.478) (0 0.043) (932 0.304) (784 0.174) (0 0.043) (932 0.304)
  (784 0.174) (0 0.043) (932 0.304) (784 0.130) (932 0.217) (784 0.130)
  (932 0.217) (784 0.130) (932 0.217) (784 0.130) (932 0.217) (784 0.130)
  (932 0.217) (784 0.130) (932 0.217) (784 0.130) (932 0.217) (784 0.130)
  (932 0.217) (784 0.130) (932 0.217) (784 0.130) (932 0.217) (784 0.130)
  (932 0.217) (784 0.130) (932 0.217) (784 0.130) (932 0.217) (784 0.130)
  (932 0.217) (784 0.130) (932 0.217) (784 0.130) (932 0.217) (784 0.130)
  (932 0.217) (784 0.174) (0 0.043) (698 0.261) (784 0.217) (698 0.130)
  (784 0.217) (698 0.130) (784 0.217) (698 0.130) (784 0.217) (698 0.130)
  (784 0.217) (698 0.130) (784 0.217) (698 0.130) (784 0.217) (698 0.130)
  (784 0.217) (698 0.130) (784 0.217) (698 0.130) (784 0.217) (698 0.130)
  (784 0.217) (698 0.130) (784 0.217) (698 0.130) (784 0.217) (698 0.130)
  (784 0.217) (698 0.130) (784 0.217) (698 0.130) (784 0.217) (698 0.130)
  (784 0.391) (0 0.043) (784 0.304) (0 0.043) (784 0.304) (587 0.043)
  (698 0.217) (587 0.130) (698 0.217) (587 0.130) (698 0.217) (587 0.130)
  (698 0.217) (587 0.130) (698 0.217) (587 0.130) (698 0.217) (587 0.130)
  (698 0.217) (587 0.130) (698 0.217) (587 0.130) (698 0.217) (587 0.130)
  (698 0.217) (587 0.130) (698 0.217) (587 0.130) (698 0.217) (587 0.130)
  (698 0.304) (440 0.043) (466 0.391) (392 0.087) (294 0.043) (466 0.391)
  (392 0.043) (466 0.087) (587 0.217) (466 0.043) (698 0.217) (587 0.130)
  (698 0.217) (587 0.130) (698 0.217) (587 0.130) (698 0.217) (587 0.130)
  (698 0.217) (587 0.130) (698 0.217) (587 0.130) (698 0.217) (587 0.130)
  (698 0.217) (587 0.348) (523 0.043) (587 0.217) (523 0.043) (587 0.217)
  (523 0.043) (587 0.217) (523 0.043) (587 0.217) (523 0.043) (587 0.217)
  (523 0.043) (587 0.217) (523 0.043) (587 0.217) (523 0.043) (587 0.217)
  (523 0.043) (587 0.217) (523 0.043) (587 0.217) (523 0.043) (587 0.217)
  (523 0.043) (587 0.217) (523 0.043) (587 0.217) (523 0.043) (587 0.217)
  (523 0.043) (587 0.217) (523 0.043) (587 0.217) (523 0.043) (587 0.217)
  (523 0.043) (587 0.217) (523 0.043) (587 0.217) (523 0.043) (587 0.217)
  (523 0.217) (587 0.304) (0 0.043) (587 0.304) (0 0.043) (587 0.304)
  (0 0.043) (523 0.087) (587 0.217) (523 0.087) (466 0.348) (392 0.087)
  (294 0.043) (466 0.391) (392 0.130) (466 0.391) (392 0.087) (294 0.043)
  (466 0.391) (392 0.130) (466 0.217) (392 0.130) (466 0.217) (392 0.087)
  (349 0.043) (392 0.304) (0 0.043) (392 0.217) (349 0.130) (392 0.217)
  (349 0.130) (392 0.217) (349 0.130) (392 0.304) (0 0.043) (392 0.217)
  (349 0.130) (392 0.217) (349 0.130) (392 0.087) (466 0.217) (392 0.348)
  (0 0.043) (392 0.217) (349 0.130) (392 0.217) (349 0.130) (392 0.087)
  (466 0.217) (392 0.348) (0 0.043) (392 0.217) (349 0.130) (392 0.217)
  (349 0.130) (392 0.087) (466 0.217) (392 0.348) (0 0.043) (392 0.217)
  (349 0.043) (466 0.217) (392 0.174) (0 0.043) (392 0.217) (349 0.043)
  (466 0.217) (392 0.174) (0 0.043) (392 0.217) (349 0.043) (466 0.217)
  (392 0.174) (0 0.217) (587 0.478) (466 0.043) (587 0.478) (466 0.043)
  (587 0.478) (466 0.043) (587 0.478) (466 0.043) (587 0.478) (466 0.043)
  (587 0.478) (466 0.043) (587 0.478) (466 0.043) (587 0.478) (466 0.043)
  (587 0.304) (0 0.043) (554 0.478) (523 0.348) (494 0.348) (466 0.043)
  (587 0.478) (466 0.043) (587 0.478) (466 0.043) (698 0.652) (587 0.522)
  (523 0.522) (466 0.174) (392 0.174) (0 0.043) (392 0.174) (466 0.304)
  (392 0.174) (0 0.043) (392 0.174) (466 0.304) (392 0.174) (0 0.043)
  (466 0.304) (392 0.174) (0 0.043) (294 0.174) (392 0.304) (294 0.043)
  (349 0.174) (392 0.478) (0 0.043) (392 0.304) (0 0.043) (349 0.304)
  (294 0.043) (349 0.174) (392 0.304) (349 0.174) (294 0.043) (392 0.304)
  (0 0.043) (349 0.304) (294 0.043) (392 0.478) (0 0.043) (466 0.304)
))
@const-end
