import 'package:flutter/material.dart';

import 'app.dart';
import 'coordinator.dart';

Future<void> main() async {
  WidgetsFlutterBinding.ensureInitialized();
  await Coordinator.instance.start();
  runApp(const AaBridgeApp());
}
