# Head unit — User Guide

**Applies to firmware 1.3.42 and later · companion app 0.3.51 and later**
ESP32-P4 head unit with an 800 × 480 touch display

This guide describes every screen of the head unit, the elements on it, and
the procedures that are easier to follow step by step: choosing a battery
source, pairing sensors, setting the odometer, and recovering when something
does not behave as expected.

The illustrations are screenshots of the device's own interface and show the
actual layout, typography and colours of each screen.

---

## Contents

1. Overview
2. Touch gestures
3. Dashboards
   - 3.1 Cockpit
   - 3.2 Classic Max
   - 3.3 Cockpit (Amber)
   - 3.4 Twin Arc
4. Settings
   - 4.1 Connection
   - 4.2 Display
   - 4.3 Bike
   - 4.4 System
5. Vehicle tools
   - 5.1 VESC Config
   - 5.2 Realtime
   - 5.3 LISP editor
   - 5.4 Quick actions
6. Rides and history
   - 6.1 Trip statistics — list
   - 6.2 Trip statistics — detail
   - 6.3 Odometer dialog
7. Sensors and battery
   - 7.1 Pedal assist (PAS)
   - 7.2 Speed sensor
   - 7.3 Battery monitor (JK-BMS)
8. Utilities
   - 8.1 QR codes
   - 8.2 Logs
9. Common tasks
10. Troubleshooting
11. Abbreviations and units

---

## 1. Overview

The head unit is a touch display for an electric vehicle built around a VESC
controller. It runs a real-time dashboard (or Android Auto, when a phone is
connected), reads telemetry from the controller over CAN or Bluetooth, and can
take battery data from a BLE battery-management system. Settings are stored on
the device and applied immediately.

The interface is organised in three layers:

| Layer | What it contains |
| --- | --- |
| Dashboard | Live ride data. Four themes, one active at a time (§3). |
| Menus | Settings, VESC configuration, trip history, sensors, diagnostics. |
| Overlays | Quick actions, the odometer dialog, QR codes, the boot splash. |

There is no OK / Apply button anywhere: every switch and every number field is
saved the moment it is changed.

## 2. Touch gestures

| Gesture | Result |
| --- | --- |
| Tap the top-bar labels `VESC`, `STATISTICS`, `SETTINGS` | Open VESC Config (§5.1), trip history (§6.1) or Settings (§4) |
| Three-finger tap | Switch between the dashboard and the phone screen configured in Settings → Connection (§4.1) |
| Swipe right from the left edge | Open Quick actions (§5.4) |
| Tap outside the Quick-actions drawer | Close it |
| Drag horizontally on the dashboard background | Adjust display brightness — only while **Brightness gesture** is enabled (§4.2) |
| Tap the battery percentage on the dashboard | Open the battery monitor (§7.3) |
| Tap anywhere outside a dialog | Close the dialog without saving |

> **Note.** Gestures are disabled while Android Auto is on screen; they are
> handled by the dashboard and the menus, not by the phone.

---

## 3. Dashboards

The dashboard is the screen the rider sees while moving. One of the four themes
is active at a time; all of them show the same data and differ in layout and
colour. Choose the theme in **Settings → Display → Dashboard theme** (§4.2).

Two themes are designed for the 800 × 480 panel only. On the 480 × 480 ESP32-S3
board, Cockpit and Classic Max are available; Cockpit (Amber) and Twin Arc are
not compiled into that build.

### 3.1 Cockpit

![Cockpit dashboard](images/ui/dashboard-cockpit.png)

The default theme. It groups the ride into four areas: battery, speed, power and
a bottom telemetry strip.

