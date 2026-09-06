# Changelog

Two version trains ship together and are bumped in lockstep by
`scripts/release.sh`: the **ESP32-P4 firmware** (`1.x.y`, `version.txt`) and the
**Flutter companion app** (`0.x.y`, `pubspec.yaml`). The external BT-agent
firmware is versioned separately and is only bumped when `tools/bt_agent/`
changes.

Entries below name the firmware version; the app version of the same release is
the one recorded in the release commit.

## v1.3.15 / app 0.3.15 — 2026-09-06

Curated Android Auto branch: only the fixes and the changes that showed a
clear effect, without the experimental in-tree decoder. Not the 1.3.12-1.3.14
line (that stays on the video-perf branch).

### Android Auto reconnect and the Connect button

- A dropped session no longer strands the head unit. The socket gained TCP
  keepalive and receive/send timeouts, so a phone that leaves the AP silently
  (out of range, pocket) is noticed within ~15 s instead of hanging the session
  forever. A clean goodbye (ByeBye) is told apart from a lost link, and the
  phone is given a 3 s grace to restart projection on its own before the head
  unit kicks it off the AP.
- The BT agent leaves the air for the whole session (agent 0.6.4 -> 0.6.6),
  the way the reference dongles power their radio off, so a stray Bluetooth
  event can no longer make the phone restart projection mid-ride.
- **Connect now works.** Two dead paths fixed in agent 0.6.7: the phone it
  pages is the one that last ran Android Auto (not whichever paired last), and
  a Connect tap with the link half-up but no session tears it down so the phone
  re-runs the wireless setup instead of doing nothing.
- Agent updates no longer wipe the agent's own pairing. The OTA used to erase
  the agent's NVS on every write, so after each update the phone had to be
  re-paired; it now skips the NVS region.

### Screen

- The Android Auto idle screen shows the link state — Disconnected /
  Connecting... / Connected — in colour, with the step in progress as the
  subtitle and the IP / port on a dim third line.
- The backlight goes dark during flash erases and bulk writes instead of
  letting them tear the panel blue, and comes back a moment after the last one.
- The LVGL worker no longer busy-spins on core 0; it was starving Bluetooth and
  the idle task badly enough to trip the watchdog.
- A touch is ignored for 1 s after a dashboard <-> Android Auto switch so a
  stray finger doesn't land on the wrong screen.
- Boot-time "battery charged — reset trip?" prompt: the dashboard asks before
  resetting the trip instead of doing it silently.

## v1.3.11 / app 0.3.11 — 2026-09-02

### Faster firmware updates over Bluetooth

Why a BLE update took 10+ minutes: a 4.4 MB image went out as ~18 000
writes of 244 bytes, the phone stack issues them one per ATT round trip, and
four things kept each round trip slow:

- **Connection interval.** Nothing asked for a fast link, so the transfer ran
  at Android's default ~45 ms interval. The LISP editor already requests
  high priority (7.5–15 ms) for its transfers; the OTA path did not. Now the
  app requests it for the duration of the update, and the firmware asks from
  its side too on BEGIN (`ble_gap_update_params`, 11.25–15 ms — Android
  refuses anything shorter), reverting on failure.
- **Sensor initiator scan.** With a PAS or wheel-speed sensor bound but asleep
  (the usual state of a parked bike), the head unit keeps a connect attempt
  pending — and NimBLE's default connect parameters scan for the peer at
  100 % duty, sharing the radio with the phone link the whole time. The
  arbiter now scans 30 ms in every 100 ms (a sensor advertising at ~1 Hz is
  still caught within seconds of waking), and the OTA parks the arbiter
  entirely while receiving.
- **Chunk size.** The firmware's flatten buffer capped DATA writes at 244 B
  although both sides negotiate MTU 512. The buffer now takes 509 B and READY
  advertises that cap in its `detail` field; the app uses the largest chunk
  the cap and the MTU allow — half the writes. Older firmware sends 0 and the
  app keeps 244.
