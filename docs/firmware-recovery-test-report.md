# Recovery verification — 2026-11-20

Scope: firmware 1.3.50, companion app 0.3.60+124. This worktree also contains
parallel dashboard changes; they were preserved. Recovery implementation and
physical fault-test procedure: [firmware-recovery.md](firmware-recovery.md).

## Completed automated checks

| Check | Result |
| --- | --- |
| Native C policy tests, TinyCC 0.9.27 x64, `-Wall -Werror` | PASS |
| Production display backup/reset C code against memory NVS | PASS |
| Flutter recovery protocol + existing BLE tests | 29 PASS |
| Flutter analysis of eight modified source/test files | No issues |
| Full application analysis | Existing unrelated warnings/info remain; no compile errors |
| JC4880 firmware build / partition size check | PASS; 4,717,472 bytes, 10% slot free |
| Waveshare firmware build / partition size check | PASS; 4,721,744 bytes, 43% slot free |
| Waveshare rev.3 firmware build / partition size check | PASS; 4,699,456 bytes, 43% slot free |
| `esptool image_info --version 2`, all three images | Version 1.3.50; checksum and validation hash valid |
| Release APK build | PASS; 0.3.60+124, 34,562,369 bytes |
| APK ZIP CRC + embedded firmware SHA-256 comparison | PASS; all three exact images; S3 not bundled |
| APK signature and package metadata | PASS; same certificate as local 0.3.59 APK |

APK: `build/aa_bridge-0.3.60-124.apk`.
SHA-256: `647d1892c1ed4cb27594d0406f8c92e438754cd2799fe5e3c8543e3d32cc5e63`.

The native policy suite covers a three-fault streak, saturation, new-image,
unarmed and ordinary reset handling, startup timeout, fresh/stale task ticks,
headless/intentional-UI-pause cases, grace periods, command lengths, protocol
versions, opcode permissions and unsigned per-boot nonces.

The NVS suite calls the actual production implementation. It verifies safe
values, backup durability before settings writes, repeated reset without
overwriting the original backup, restoration of absent keys, unrelated motor
key preservation, failed backup commit, interruption after partial settings
writes, retry/restore after interruption and malformed backup refusal.

Detailed logs and host test executables are in the ignored
`build/recovery-work/` directory. Reproduce native tests with
`tools/tests/run_boot_guard_tests.ps1 -Compiler <path-to-tcc-or-gcc>`.

## Physical tests

**NOT RUN:** no ESP32 head unit or Android phone was connected. Windows exposed
only virtual Bluetooth COM7/COM8. In particular, a successful build or simulated
heartbeat test does **not** prove a real watchdog reboot, OTA-slot boot,
Bluetooth reconnection in headless mode, LCD behaviour or preservation of real
on-device trip data. Execute the hardware matrix in the procedure before
considering recovery validated on hardware.
