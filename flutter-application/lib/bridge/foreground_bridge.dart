import 'dart:io' show Platform;

import 'package:flutter/services.dart';
import 'package:flutter_foreground_task/flutter_foreground_task.dart';
import 'package:permission_handler/permission_handler.dart';
import 'package:shared_preferences/shared_preferences.dart';

import '../ble/ble_host.dart' show startBleTask;
import '../log/app_log.dart';

/// Keeps the app process alive while a BLE link to the head unit is wanted.
///
/// Backed by flutter_foreground_task's sticky foreground service: it holds a
/// connection-scoped wakelock and auto-restarts after the OS kills it or on
/// boot. The service's background isolate ([startBleTask] in ble_host.dart)
/// owns the whole BLE
/// stack, so the link survives the UI being backgrounded or killed. The
/// persistent notification doubles as a connection-status pill.
///
/// No-op on iOS (its background-BLE model is different and out of scope).
class ForegroundBridge {
  static bool _inited = false;
  static const _power = MethodChannel('aabridge/link-power');
  static const _idlePowerConfigured = 'ble_idle_power_v1';

  static ForegroundTaskOptions get _taskOptions => ForegroundTaskOptions(
        eventAction: ForegroundTaskEventAction.nothing(),
        autoRunOnBoot: true,
        autoRunOnMyPackageReplaced: true,
        // Version 8 does not apply lock changes through updateService().
        // Native LinkPower owns the lock for established links only.
        allowWakeLock: false,
        allowWifiLock: false,
      );

  /// Version 8 restores its old options from disk after an APK update. Updating
  /// those options alone does not release an already-held plugin wakelock, so
  /// migrate and restart once while the Activity is visible (location FGS
  /// restarts from the background are restricted on Android 14+).
  Future<void> _migrateIdlePower() async {
    final prefs = await SharedPreferences.getInstance();
    if (prefs.getBool(_idlePowerConfigured) == true) return;
    final updated = await FlutterForegroundTask.updateService(
        foregroundTaskOptions: _taskOptions);
    if (updated is! ServiceRequestSuccess) {
      AppLog.w('svc', 'could not configure idle power: $updated');
      return;
    }
    await prefs.setBool(_idlePowerConfigured, true);
    final restarted = await FlutterForegroundTask.restartService();
    if (restarted is ServiceRequestSuccess) return;
    await prefs.remove(_idlePowerConfigured);
    AppLog.w('svc', 'could not restart for idle power: $restarted');
  }

  static void _ensureInit() {
    if (_inited) return;
    FlutterForegroundTask.init(
      androidNotificationOptions: AndroidNotificationOptions(
        channelId: 'aabridge.link',
        channelName: 'VESC Display Tool link',
        channelDescription: 'Keeps the head-unit link alive',
        // LOW: a silent status pill, not a heads-up alert.
        channelImportance: NotificationChannelImportance.LOW,
        priority: NotificationPriority.LOW,
        onlyAlertOnce: true,
      ),
      iosNotificationOptions: const IOSNotificationOptions(),
      foregroundTaskOptions: _taskOptions,
    );
    _inited = true;
  }

  Future<void> start({bool migrateIdlePower = false}) async {
    if (!Platform.isAndroid) return;
    // The service declares the location type — that is what keeps the
    // navigator's fixes coming with the app in a pocket — and Android 14
    // throws SecurityException out of startForeground() when the permission
    // is not held by then. That throw happens inside the service and takes
    // the process with it, so it cannot be caught in Dart: a phone where
    // location was refused gets no background service rather than a crash
    // loop. (A grant revoked after the fact still bites on the next boot,
    // where autoRunOnBoot starts the service with no Dart of ours involved.)
    if (!await Permission.locationWhenInUse.isGranted) {
      AppLog.w('svc', 'foreground service not started: no location permission');
      return;
    }
    _ensureInit();
    if (await FlutterForegroundTask.isRunningService) {
      if (migrateIdlePower) await _migrateIdlePower();
      return;
    }
    // `callback` runs in the service's background isolate — it owns the BLE
    // connection + pump (see ble_host.dart).
    final started = await FlutterForegroundTask.startService(
      serviceId: 7242,
      notificationTitle: 'VESC Display Tool — disconnected',
      notificationText: 'Waiting for head unit…',
      callback: startBleTask,
    );
    if (started is ServiceRequestSuccess) {
      final prefs = await SharedPreferences.getInstance();
      await prefs.setBool(_idlePowerConfigured, true);
    }
  }

  Future<void> stop() async {
    if (!Platform.isAndroid) return;
    if (await FlutterForegroundTask.isRunningService) {
      await FlutterForegroundTask.stopService();
    }
  }

  /// Pushes a fresh status into the persistent shade notification.
  /// [connected] swaps the title between connected/disconnected variants;
  /// [subtitle] is the second line (device name / "Connecting…").
  Future<void> setStatus({required bool connected, String subtitle = ''}) async {
    if (!Platform.isAndroid) return;
    await _power.invokeMethod<void>('setConnected', {'connected': connected});
    if (!await FlutterForegroundTask.isRunningService) return;
    await FlutterForegroundTask.updateService(
      notificationTitle:
          connected
              ? 'VESC Display Tool — connected'
              : 'VESC Display Tool — disconnected',
      notificationText: subtitle.isNotEmpty
          ? subtitle
          : (connected ? 'Linked to head unit' : 'Waiting for head unit…'),
    );
  }
}