| Element | Description |
| --- | --- |
| `VESC` | Opens VESC Config (§5.1) |
| `MODE n` | Active LISP speed profile reported by the controller |
| Uptime | Time since the controller was powered on |
| `STATISTICS` | Opens trip history (§6.1) |
| `BT` | Lit while a BLE peer — phone or sensor — is connected |
| `SETTINGS` | Opens Settings (§4) |
| `BATTERY` | State of charge, remaining capacity in Ah, and a level bar |
| `RANGE` | Estimated remaining distance, from the configured capacity and recent consumption |
| `SPEED` | Speed in the selected unit; `MAX 60` marks the top of the scale |
| `POWER` | Instantaneous power in kW, current in A, and a bar scaled to **Power Max** (§4.2) |
| Bottom strip | Pack voltage, trip distance, lifetime odometer, motor temperature, controller temperature |

The battery percentage is a tap target for the battery monitor (§7.3).

In `JK-BMS (BLE)` mode the percentage, remaining Ah and range come from the
battery pack itself. While the pack is offline they read `N/A`; the head unit
never substitutes an unrelated controller estimate.

### 3.2 Classic Max

![Classic Max dashboard](images/ui/dashboard-classic-max.png)

The Cockpit layout with session comparison values added:

| Element | Description |
| --- | --- |
| `P MAX` | Peak power for the current session |
| `V MAX` | Top speed for the current session |
| `RESET` | Zeroes the trip counters: distance, energy, time, peak power and top speed |

`RESET` does **not** touch the odometer. The odometer is the lifetime total and
is changed only through **Settings → Bike → Odometer → Set** (§6.3).

### 3.3 Cockpit (Amber)

![Cockpit Amber dashboard](images/ui/dashboard-amber.png)

The Cockpit layout in a warm amber palette: seven-segment numerals and amber
bars on black. Every element behaves exactly as in Cockpit (§3.1); only the
appearance changes.

The battery percentage remains a tap target for the battery monitor (§7.3).

### 3.4 Twin Arc

![Twin Arc dashboard](images/ui/dashboard-twin-arc.png)

Two 270° rings frame the ride: speed on the left, power on the right. The centre
column is the battery block; the card below it shows the phone's now-playing
track.

| Area | Contents |
| --- | --- |
| Top bar | `VESC`, `STATISTICS`, clock, `SETTINGS`, `BT`, and a green `CC nn` badge while cruise control is active |
| Speed ring | `KM/H` or `MPH`, the speed digits, `SPEED` and `MODE n`; the ring fills against a 0–60 km/h scale |
| Power ring | Instantaneous `kW`, `POWER`, current in A and the session's `MAX n.n kW` |
| Centre column | Battery percentage, level bar, remaining Ah and estimated range |
| Now-playing card | Track title and artist from the phone |
| Bottom strip | Voltage, trip, odometer, motor temperature, controller temperature |

The battery percentage opens the battery monitor (§7.3). In `JK-BMS (BLE)` mode
an offline pack reads `N/A`.

An animated preview of the rings and the now-playing card is available in
[`twin-arc-demo.gif`](images/ui/twin-arc-demo.gif).

---

## 4. Settings

Settings is a single screen with four tabs. Every change is applied and stored
immediately.

### 4.1 Connection

![Settings — Connection](images/ui/settings-connection.png)

