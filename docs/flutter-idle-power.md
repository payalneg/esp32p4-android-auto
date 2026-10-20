# Flutter idle power investigation

Checked `main`, app 0.3.41+101, on an Android 16 phone on 2026-10-20.
The head unit was unavailable, so this does not validate reconnection to real
hardware or quantify the change in battery drain.

## Confirmed on the installed app

After launching the app and returning to Home with no head unit nearby,
`dumpsys power` still showed the app's `ForegroundService:WakeLock` held after
20 seconds. The location request had disappeared by that point. This confirms
the unconditional CPU lock independently of GPS usage.

Code also kept the native media ticker/album-art encoding active while
disconnected, left the clock timer running after a lost link, and restarted
the OS autoConnect request roughly every 40 seconds indefinitely.

## Changes

- The foreground-task plugin's unconditional wake lock is disabled. Native
  `LinkPower` holds a lock only for a connected head unit and releases it on
  disconnection or task destruction. Its lifecycle is independent of the UI.
- On the first visible launch after an update, old persisted plugin options
  are migrated once. A service restart releases a legacy plugin lock; merely
  updating task options in flutter_foreground_task 8.17.0 does not release it.
- The native media listener detaches its controllers and timer while the link
  is absent. Reconnection attaches again and immediately sends current media.
- Notifications are discarded before icon rendering while disconnected.
- Clock and greeting timers are cancelled outside the connected state.
- Manual autoConnect recovery backs off through 40, 80, 160 and 300 seconds.
  The OS request remains pending between retries; the five-minute cap is not
  a five-minute delay imposed on normal OS reconnection.
- A location permission check completing after the app was hidden cannot
  start GNSS without a connected head unit or active navigation. A background
  GPS subscription is detached when neither needs it, even if its start raced
  the lifecycle callback.

Active standalone navigation is allowed to keep GPS; waiting for an absent
head unit with no active route is the idle case.

## Checks performed

- 25 Flutter tests passed: reconnect backoff, head-unit feed and navigation
  controller. The backoff tests include an hour away from the head unit and
  reset after a successful connection.
- Flutter analysis of the changed Dart code and new tests reported no issues.
- Arm64 Android debug and release APKs built successfully, including the native Kotlin
  power/media changes. Dependencies were taken from the committed lockfile;
  missing firmware assets for this isolated build were recovered from the
  repository's existing 0.3.41 APK.
- The patch was checked against a clean archive of the original commit.

The original signing key was on an unavailable computer. With the user's
explicit approval, the old app was uninstalled and the updated app installed
with a new key. Its accessible external files were backed up and restored;
private settings, maps, chat history and API keys were reset by uninstalling.
The new signing key is retained outside the repository at
`%USERPROFILE%\.android\debug.keystore` for subsequent updates.

The updated debug app was launched and backgrounded on the phone. Its BLE
foreground service remained running, with neither app CPU wake lock held and
no active app location request. Its log showed a successful service start and
idle connection state, without Flutter channel errors. It was then updated to
the release APK using the same new key, without another data reset.
The existing Android 16 location-FGS startup failure recurred on package
replacement. After force-stopping and explicitly launching the release app
with its Activity visible, its foreground service started successfully. This
is the separate startup limitation described below, not an idle-power fix.

## Device validation still required

Launch the updated app once, return to Home, and leave the head unit off.
Check that neither `ForegroundService:WakeLock`
nor `aabridge:connected` is held, and that the app has no active location
registration. Measure screen-off drain unplugged over a longer interval;
USB charging and an inactive old service are not a useful before/after test.
After the reinstall the app has no saved device: repeat with the head unit
paired and then switched off, to cover the pending autoConnect case as well.

Then power the head unit on without opening the app. Verify reconnection,
current music/cover, map transmission, and release of the lock/GPS after the
head unit goes away again. Also test screen-off navigation and file transfers.

## Separate issues

The phone's crash history contains repeated `SecurityException` failures when
automatically starting the combined `connectedDevice|location` foreground
service from the background with only while-in-use location permission.
This startup problem is not fixed by the idle-power change. See Android's
[foreground service restrictions](https://developer.android.com/develop/background-work/services/fgs/restrictions-bg-start).

The navigation feed and routing controller are still owned by the UI engine,
whereas music and BLE are hosted by the foreground-service engine. The report
that a map needs the app opened after reconnection still needs reproduction
with the head unit; its exact cause was not established in this session.
