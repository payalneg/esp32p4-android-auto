import 'package:flutter/material.dart';

import 'app.dart';
import 'ble/ble_service.dart';
import 'coordinator.dart';

Future<void> main() async {
  WidgetsFlutterBinding.ensureInitialized();
  await Coordinator.instance.start();
  // Fire-and-forget — if a head unit is saved, kick off the OS-level
  // autoConnect immediately. The UI shows "connecting…" until the link
  // comes up, no scanner step required.
  BleService.instance.resumeIfPaired();
  runApp(const AaBridgeApp());
}