- **Ack fallback.** On Android's BUSY (TX credits exhausted) the app retried
  after 15 and 30 ms and then switched that chunk to an *acknowledged* write
  — one chunk per connection event. Under load that path swallowed most of
  the image. Retries now wait roughly a connection interval and stay
  unacknowledged for six attempts before falling back.
- Also: progress IPC to the UI isolate is throttled (was one message per
  chunk).

Note: the update *to* 1.3.11 still runs on the old firmware's side of the
protocol (244-byte chunks, no peripheral-side interval request, scan still
running) — only the app-side improvements apply to it. Updates *from* 1.3.11
onward get the full effect. Not hardware-measured yet.

## v1.3.10 / app 0.3.10 — 2026-09-02

### Android Auto no longer drops on a big video frame

- Field log (1.3.9, five minutes into a session): `aa_tls: decrypt: out_buf
  full (1564)` → `recv_decrypted: ESP_ERR_NO_MEM` → `tcp: client closed` —
  the panel fell back to the dashboard and the phone had to reconnect. A
  fragmented AA message (a video I-frame) came in larger than the fixed
  96 KiB reassembly buffer. The same cap had already been raised once, from
  32 KiB, for the post-BT-handover key-frame burst.
- `recv_decrypted` now reassembles into per-channel buffers that grow on
  demand (doubling from 96 KiB, hard ceiling 2 MiB — an 800×480 I-frame is
  ~100–300 KiB) and hands the message to the dispatcher by pointer instead
  of copying it out, which also removes a ~100 KiB memcpy per video frame.
  Growth steps are logged once (`ch 3 reassembly buffer 98304 -> 196608
  bytes`) so real frame sizes show up in the log.

### Trip statistics switch removed again

- The opt-in switch introduced in 1.3.9 is gone; statistics are always on,
  as before 1.3.9 (the NVS key it left behind is ignored). What stays from
  1.3.9 is the actual fix: the trip log's sector runway is erased at boot,
  before the display comes up, instead of at every stop longer than ~30 s.
