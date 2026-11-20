# Navigation stop/go regression — 2026-11-20

Firmware 1.3.51; app 0.3.61+125. Other dashboard/recovery changes already in
the shared worktree were preserved. Logs: `build/nav-test-runs/stop-go-20261120/`.

## Reproduced causes

1. A 1 Hz GPS fix is repeated by the 2 Hz VIEW sender. Reassigning the target
   on each repeat rewinds the already advancing estimate. The native regression
   holds the other motion arithmetic constant and enables the old unconditional
   target reset: 58 stopped/backward steps over 20 seconds at a 20 ms model
   cadence; ignoring repeated anchors gives zero. These are model steps, not
   measured LCD frames.
2. The absolute Mercator centre was calculated in float at tens of millions
   of world pixels. On a 6 m/s northbound trajectory at latitude 50.0619,
   zoom 18, sampled every 60 ms, the old formula leaves the same pixel centre
   for up to 300 ms. The production double-centre helper reduces this to 60 ms
   (ordinary whole-pixel rounding). Local per-point drawing stays float.
3. The ride simulator advanced by the nominal timer period even when a timer
   fired late, while advertising the full speed. It now uses monotonic elapsed
   time, starts at the route origin and emits zero speed at the destination.

Incoming VIEWs also now use a protected latest-value slot. The map worker
applies them before rendering, including when the wake-event queue is full.
Old queued STOP events cannot override a later resumed VIEW. This integration
is compiled; a clean hardware stress replay is still required.

## Completed checks

| Check | Result |
| --- | --- |
| Native production motion helpers, `-Wall -Werror` | PASS |
| 36 straight rides: four headings, three speeds/cadences, GPS 1/2/5 Hz, VIEW 2 Hz | Forward progress and distance accuracy PASS |
| Repeated stale fix with old positive speed for 10 s | Prediction bounded to 1.5 s of travel; PASS |
| Production projection vs double reference, zoom 14–18 | Maximum centre Y error 0.49 px; PASS |
| Simulator, controller, head feed and BLE tests | 58 PASS |
| Analysis of modified simulator source/test | No issues |
| JC4880 / Waveshare / Waveshare rev.3 builds | PASS, all version 1.3.51 |
| All three firmware image checksums / validation hashes | PASS |
| Release APK ZIP CRC / embedded image hashes / signature / version | PASS; 0.3.61+125 |

Artifact: `build/aa_bridge-0.3.61-125.apk` (34,579,049 bytes).
SHA-256: `4a9db5da692e609d7284a5e3089172bdace310ce644170750e3cd5ed91048edd`.

Reproduce native checks with a host C compiler (`-lm` on Linux):

```text
tcc -Wall -Werror -o build/test_nav_motion.exe tools/tests/test_nav_motion.c
build/test_nav_motion.exe
tcc -Wall -Werror -o build/nav_proj_check.exe scripts/nav_proj_check.c
build/nav_proj_check.exe
```

The TinyCC Windows-only test shims round standard double math results back to
float because its old msvcrt lacks float trig exports; firmware uses ESP-IDF's
normal math library. No test-only branches are added to production motion code.

## Physical replay status

The connected JC4880 reported installed firmware **1.3.50**. Two Windows BLE
setup attempts were inconclusive: the phone was also publishing navigation,
and the UART log repeatedly showed `bridge re-bound, conn=1` followed by
`bridge re-bound, conn=0`. Route setup returned TRUNCATED, so these attempts
were **not** counted as a successful motion/FPS test. The screen mode was
returned to VESC and the PC connection closed.

After the user stopped the feed, `before-exclusive` completed on 1.3.50:
112 VIEW packets, GPS coordinates updated at 1 Hz and repeated at 2 Hz,
5 m/s, stationary/NORTH/AUTO-turn/stopped phases and a six-second packet gap.
There was no ownership flapping during this run. This was a Windows BLE
display replay, bypassing Android; the RAM tile cache was not empty, so it is
not a controlled cold-cache or video comparison.

A subsequent reboot resumed the phone's navigation feed automatically. The
attempt to collect baseline framebuffer images (`before-captured`) again
encountered ownership flapping and TRUNCATED route setup. No frames from that
attempt are counted as evidence of smoothness.

### Installed firmware

The incomplete BLE OTA upload was cancelled during RAM staging, before END;
it did not program a firmware image. For the faster USB update, the actual
partition table and OTA data were read first. They selected `ota_0` at 0x20000.
Its entire 5 MiB image was saved as `installed-1.3.50-slot0.bin`; esptool
validated version 1.3.50, its checksum and validation hash.

Firmware 1.3.51 was written to **ota_1 at 0x520000** over USB; esptool verified
the programmed hash. ESP-IDF's `otatool.py` selected slot 1. The old application
in slot 0 was retained. No bootloader, partition table, settings or trip-log
partition was written by these commands.

Post-boot BLE reads (`running-after.json`) confirmed version **1.3.51**, board
JC4880, running slot `ota_1`, other image 1.3.50, recovery false, safe defaults
false and failure counter 0. The board remained running beyond two minutes.
`after-1.3.51-dashboard.png` is a valid CRC-checked 800×480 framebuffer snapshot
showing the normal dashboard; it does not demonstrate navigation smoothness.

**Historical status before the Android runs below — NOT RUN:** uncontended motion replay of the new firmware, actual LCD frame
intervals/video, real Android GPS ride, NORTH/AUTO turns on the fix and
cold-cache stress. A read-only link check still showed two peripheral links
with the PC connected and one after it disconnected. The user was asked to
turn off Bluetooth on the phone, since stopping guidance did not prevent its
automatic feed from restarting. These dependent comparisons await that setup.
Builds, native tests and a dashboard snapshot do not establish absence of
pauses on the physical navigation screen.

## Subsequent physical phone testing

The Samsung-to-JC4880 runs, rejected attempts, background-producer defect,
arrival correction and measured 1.3.52 / 0.3.63 results are recorded in the
[phone test report](navigation-phone-test-report.md). That report supersedes
the pending Android checks above; LCD camera recording, overnight endurance
and other-board hardware validation remain outside the completed scope.
