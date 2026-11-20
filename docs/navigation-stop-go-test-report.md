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

**NOT RUN:** uncontended replay of the fix, actual LCD frame intervals/video,
real Android GPS ride, NORTH/AUTO turns on the new firmware and cold-cache
stress. The user was asked to stop the phone feed before these dependent tests.
No new firmware was flashed during this run. Builds and native tests do not
establish absence of pauses on the physical screen.