- On the log's own device the runway was already clean (`runway: 64 clean
  sectors ahead`) — so the stop-time erases were NOT what that unit was
  freezing on; the 10 s record writes remain the only flash activity during
  a ride.

## v1.3.9 / app 0.3.9 — 2026-09-02

### Trip statistics are now opt-in (Settings → Trip statistics, default OFF)

- New switch **Settings → Trip statistics**. Off (the default): the dashboard's
  STATISTICS entry point is hidden on every theme and the trip log does
  nothing at all — no boot scan, no 10 s record writes, no sector erases, no
  flash I/O. On: behaves as before; toggling takes effect without a reboot
  (off stops sampling and erases at once, on scans the ring and starts a new
  trip). The Settings "Reset" button returns it to off.
- Why: every flash write or erase on this board suspends the cache for both
  cores AND stalls the DSI DMA that feeds the panel from PSRAM (AUTO_SUSPEND is
  unavailable on this GD25Q256, see `sdkconfig.defaults`), so the trip log was
  the one subsystem that touched flash *during a ride* — a 64-byte record
  every 10 s, plus 4 KB sector erases (45–400 ms each = a frozen screen with a
  blue flash) whenever the runway of pre-erased sectors ahead of the log's
  head had to be rebuilt. It is the prime suspect for the "dashboard freezes
  for a moment, often" reports. With the switch off a ride performs zero flash
  writes; if the freezes persist with it off, the cause is elsewhere (CAN data
  dropouts, render path) — the switch doubles as the experiment.
- Consequence of "off": the dashboard TRIP / Ah / uptime totals no longer
  survive a head-unit reboot (the log was their only persistence) and start
  from zero at every power-on; the smart-battery tracker falls back to its
  10-minute NVS backup. The odometer is unaffected (it comes from the VESC, or
  from the BLE speed sensor's own NVS counter).
- Firmware side (`components/trip_log`): the old boot scan never *cleaned*
  anything — at start it only read one record per sector to find the head and
  erased 2 sectors ahead; old trips were left for the ring to overwrite, and
  the erases of that stale data ran from the writer's idle loop, i.e. at every
  stop longer than ~30 s (one 45–400 ms freeze every 3 s until 64 sectors were
  clean), or just-in-time mid-ride once the runway ran out. Now, when the
  feature is on, the runway is rebuilt synchronously in `trip_log_init()`
  BEFORE `display_init()` — the panel is still dark, so a dirty sector costs
  boot time (typically a few sectors ≈ 0.1–0.3 s; capped at 3 s, the rest is
  left to the idle trickle) instead of a mid-ride freeze. The idle trickle and
  the just-in-time erase remain as fallbacks only.
- Not hardware-verified: the freeze diagnosis itself. Compile-tested on
  Waveshare + the desktop simulator.

## v1.3.8 / app 0.3.8 — 2026-08-19

### BLE wheel-speed sensor (stock Coospo-class CSC sensors)

- The head unit can now take its speed from an off-the-shelf BLE wheel-speed
  sensor (Coospo, Magene, ... — anything speaking the standard Cycling Speed
  and Cadence profile, service `0x1816` / CSC Measurement `0x2A5B`). New
  files: `main/ble_speed_client.c` (central client + wrap/reset-safe CSC
  parser), `main/speed_sensor.c` (source setting, rev→km/h conversion, local
  trip/odometer integrators + their NVS persistence in namespace `spdsns`),
  `custom/speed_screen.c` (settings screen).
- **Settings → Speed sensor → Open**: pick the speed source (VESC vs BLE
  sensor — an explicit switch, no auto-fallback), pair/forget the sensor
  (same address-binding flow as the PAS cadence sensor), set the wheel
  diameter (the shared `wheel_mm` setting, now with a debounced NVS commit so
  +/- taps don't stall LVGL), and watch live speed / trip / odometer /
  sensor battery.
- With the BLE source selected, dashboard **speed, trip and odometer** come
  from wheel revolutions × circumference — they keep updating even when the
  VESC is silent (the "ESC NOT CONNECTED" banner still reflects the VESC).
  The AA video HUD speed and the trip-log speed samples follow the same
  source; the trip-log idle detector (which gates flash-erase trickling)
  now requires BOTH sources to look idle before erasing. The local odometer
  is stored in NVS (saved on ride stops + a 10 min fallback, on a dedicated
  writer task) and is never resettable; "Reset trip" zeroes the BLE trip
  alongside the VESC one.
- Both BLE sensors (cadence + speed) can be bound at once: a new
  `main/ble_central_arb.c` shares NimBLE's single connect-initiator between
  them (one bound sensor behaves exactly as before; with two, connect
  attempts rotate on 8 s windows). `CONFIG_BT_NIMBLE_MAX_CONNECTIONS` is now
  4 — delete the stale per-board `build_*/sdkconfig` before rebuilding.
- NOT yet hardware-verified: CSC parsing against a real Coospo, dual-sensor
  coexistence, and the BLE-source trip/odometer accuracy.

## v1.3.6 / app 0.3.6 — 2026-08-05

### One navigation strip across the on-device web UI

- The three built-in pages — `/ota` (firmware update), `/files` (file manager)
  and `/lisp` (LISP editor) — now share an identical tab strip, so they read as
  one interface instead of three unrelated pages. Markup and CSS live in
  `main/web_nav.h`; the current tab is marked by appending `on` to its class.
- No JavaScript: every tab is a plain link to a sibling page, which costs
  nothing and survives a reload.
- The LISP editor is a standalone gzipped `.html` and cannot include a C
  header, so it carries its own copy of the strip — the header says so, and the
  two have to be kept in step by hand.

### Line-by-line Russian walkthrough of the LISP script

- `lisp/main.ru.lisp`: the same code as `main.lisp` with an explanation above
  every line. Documentation only — **`main.lisp` is what gets flashed**, and
  this copy has to be updated by hand when the real script changes.

## v1.3.5 / app 0.3.5 — 2026-08-05

### The display no longer dies on the CAN bus (and neither does the helper)

- `comm_can.c` never handled **TWAI bus-off**. Past 255 transmit errors the
  controller stops, and it does not come back on its own: recovery has to be
  requested, and after 128 sequences of 11 recessive bits the driver waits in
  STOPPED for an explicit start. Without those two calls a node that hit bus-off
  was simply gone from the bus until its power was cycled — every send returned
  silently, with nothing in the log.
- Found while chasing dead pedal assist: the **C3 BLE helper** lost PAS, the
  throttle toggle and its state queries all at the same instant, and only a
  power cycle brought them back. Same bug, same file — the helper's fix ships as
  its `v1.0.8`.
- `can_health_check()` now runs every 500 ms from the receive task (which
  already wakes on a 10 ms timeout, so no new task): bus-off → recovery,
  STOPPED → restart, both logged.

### Why the bus gets there

A trace of the wire counted **35 615 bus errors** with our own
`tx_error_counter` pinned at 128, on a 1 Mbit/s bus with three nodes. Recovery
removes the permanent death, not the errors. If the recovery counter climbs in
normal use, the bus itself wants attention — 500 kbit/s is the VESC default and
has twice the timing margin.

### CAN polling hold-off

- New `CONFIG_VESC_CAN_POLL_START_DELAY_MS`, default **4000**: the display stays
  off the bus for the first four seconds, while the other nodes and the VESC's
  LISP script come up. Cheap insurance, not the fix — note that it delays only
  the *polling*; the TWAI controller itself joins the bus at `comm_can_start()`
  and participates in ACK and error signalling from that moment.

### Bundled helper firmware

- Now **1.0.8** (was 1.0.6 in the 0.3.4 APK, which predated the helper release).
  `stage_firmware_asset.sh` pulls it from the helper repo's latest GitHub
  release at build time, so cutting this release after publishing the helper's
  is what keeps the two in step.

## v1.3.4 / app 0.3.4 — 2026-08-03

No firmware or app code changed in this repository — this release exists to ship
the enclosure models and to cut a numbered build of the current tree.

### Enclosure models for the Guition board

- `3d-model/esp32-p4-wifi6-guiton-800x480/` — main body, insert, u-holder and a
  TPU cover, plus the STEP source. The Waveshare models move into
  `3d-model/esp32-p4-wifi6-waveshare/`, so the directory now names its board
  instead of implying there is only one.

### Related: the PAS chain fix landed in the helper, not here

- The pedal-assist regression traced to the **C3 BLE helper** (separate repo,
  firmware `v1.0.7`): its single boot-time `REQ_STATE` missed while the VESC was
  still loading `main.lisp`, and with periodic polling off by design the miss was
  permanent — hence "no data from vesc" in the app, and a throttle toggle that
  flipped `throttle-on` from a guess, which coasts the motor arbiter
  (`lisp/main.lisp:451`) with a healthy cadence sensor.
- Nothing on the P4 side was involved, so this app build carries the same P4
  firmware as 1.3.3. The helper image is fetched from the helper repo's GitHub
  releases at update time, not bundled here.

## v1.3.3 / app 0.3.3 — 2026-08-03

### New: three concept dashboard themes

- **Classic Max**, **Lamborghini** and **Supermoto**, plus a reference screen,
  generated by the new `tools/build_classic_max.py` and
  `tools/build_concept_dashboards.py`.
- The amber dashboard is gone, and with it the DSEG7 / DSEG14 segment fonts: no
  screen references them any more, so they are neither compiled from
  `generated/guider_fonts` nor re-subset into the build. The `.ttf` files stay
  in `import/font/` and the CMake comment says what to re-add to bring them
  back. Antonio gains a size-100 subset.
- The simulator's `dashboard_themes_auto.c` is generated from `gui_guider.h` by
  `scripts/gen_dashboard_themes.py`, so it is now gitignored as a build artifact.

### The AI assistant can no longer ship a dead panel control

- Asked to add on-screen profile switching, the assistant emitted
  `(pu8 1) (pu8 8) (pstr "Profile")` — `<id>` and `<type>` swapped, so the type
  byte read 8. An unknown type has no known tail length, so the P4 stops
  decoding and throws away the **rest of the frame**: the row silently never
  rendered, with nothing but an `ESP_LOGW` on the head unit's own console to
  show for it. The protocol lived only in `lisp/README.md`, which the model is
  never shown, and the linter knew nothing about it — so the invariant was
  enforced nowhere.
- The prompt now carries the panel as a fails-silently section (id-then-type,
  the four control types, the count byte in both senders, one id across all
  three functions, the 16-control cap, and that a panel change is three edits),
  the full byte layout, copy-paste templates for all four types, and the
  radio-group pattern — there is no 1-of-N control, so a profile selector is N
  toggles over one variable.
- The linter gates flashing on nine new checks: unknown control type, a count
  byte that disagrees with the rows listed, duplicate ids, ids that only
  `panel-send-state` or `panel-action` know about, over-long labels and unit
  suffixes (a label ≥ 40 bytes leaves the P4 decoder stopped mid-string and
  everything after it is misread), and a frame wider than its `bufcreate`.
- False positives were the design constraint — a gate that cries wolf gets
  switched off, taking the real checks with it. An unrecognised shape produces
  **no** diagnostic: broken parens skip the pass, every emit in a frame must
  share one parent (so rows emitted inside an `if` or a loop are left alone), a
  foreign call between emits abandons the frame (helper emitters), and
  non-literal arguments disable only the rule that needed the value.
- Also written down for the model: why the arbiter ticks at 100 Hz (at 20 Hz the
  ramp advanced in 12.5 %-of-max steps that FOC executes instantly — the rider
  felt jerks) and why `app-disable-output` is refreshed every tick rather than
  once (if the script dies the stock throttle returns ~1.5 s later and the bike
  stays rideable).

### Pedal assist: one sender at a time

- Two nodes may stream PAS setpoints, and the head unit's own PAS idles at 0 A,
  20 Hz, forever. Interleaved with a real assist current those zeros chopped the
  setpoint into 3 A → 0 → 3 A and the motor jerked. The script now locks onto
  whoever sent the last non-zero setpoint and ignores the others until that
  source goes silent or sends zero; a 0.4 s staleness check releases the lock
  too.
- The BLE helper can toggle the throttle master switch atomically (`msg 0x06`,
  it never needs to know the current state), and its buttons arrive as plain
  standard-id CAN frames on `0x123` mapped to throttle-toggle and
  profile-switch.

## v1.3.2 / app 0.3.2 — 2026-08-02

### New: LISP editor in the browser

- `http://android-auto.local/lisp` — a full LispBM editor on the head unit's
  own HTTP server, next to `/ota` and `/files`. Syntax highlighting, line
  numbers, matching and rainbow parens, find/replace, block indent, hotkeys.
