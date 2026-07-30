import 'dart:async';
import 'dart:io';

import 'package:flutter/material.dart';
import 'package:flutter_foreground_task/flutter_foreground_task.dart';
import 'package:permission_handler/permission_handler.dart';

import 'app.dart';
import 'ble/ble_proxy.dart';
import 'bridge/foreground_bridge.dart';
import 'i18n/strings.dart';
import 'settings/agent_settings.dart';

Future<void> main() async {
  WidgetsFlutterBinding.ensureInitialized();
  // Must run before addTaskDataCallback / sendDataToTask can reach the
  // background BLE isolate.
  FlutterForegroundTask.initCommunicationPort();
  if (Platform.isAndroid) {
    // Android 13+ blocks every notification until the user grants
    // POST_NOTIFICATIONS at runtime. Ask once on first launch — without this
    // the foreground-service pill silently never appears.
    await Permission.notification.request();
    // Android 14+ refuses to start a `connectedDevice` FGS unless the caller
    // already holds BLUETOOTH_CONNECT. Without this the first startForeground()
    // throws SecurityException and kills the process before the UI paints.
    await [
      Permission.bluetoothConnect,
      Permission.bluetoothScan,
    ].request();
    // Without an ignore-battery-optimizations grant, OEM power managers
    // (Xiaomi/Samsung/Huawei/OnePlus…) reap the foreground service after a
    // while in the background — the BLE link dies and never comes back. Prompt
    // for the exemption until it's granted.
    if (!await Permission.ignoreBatteryOptimizations.isGranted) {
      await Permission.ignoreBatteryOptimizations.request();
    }
  }
  // Wire up the UI side of the port and prime the saved-device id so the home
  // screen shows it at first paint.
  await BleProxy.instance.init();
  // Reads the API key from the system keystore; the editor hides the Assistant
  // tab until this says a key is configured.
  await AgentSettings.instance.load();
  final locale = LocaleNotifier();
  await locale.load();
  // Start the foreground service — this launches the background isolate
  // (ble_host.startBleTask), which owns the connection + notification/media
  // pump and re-arms auto-connect. It survives the UI being closed.
  if (Platform.isAndroid &&
      await Permission.bluetoothConnect.isGranted) {
    unawaited(ForegroundBridge().start());
  }
  runApp(LocaleScope(
    notifier: locale,
    child: const AaBridgeApp(),
  ));
}
