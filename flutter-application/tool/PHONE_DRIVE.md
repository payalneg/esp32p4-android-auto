# Physical phone navigation replay

This entry point uses the normal application bootstrap, `NavigatorScreen`,
`NavController.attachFixes`, `HeadUnitFeed`, service IPC and Android BLE stack.
Only the route fixture and GPS source are supplied by the harness. It does not
evaluate route optimality, real GNSS reception or background simulation handoff.

Build from `flutter-application` in PowerShell with the usual Java/Android SDK:

```powershell
$env:NAV_TEST_APPLICATION_ID = 'com.aabridge.aa_bridge.navtest'
flutter build apk --release --target-platform android-arm64 -t tool/nav_phone_drive.dart
Remove-Item Env:NAV_TEST_APPLICATION_ID
```

Verify the APK package is `com.aabridge.aa_bridge.navtest` before installing.
The label is **VESC Navigator Test**. Do not uninstall the production package,
clear its data, or try to replace a differently signed installed app. The entry
point refuses to run outside a `.navtest` package.

Temporarily stop the production app to avoid two BLE producers. Give the test
copy Bluetooth, location and notification permissions and a battery exemption.
Launch `com.aabridge.aa_bridge.navtest/com.aabridge.aa_bridge.MainActivity` with
the head attached. The fixture currently connects to bench head
`58:E6:C5:D2:AA:2A`; change that address for a different unit. Select NAV on the
head. The replay waits for the connection, a visible NAV screen and tile cache.

It holds the first point for 20 seconds, then rides Rynek–Kazimierz at 5 m/s,
1× wall time and 1 Hz GPS through `simulateRide`. The final speed is zero.
Each launch starts again. The JSON fixture geometry was saved by the
2026-11-19 route regression; following it checks consumption, not road access.
The normal tile cache may fetch tiles from the network; warm and cold runs
must be identified separately.

For a shorter performance replay, add `--dart-define=PHONE_DRIVE_SECONDS=120`.
After the unchanged first 120 seconds it injects a point eight metres before
the original destination to exercise arrival. This final jump is artificial;
it does not represent a completed ride along the full route.

Events, UTC timestamps, GPS fixes and BLE status are written to
`Android/data/com.aabridge.aa_bridge.navtest/files/phone-drive-*.jsonl` and
`PHONE_DRIVE` logcat lines. Save those alongside head UART logs/frame captures,
the source revision, APK hash, firmware slot/version and test conditions.
Frame capture can itself affect timings and is not a camera recording of LCD
refresh. Stop the test copy afterwards before returning to normal navigation.