- The linter encodes the failures that cost the most time here: paren balance,
  unterminated strings, `@const-start` / `@const-end` pairing, defuns left
  outside the const block, buffers created inside it, and a thread started
  before the function it runs is defined (the handler dies with
  `variable_not_bound` and the feature is simply gone).
- Read VESC / Upload / Upload + Run / Start / Stop over CAN with a progress
  bar. The transfer is asynchronous — an upload takes tens of seconds and the
  server serves everything from one task, so blocking it would freeze `/files`
  and `/ota` too.
- Live console of the script's `(print ...)` output. The firmware never parsed
  `COMM_LISP_PRINT` / `COMM_PRINT` before; the packets were arriving all along.
- A REPL line evaluates an expression without reflashing the script (capped at
  240 bytes — one CAN buffer transfer).
- The script library on the device (`/vescfs/lisp` and microSD) is browsable
  from the same page: open, save, rename, **move**, delete, mkdir, upload.
- `scripts/lisp_web_mock.py` serves the page against a fake device, so the
  editor can be worked on without a board.

### CAN identity

- The head unit answers `COMM_FW_VERSION` on the bus, so VESC Tool's CAN scan
  lists it as **Super VESC Display** instead of "Unknown". It replies to that
  one request only — it is not pretending to be a motor controller (the scan
  icon comes from `HW_TYPE_CUSTOM_MODULE`).