| Element | Description |
| --- | --- |
| `VESC link → Open` | Transport screen: CAN or Bluetooth to the controller, VESC Express pairing, live link state |
| `Phone screen (3-finger)` | What the three-finger gesture shows: `Android Auto` (phone projection) or `Navigator` (the companion app's map over BLE) |
| `AA Auto-Connect` | Let the BT agent page the last paired phone at power-on. **Off by default.** With it off, the connection is started from the phone or with the manual **Connect** button |

Manual connection is always available regardless of this setting. Enabling it
only adds the automatic reconnection at boot.

### 4.2 Display

![Settings — Display](images/ui/settings-display.png)

| Element | Description |
| --- | --- |
| `Dashboard theme` | `Cockpit`, `Classic Max`, `Cockpit (Amber)` or `Twin Arc`; switches without a reboot. The last two are 800 × 480 builds only |
| `Boot splash repeats` | Number of times the boot animation plays (`Off` … `5`) |
| `Flip screen 180` | For an upside-down installation; takes effect after a reboot |
| `Power Max (kW)` | Full scale of the power bar and the `MAX` label |
| `Brightness gesture` | Enables the invisible full-screen brightness slider (horizontal drag on the dashboard) |
| `Units (miles)` | `Off` = km and km/h; `On` = miles and mph. Applies live to the dashboard, trip history and odometer dialog |
| `Temperature (F)` | `Off` = °C; `On` = °F |

### 4.3 Bike

![Settings — Bike](images/ui/settings-bike.png)

| Element | Description |
| --- | --- |
| `Battery Capacity (Ah)` | Pack size used for the percentage and range estimate |
| `Battery Calculation` | `Direct from Controller` — use the VESC's own percentage; `Smart Calculation` — count Ah in and out against the capacity, with charge detection; `JK-BMS (BLE)` — read SOC, remaining Ah and range from a paired Jikong pack |
| `Battery monitor (JK-BMS) → Open` | Battery pack screen: live state, cells, temperatures and Bluetooth pairing (§7.3) |
| `Pedal assist (PAS) → Open` | Pedal-assist screen (§7.1) |
| `Speed sensor → Open` | Wheel-sensor screen (§7.2) |
| `Odometer → Set` | Re-base the lifetime total (§6.3). The total is checkpointed every 10 s, so a key-off never loses the end of a ride |

### 4.4 System

![Settings — System](images/ui/settings-system.png)

| Element | Description |
| --- | --- |
| `Demo mode` | Drive the dashboard with synthetic values for demonstration |
| `VESC Emulator` | Simulate a controller on the bus; requires a restart |
| `Reset trip` | Same as the dashboard's `RESET` (§3.2) |
| `Files` | On-device file browser for the internal flash and an SD card |
| `Logs` | The firmware log viewer (§8.2) |
| `QR codes` | Wi-Fi and firmware-OTA codes for the phone (§8.1) |
| `Reset` | Restore all settings to defaults. The odometer, paired sensors and trip log survive by design |

Below the buttons the screen reports the firmware versions (host, BT agent, C6
Wi-Fi module) and CAN bus health.

---

## 5. Vehicle tools

### 5.1 VESC Config

![VESC Config](images/ui/vesc-config.png)

**To open:** tap `VESC` in the dashboard's top bar.

The controller's own configuration, edited from the panel without a laptop. The
title shows the controller firmware version.

| Element | Description |
| --- | --- |
| `Motor` / `App` | Select the parameter set |
| Two dropdowns | Select the group and sub-group (`General`, `Current`, `Speed`, …) |
| Parameter rows | One row per VESC parameter — switches, dropdowns and number fields — generated from the VESC Tool description |
| `Read` | Load the configuration from the controller |
| `Write` | Push the edited configuration back |
| `Defaults` | Load the controller's default values |
| `Detect` | Start the FOC detection wizard |
| `Restore` | Bring back the last backup |
| `LISP` / `Realtime` | Jump to the LISP editor (§5.3) or the live telemetry screen (§5.2) |

The status line at the bottom confirms the result of the last operation
(`Read OK`, `Write OK`, and so on).

### 5.2 Realtime

![Realtime](images/ui/realtime.png)

**To open:** VESC Config → `Realtime`.

A live telemetry read-out straight from the controller: input voltage, battery
current, motor current, power, duty cycle, ERPM, speed, temperatures and the
remaining values. Use it when the dashboard tiles do not show enough detail.
The list scrolls; `Back` returns to VESC Config.

### 5.3 LISP editor

![LISP editor](images/ui/lisp-editor.png)

**To open:** VESC Config → `LISP`.

A small editor for LISP scripts that run on the controller.

| Element | Description |
| --- | --- |
| `Open…` | Pick a script already stored on the device |
| `New` | Clear the buffer for a new script |
| Text area | Editable with the on-screen keyboard (tap the area to raise it) |
| `Save` | Write `<name>.lisp` to the device storage |
| `Read VESC` | Pull the script currently stored on the controller into the editor |
| `Upload+Run` | Erase the controller's script, write this buffer and start it |
| `Stop` | Halt the running script |

The `.lisp` suffix is added to the name automatically.

Scripts are not limited to the editor: the Quick-actions drawer (§5.4) is built
from the UI descriptor of the master script running on the controller.

### 5.4 Quick actions

![Quick actions](images/ui/quick-actions.png)

**To open:** swipe right from the left edge of the dashboard.

A drawer over the dashboard, so ride data stays visible while a setting is
changed. Tap anywhere outside it to close.

| Element | Description |
| --- | --- |
| `Cruise control` | Toggle cruise control |
| `Lights` | Toggle the lighting output |
| `Reset trip counters` | Zero the trip counters |
| `Assist level` | Stepper for the assist level |
| `Current limit` | Stepper for the current limit |
| Motor temperature | Live value at the bottom of the drawer |

The contents are **not hard-coded**: the drawer renders the UI descriptor
declared by the master LISP script running on the VESC. Editing the script
changes the controls that appear here.

---

## 6. Rides and history

### 6.1 Trip statistics — list

![Trip statistics — list](images/ui/trips-list.png)

**To open:** tap `STATISTICS` in the dashboard's top bar.

| Element | Description |
| --- | --- |
| Rows | One row per trip, newest first. `*` marks the trip still being recorded |
| Columns | Distance, duration, average speed, energy in Wh |
| Footer | Totals for everything the trip-log partition still holds — about 11 days of powered-on time at the 10 s sample rate |
| `Back` | Return to the dashboard |

### 6.2 Trip statistics — detail

![Trip statistics — detail](images/ui/trips-detail.png)

**To open:** tap any row in the trip list.

The header summarises the trip: distance, duration, average and maximum speed,
energy and charge (Wh / Ah), efficiency (Wh per km or mile) and the lowest pack
voltage.

| Element | Description |
| --- | --- |
| `Speed` `Power` `Voltage` `Temp` `Batt` | Metric tabs — each redraws the chart for the same trip. A second temperature tab appears when a second head is enabled |
| `X÷` / `X+` | Zoom the time axis |
| Drag on the chart | Pan left and right once zoomed |
| `Delete` | Delete the trip after confirmation. The trip still being recorded (`*`) cannot be deleted |

### 6.3 Odometer dialog

![Set odometer](images/ui/odometer-dialog.png)

**To open:** Settings → Bike → `Odometer → Set`.

A modal dialog with a numeric keypad, used when the lifetime total no longer
matches reality — after a controller replacement, a frame swap, or a firmware
flash that reset the controller's counter.

- The value is entered in the current display units (km or miles).
- `,` is the decimal separator; the value is converted before it is stored.
- `Save` writes the new total; `Cancel` or the keypad's `✕` leaves it unchanged.

The dialog edits whichever odometer the dashboard is showing: the VESC mirror,
or the BLE wheel sensor's when that is the selected speed source. The
dashboard's `ODO` field and this dialog always show the same number.

---

## 7. Sensors and battery

### 7.1 Pedal assist (PAS)

![Pedal assist](images/ui/pas.png)

**To open:** Settings → Bike → `Pedal assist → Open`.

The pedal-assist feature runs entirely on the head unit; no phone is involved.

| Element | Description |
| --- | --- |
| `Cadence sensor` | Status, battery, live cadence in RPM, rotation direction and the assist current it requests |
| `Search` / `Forget` | Pair or unpair a BLE cadence sensor |
| `Throttle / assist enabled` | Master switch for pedal assist |
| `Reverse rotation` | For a sensor mounted on the opposite side |
| `Assist level` | Amount of assistance |
| `Max current` | Upper current limit |
| `Mode` | Assist strategy, for example `Proportional to cadence` |
| `Advanced` | Start delay, start current and the remaining ramp parameters |

Any standard BLE cadence sensor can be used. The companion project
[cadence-for-ebike](https://github.com/payalneg/cadence-for-ebike) provides an
open custom firmware for the COOSPO BK6LS sensor: it keeps the standard Cycling
Speed & Cadence profile and adds a service that streams signed RPM — with the
rotation direction — every 100 ms.

### 7.2 Speed sensor

![Speed sensor](images/ui/speed-sensor.png)

**To open:** Settings → Bike → `Speed sensor → Open`.

| Element | Description |
| --- | --- |
| `Dashboard speed from` | `VESC` — speed from the controller's ERPM; `BLE wheel sensor` — speed from the paired sensor |
| `Wheel sensor` | Status, battery, live speed, trip and odometer |
| `Search` / `Forget` | Pair or unpair a BLE CSC sensor |
| `Wheel diameter` | Outer tyre diameter in millimetres; also used by the controller's speed calculation helper |

With the wheel sensor selected, speed, trip and the odometer all come from it
and are kept on this display.

### 7.3 Battery monitor (JK-BMS)

![Battery monitor](images/ui/bms.png)

**To open:** Settings → Bike → `Battery monitor (JK-BMS) → Open`, or tap the
battery percentage on any dashboard.

The screen reads a Jikong (JK-BMS) pack over Bluetooth LE.

| Element | Description |
| --- | --- |
| `Status` | `Connected`, `Searching…` or `Not paired` |
| State of charge | The pack's own SOC, in large digits |
| `Remaining` / `Full capacity` | Pack Ah counters — not a controller estimate |
| `Voltage` / `Current` / `Power` | Values reported by the BMS. Motor current and power still come from the VESC |
| Temperatures | MOSFET, T1 and T2 probes |
| Cell range | Lowest and highest cell voltage, and the delta between them |
| `Cycles` / `MOSFETs` / `Model` | Cycle count, state of health, charge and discharge FET state, and the pack model string |

![JK-BMS pairing](images/ui/bms-pairing.png)

**Pairing.** `Search` scans for JK-BMS packs; tap a result to bind it, `Forget`
drops the binding. A BMS accepts only one BLE connection at a time, so close the
phone app before pairing. The selected pack is saved and re-bound at the next
boot.

Select `JK-BMS (BLE)` under **Battery Calculation** to use the pack for the
dashboard, the Android Auto HUD and the range estimate. If the selected pack is
offline or its data is stale, SOC, remaining Ah and range read `N/A` rather than
an unrelated controller value.

---

## 8. Utilities

### 8.1 QR codes

![QR codes](images/ui/qr-codes.png)

**To open:** Settings → System → `QR codes`.

Two codes for the phone:

| Code | Purpose |
| --- | --- |
| Wi-Fi | The head unit's access point, so the companion app can join it and transfer firmware over HTTP. The caption states whether the access point is running |
| Firmware OTA | The URL of the on-device OTA page. The caption states whether the HTTP server is running |

### 8.2 Logs

![Logs](images/ui/logs.png)

**To open:** Settings → System → `Logs`.

The firmware's own log — the same lines that are written to the UART console —
captured in a PSRAM ring buffer so it can be read on the panel. It is the first
place to look when the device misbehaves away from a computer.

New lines follow the tail while the view is at the bottom; scrolling up pins the
view. `Back to settings` returns.

---

## 9. Common tasks

### Choose the battery source

1. Open **Settings → Bike**.
2. Set **Battery Capacity (Ah)** — the pack's usable capacity.
3. Choose **Battery Calculation**:
   - `Direct from Controller` — the VESC's own percentage;
   - `Smart Calculation` — Ah counting with charge detection;
   - `JK-BMS (BLE)` — the pack's own SOC and remaining capacity.
4. For `JK-BMS (BLE)`, open **Battery monitor (JK-BMS)** and pair the pack
   (§7.3).

### Pair a BLE sensor

1. Open the sensor's screen: **Settings → Bike → Pedal assist** (§7.1) or
   **Speed sensor** (§7.2).
2. Tap `Search`; the screen lists the sensors in range.
3. Tap the required sensor to bind it.
4. `Forget` removes the binding.

### Set or re-base the odometer

1. Open **Settings → Bike → Odometer → Set**.
2. Enter the new total in the current display units.
3. Tap `Save`.

### Change the dashboard theme

1. Open **Settings → Display**.
2. Choose one of the themes under **Dashboard theme**.
3. The dashboard changes immediately; `Back` returns to it.

### Change units

Open **Settings → Display** and set **Units (miles)** and **Temperature (F)**.
Both apply immediately to the dashboard, trip history and dialogs.

### Reset the counters without losing the total

Use `RESET` on the Classic Max dashboard (§3.2), **Settings → System → Reset
trip**, or `Reset trip counters` in Quick actions (§5.4). A factory reset
(**Settings → System → Reset**) restores the settings but keeps the odometer,
the paired sensors and the trip log.

---

## 10. Troubleshooting

| Symptom | What to check |
| --- | --- |
| The dashboard is blank or frozen | Confirm the firmware is 1.3.42 or later — earlier versions could freeze the Twin Arc dashboard after boot. Power-cycle the unit; if the screen stays blank, open **Settings → System → Logs** from a working screen and read the last lines |
| The dashboard does not refresh after switching themes | Update to 1.3.42 or later; if it still happens, note the active theme and the last log lines |
| `N/A` instead of battery percentage, Ah and range | In `JK-BMS (BLE)` mode the selected pack is offline or its data is stale. Open **Battery monitor (JK-BMS)**, check the status and the pairing. Only one BLE connection is possible — close the phone app. Alternatively select another **Battery Calculation** mode |
| A sensor is not found by `Search` | The sensor must be a BLE CSC (speed/cadence) device, awake, in range and not connected to another host |
| Speed and trip do not move | Check **Settings → Bike → Speed sensor**: the source must match the hardware. With `BLE wheel sensor` selected, the sensor has to be paired and connected |
| Android Auto does not connect at power-on | **AA Auto-Connect** is off by default (§4.1). Start the connection from the phone, or tap **Connect** on the idle screen |
| The trip counters reset unexpectedly | They are reset by `RESET`, **Reset trip**, Quick actions or the demo/emulator modes. The odometer is not affected |
| The display is upside down | Enable **Flip screen 180** in **Settings → Display** and reboot |
| A setting was changed by mistake | **Settings → System → Reset** restores defaults. The odometer, paired sensors and trip log are preserved |

---

## 11. Abbreviations and units

| Term | Meaning |
| --- | --- |
| AA | Android Auto |
| AP | Wi-Fi access point |
| BLE | Bluetooth Low Energy |
| BMS | Battery management system |
| CAN | Controller Area Network — the wired link to the VESC |
| CSC | Cycling Speed and Cadence — the standard BLE profile used by wheel and cadence sensors |
| ERPM | Electrical RPM — the controller's raw speed value |
| FET | Field-effect transistor — the switching devices inside the controller and the BMS |
| HUD | The status strip drawn over the Android Auto projection |
| JK-BMS | Jikong battery management system |
| LISP | The scripting language executed by the VESC controller |
| OTA | Over-the-air firmware update |
| PAS | Pedal assist system |
| RPM | Revolutions per minute |
| SOC / SOH | State of charge / state of health |
| VESC | The motor controller family this head unit is built around |
| Ah / Wh | Ampere-hours / watt-hours — charge and energy |
