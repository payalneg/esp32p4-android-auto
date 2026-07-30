import 'dart:io' show Platform;

import 'package:flutter_foreground_task/flutter_foreground_task.dart';

import '../ble/ble_host.dart' show startBleTask;

/// Keeps the app process alive while a BLE link to the head unit is wanted.
///
/// Backed by flutter_foreground_task's sticky foreground service: it holds a
/// wakelock, auto-restarts after the OS kills it and re-runs on boot — far
/// harder for OEM battery managers to reap than a bare Service. The service's
/// background isolate ([startBleTask] in ble_host.dart) OWNS the whole BLE
/// stack, so the link survives the UI being backgrounded or killed. The
/// persistent notification doubles as a connection-status pill.
///
/// No-op on iOS (its background-BLE model is different and out of scope).
class ForegroundBridge {
  static bool _inited = false;

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
      foregroundTaskOptions: ForegroundTaskOptions(
        // No periodic Dart event — the service exists only to keep the
        // process alive; the BLE link lives in the app isolate.
        eventAction: ForegroundTaskEventAction.nothing(),
        autoRunOnBoot: true,
        autoRunOnMyPackageReplaced: true,
        allowWakeLock: true,
        allowWifiLock: false,
      ),
    );
    _inited = true;
  }

  Future<void> start() async {
    if (!Platform.isAndroid) return;
    _ensureInit();
    if (await FlutterForegroundTask.isRunningService) return;
    // `callback` runs in the service's background isolate — it owns the BLE
    // connection + pump (see ble_host.dart).
    await FlutterForegroundTask.startService(
      serviceId: 7242,
      notificationTitle: 'VESC Display Tool — disconnected',
      notificationText: 'Waiting for head unit…',
      callback: startBleTask,
    );
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