### Traction control in the LISP arbiter

- The native VESC traction-control algorithm, ported into the arbiter as a
  0..1 current scale. It cannot be left to the ADC app: that app's control
  block sits behind `app_is_output_disabled()`, which the arbiter keeps
  asserted, so the app's own TC never runs.
- On/off and the threshold are the real appconf fields `adc-tc` /
  `adc-tc-max-diff` — the panel toggle, the on-device VESC Tool menu and VESC
  Tool itself are one setting. `tc-peer` (the other motor's CAN id) must be set
  by hand; 255 disables the limiter, which is the safe default.
- Profiles are now also selectable directly from the on-screen panel
  (`panel-set-profile`), a radio group of three. The tune/melody controls are
  gone, and with them the ~2 kB quoted literal that cost cons heap at load.

### Reference and docs

- `conf-get`'s second argument is a "read the firmware defaults" **flag**, not a
  fallback value, and `conf-store` writes both mcconf and appconf from live RAM
  — in a script that scales limits at runtime that bakes temporary values over
  the rider's master config. Both are now documented in the assistant's LISP
  reference and in `lisp/README.md`.

## v1.3.1 / app 0.3.1 — 2026-07-31

30 commits, 130 files, +27 929 / −8 284 since v1.2.35.

### New: AI assistant for LISP

- An "Assistant" tab in the LISP editor. It reads the script off the VESC,
  edits it with anchored search/replace patches, runs the linter, flashes only
  on an explicit tap, and then **verifies on hardware** — doneCtx, heap trend,
  sentinel globals, print output. The protocol acks only prove the bytes
  reached flash; they succeed even when the script is dead, so they are not
  trusted.
- Every flash and every script start needs a tap; stopping a script never does.
  Provider is OpenRouter (or DeepSeek directly), the key is yours and lives in
  the system keystore.
- A VESC LispBM reference ships in the prompt — motor commands, config, CAN,
  buffers, events — taken from the upstream bldc docs, plus this project's own
  rules and worked examples.

### New: ESP32-C3 BLE helper configurator

- Status, parameters, pairing and firmware screens: cadence gauge, assist
  current and levels, binding of buttons and the cadence sensor (the helper
  itself does the scanning), per-button CAN commands, PAS tuning.
- Runs as a second GATT link from the background isolate, so the head-unit
  connection stays up while you configure the helper.
- The helper's firmware is pulled from its GitHub releases at build time and
  bundled into the APK, so it can be flashed with no network.

### VESC console and LISP linter

- Script `(print ...)` output is finally visible on the phone. The bytes always
  arrived — the head unit's bridge forwards every packet — but the link layer
  dropped anything that matched no pending request.
- The linter encodes the rules that fail SILENTLY on hardware: one `@const`
  block, mutable `def`s and `bufcreate` above it, and no forward references
  from top-level statements (not "spawns last" — `main.lisp` legitimately
  spawns from the middle of its const block).
- Syntax highlighting in the editor and in the assistant's code blocks.

### LISP and motor control

- LISP editor in the app over any NUS link: the head unit's bridge, a
  stand-alone VESC BLE adapter, or the helper.
- Upload format now matches VESC Tool's CodeLoader. WRITE offsets include the
  8-byte flash header the VESC validates at startup, so raw uploads flashed
  fine and then never loaded. Limit raised 16K → 120K, pollers pause during a
  transfer, faster BLE reads.
- The motor is owned by a single current-based arbiter in LISP (100 Hz):
  smooth ramps, brake slew, no step to zero. The stock ADC app stays configured
  but its output is suppressed by a rolling `app-disable-output` — if the
  script ever dies the motor stops on the command timeout and the stock
  throttle comes back in ~1.5 s.
- Tuning moved out of the quick panel: ramp times are read live from VESC Tool,
  cruise PI gains live in the script.

### Pedal assist (PAS)

- Entirely on the head unit: a BLE central link to the cadence sensor, the PAS
  loop, and a current setpoint handed to the arbiter on the VESC. The phone is
  not involved.
- Quiet on CAN unless actually assisting. PAS_SET frames used to stream at
  ~20 Hz forever, even with no sensor connected.

### Display and settings

- 180° screen rotation for upside-down mounting, done at render level:
  panel-level mirroring on the ST7701 produces stripes because the DPI scan
  order is fixed — the data has to be flipped, not the panel.
- Hold-to-repeat on the +/− step buttons in settings.
- "Display CAN ID" row — the head unit's own CAN node id. The backend existed;
  its UI had been left commented out.
- throttle-curve range corrected to −100..100%. An audit of 985 parameters
  across three firmware versions found this to be the only display-range
  divergence.

### BLE and stability

- Advertising continues while a peer is connected, so VESC Tool can find the
  head unit alongside the phone app. The third NimBLE slot stays reserved for
  the cadence sensor.
- The NUS→CAN bridge targets the VESC id from settings instead of a
  compile-time default. Anyone whose VESC sits on a different CAN id had a
  working dashboard but VESC Tool over BLE timing out against a node that does
  not exist.
- No more screen stalls while riding: the battery tracker's NVS commits moved
  off the LVGL thread and the trip log is trickled out over time.
- The app's whole BLE stack moved into the foreground-service isolate;
  reconnect hardened.

### Tooling

- `scripts/build_app.sh` builds the APK with **no embedded API key** by
  default; `--with-key` masks the key from `.env` into the build.
- `scripts/release.sh` aborts if a key ends up in the APK anyway. The check
  reads the decompressed zip entries — grepping the `.apk` itself always
  reports "clean" even when the key is sitting in `libapp.so`.
- `scripts/build_board.sh` uses each build directory's own Python venv, so both
  boards build in one run. Releases used to stop at the first board.

## Earlier releases

Shipped between v1.2.35 and v1.3.1; cut as release commits but not tagged.

| Version | Date | Summary |
|---|---|---|
| 1.2.44 / 0.2.44 | 2026-07-27 | BLE bridge targets the configured VESC id |
| 1.2.43 / 0.2.43 | 2026-07-27 | Display CAN ID in settings |
| 1.2.42 / 0.2.42 | 2026-07-21 | PAS merged to main; CAN-quiet PAS; VESC Tool alongside the app |
| 1.2.41 / 0.2.41 | 2026-07-04 | Deferred battery NVS, trickled trip log — no flash stalls while riding |
| 1.2.40 / 0.2.40 | 2026-07-06 | Hold-to-repeat on the +/− step buttons |
| 1.2.39 / 0.2.39 | 2026-07-05 | 180° flip fixed at render level (reboot to apply) |
| 1.2.38 / 0.2.38 | 2026-07-05 | 180° screen flip option; on-device pedal assist (2026-06-30) |
| 1.2.37 / 0.2.37 | 2026-06-29 | Companion BLE moved into the foreground-service isolate; reconnect hardening |
| 1.2.36 / 0.2.36 | 2026-06-23 | VESC config throttle-curve range −100..100% |

## v1.2.35 / app 0.2.35 — 2026-06-22

LISP quick-action panel with cruise control and profiles, LVGL partial-render
performance work, battery charge/swap detection by voltage.
