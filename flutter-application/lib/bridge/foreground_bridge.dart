import 'package:flutter/services.dart';

/// Drives the Android foreground service that keeps the BLE link alive
/// when the app is backgrounded. No-op on iOS.
class ForegroundBridge {
  static const _channel = MethodChannel('aabridge/foreground');

  Future<void> start() => _channel.invokeMethod('start');
  Future<void> stop() => _channel.invokeMethod('stop');
}
