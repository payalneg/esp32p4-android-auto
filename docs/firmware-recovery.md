# Firmware recovery

Firmware 1.3.50 / companion 0.3.60 introduces an application watchdog and a
headless Bluetooth recovery mode. The flash layout and bootloader rollback
configuration are unchanged; existing USB-flashed bootloaders remain compatible.

## Behaviour

- A dedicated guardian watches actual LVGL timer, NimBLE event queue and
  navigator worker progress. A static map, missing phone, missing GPS, missing
  motor controller or unavailable Wi-Fi is not a stalled task.
- Direct Android Auto video intentionally pauses LVGL. Only that deliberate
  pause suspends the UI heartbeat requirement; yielding the panel back to LVGL
  restores it with a fresh 15-second grace period. BLE/nav remain monitored.
- Runtime heartbeats may be absent for 15 seconds. The guardian then permanently
  stops feeding its hardware task-watchdog user; the watchdog panics/reboots
  within another 15 seconds. A busy loop starving an idle task is also covered.
- Each startup stage has a 180-second deadline. Stages are NVS/startup, display,
  services, Bluetooth, Wi-Fi. The hardware watchdog is armed before NVS/display.
- Flash update/reset work has a bounded 120-second exception. An expired
  exception is fatal, never silently renewed by a timer. Successful completion
  restores the normal timeout and gives task heartbeats 15 seconds to recover.
- PANIC, CPU lockup and task/interrupt/general watchdog resets count toward a streak for the
  same application ELF hash. A normal power/software reset or a different image
  starts a new streak. Sixty seconds of healthy completed startup clears it.
- After three consecutive unstable fault resets, a persistent recovery flag
  starts Bluetooth and BLE OTA without display, splash, overclock, trip-log
  scan, CAN/PAS or sensor central clients. The flag survives power cycling.

In the phone app open **Firmware update → shield icon (Recovery)**. Recovery can
also be requested from a normally running device when its screen is white but
the UI task is still alive. Confirmations describe these actions:

1. Enter recovery and reboot.
2. Save display preferences, apply safe display defaults, reboot current image.
3. Apply safe display defaults, select the other OTA image, reboot.
4. Restore the saved display preferences and reboot.
5. Retry the current image/settings without recovery.

Only `vesc_cfg` keys `brightness`, `show_fps`, `dash_theme`, `splash_loops`,
`disp_flip`, `music_wdg`, `music_btn`, `aa_autocon`, `phone_scr` are changed.
Their values and absent/present state are backed up in a separate NVS blob
before the first reset and are not overwritten by repeated attempts. Odometer,
trip history, battery/controller configuration and Bluetooth bindings stay in
their own keys/partitions. Safe display mode also skips optional P4 overclock.

The other slot is an available image, **not a certified known-good image**.
`esp_ota_set_boot_partition` validates it before switching. Slot selection does
not erase either application. Normal OTA overwrites the inactive slot as before.
An older image without this guard cannot provide these new protections after a
rollback. Failure before the guard/NVS can run, radio hardware failure and an
electrically failed display still need USB/hardware diagnosis.

## BLE protocol

Optional read/write characteristic `7b4e4f00-3f8e-4d2a-9d5c-2c9f1a6e0020`
is appended to the notification bridge service, preserving existing handles.
READ returns UTF-8 JSON: `v=1`, `recovery`, `safe`, `failures`, reset `reason`,
running `slot`, `other` version (empty if no image), unsigned `nonce`, and last
command ESP-IDF `error` code. WRITE WITH RESPONSE is exactly six bytes:
`[1, opcode, nonce_u32_le]`. Opcode numbers are the five actions above.
An outdated nonce, unsupported command, normal-mode destructive operation or
duplicate pending command is rejected. Commands run on a separate task, never
performing NVS erase/commit or reboot inside a NimBLE callback. An accepted ATT
write means queued, not completed; a failure remains readable in `error`.
Successful operations reboot after 500 ms. BLE OTA remains available in recovery.
The new phone screen hides unsupported recovery on old firmware with an
explanation; it does not assume every connected device supports it.

## Verification

Host C policy test (Windows TinyCC, or GCC/Clang):

```powershell
tcc -Wall -Werror -o build/test_boot_guard.exe tools/tests/test_boot_guard_policy.c
build/test_boot_guard.exe
tcc -Wall -Werror -Itools/tests/boot_guard_stubs -o build/test_boot_guard_settings.exe tools/tests/test_boot_guard_settings.c main/boot_guard_settings.c
build/test_boot_guard_settings.exe
```

Phone protocol tests:

```text
cd flutter-application
flutter test test/recovery_test.dart
flutter analyze
```

Required physical fault tests (not replaced by successful builds/unit tests):

| Test | Expected |
| --- | --- |
| Leave static dashboard/map without phone for 10 min | No watchdog reset |
| Play Android Auto video for >60 sec, return to navigator | No false reset; UI monitoring resumes |
| Stop LVGL task/timer handler in debugger, then resume the CPUs | Reset with watchdog/PANIC, streak +1 |
| Stop the NimBLE event queue or nav worker, leaving guardian running | Reset, streak +1 |
| Block startup inside display init | Deadline/reset, no false ready marker |
| Repeat three unstable fault boots | Headless BLE recovery, no display/motor initialisation |
| Power-cycle recovery | Recovery remains available |
| Enter recovery from white-but-responsive UI using phone | Headless restart |
| Save/reset settings twice, restore | Original values AND missing-key state return |
| Check trips/odometer/controller/bindings before and after reset | Identical |
| Pick an empty/truncated other slot | No reboot into invalid image; readable error |
| Switch to valid other image | Selected version boots; safe display values persist |
| Upload BLE OTA in recovery | Works without LVGL; a different valid image boots normally; identical image retains recovery |
| Upload normal BLE/HTTP OTA, force write failure | Bounded grace, normal watchdog restored on error |
| Crash after >60 sec healthy operation | Streak starts at 1 |
| Ordinary reboot/power cycle outside recovery | No fault counted |
| Send stale nonce, malformed packet, duplicate command | Rejected; no settings/slot changes |

Debugger attachment can disable hardware watchdogs; detach or resume with the
watchdog enabled when measuring real reset behaviour. A white screen whose
LVGL/BLE/nav task loops all continue cannot be diagnosed from these heartbeats;
use the explicit recovery entry action.
