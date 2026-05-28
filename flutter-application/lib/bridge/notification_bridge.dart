import 'dart:async';

import 'package:flutter/services.dart';

class IncomingNotification {
  final int id;
  final String package;
  final String appName;
  final String title;
  final String text;
  final int postedAtMs;
  final bool removed;

  IncomingNotification({
    required this.id,
    required this.package,
    required this.appName,
    required this.title,
    required this.text,
    required this.postedAtMs,
    required this.removed,
  });

  factory IncomingNotification.fromMap(Map m) => IncomingNotification(
        id: (m['id'] as num).toInt(),
        package: m['package'] as String? ?? '',
        appName: m['appName'] as String? ?? '',
        title: m['title'] as String? ?? '',
        text: m['text'] as String? ?? '',
        postedAtMs: (m['postedAtMs'] as num?)?.toInt() ?? 0,
        removed: m['removed'] as bool? ?? false,
      );
}

class NotificationBridge {
  static const _events = EventChannel('aabridge/notifications');
  static const _methods = MethodChannel('aabridge/notifications.cmd');

  Stream<IncomingNotification> get events => _events
      .receiveBroadcastStream()
      .map((e) => IncomingNotification.fromMap(e as Map));

  Future<bool> hasPermission() async {
    final r = await _methods.invokeMethod<bool>('hasPermission');
    return r ?? false;
  }

  Future<void> openPermissionSettings() =>
      _methods.invokeMethod('openPermissionSettings');

  Future<Uint8List?> getAppIconPng(String package) async {
    final r =
        await _methods.invokeMethod<Uint8List>('getIconPng', {'package': package});
    return r;
  }

  Future<List<({String package, String label})>> listInstalledApps() async {
    final r = await _methods.invokeListMethod<Map>('listInstalledApps');
    if (r == null) return const [];
    return r
        .map((m) => (
              package: m['package'] as String,
              label: m['label'] as String,
            ))
        .toList();
  }
}
