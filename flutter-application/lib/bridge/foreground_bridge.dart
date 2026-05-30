import 'package:flutter/services.dart';

/// Drives the Android foreground service that keeps the BLE link alive
/// when the app is backgrounded. No-op on iOS.
class ForegroundBridge {
  static const _channel = MethodChannel('aabridge/foreground');

  Future<void> start() => _channel.invokeMethod('start');
  Future<void> stop()  => _channel.invokeMethod('stop');

  /// Pushes a fresh status into the persistent shade notification.
  /// [connected] swaps the icon + title between connected/disconnected
  /// variants; [subtitle] is the second line (e.g. device name or
  /// "Reconnecting…").
  Future<void> setStatus({required bool connected, String subtitle = ''}) =>
      _channel.invokeMethod('status', {
        'state': connected ? 'connected' : 'disconnected',
        'subtitle': subtitle,
      });
}
