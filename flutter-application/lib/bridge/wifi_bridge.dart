import 'package:flutter/services.dart';

/// Drives the native Android in-app WiFi join used to reach the head unit's
/// SoftAP for firmware OTA. No-op / throws on iOS (not wired up there).
///
/// [join] binds the whole app process to the SoftAP so plain HTTP requests
/// route over it; [unbind] restores the phone's default network. Always pair
/// a successful join with an unbind in a finally block.
class WifiBridge {
  static const _channel = MethodChannel('aabridge/wifi');

  /// Join + bind to [ssid]/[password]. Throws [PlatformException] (code
  /// "join_failed") if the user declines or the AP isn't reachable in time.
  Future<void> join(
    String ssid,
    String password, {
    Duration timeout = const Duration(seconds: 30),
  }) async {
    await _channel.invokeMethod('join', {
      'ssid': ssid,
      'password': password,
      'timeoutMs': timeout.inMilliseconds,
    });
  }

  Future<void> unbind() => _channel.invokeMethod('unbind');

  /// Visible WiFi SSIDs from the latest scan (de-duplicated, sorted). Empty
  /// on iOS or when location is off / permission denied.
  Future<List<String>> scan() async {
    final r = await _channel.invokeMethod<List<dynamic>>('scan');
    return r?.map((e) => e.toString()).toList() ?? const [];
  }

  /// SSID the phone is currently connected to, or null if unknown.
  Future<String?> currentSsid() => _channel.invokeMethod<String>('currentSsid');
}
