import 'dart:async';

import 'package:flutter/material.dart';

import 'ble/ble_proxy.dart';
import 'i18n/strings.dart';
import 'ui/home_screen.dart';

class AaBridgeApp extends StatefulWidget {
  const AaBridgeApp({super.key});

  @override
  State<AaBridgeApp> createState() => _AaBridgeAppState();
}

class _AaBridgeAppState extends State<AaBridgeApp> {
  late final AppLifecycleListener _lifecycle;

  @override
  void initState() {
    super.initState();
    // The BLE stack runs in the foreground-service isolate and only pushes its
    // state on change, so a UI that was backgrounded (or killed and recreated)
    // can hold a stale snapshot. Re-pull on every return to the foreground —
    // see BleProxy.syncStatus().
    _lifecycle = AppLifecycleListener(
      onResume: () => unawaited(BleProxy.instance.syncStatus()),
    );
  }

  @override
  void dispose() {
    _lifecycle.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    final locale = LocaleScope.of(context).locale;
    return MaterialApp(
      title: 'VESC Display Tool',
      debugShowCheckedModeBanner: false,
      locale: locale,
      supportedLocales: supportedLocales,
      theme: ThemeData(
        useMaterial3: true,
        colorScheme: ColorScheme.fromSeed(
          seedColor: const Color(0xFF1B6CFF),
          brightness: Brightness.dark,
        ),
      ),
      home: const HomeScreen(),
    );
  }
}
