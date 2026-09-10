/// Places handed to the app by Android — see nav/nav_link.dart for what they
/// mean and MainActivity.kt for where they come from.
///
/// The payload is text: a `geo:` URI, a `google.navigation:` URI, or the
/// text of a share ("Otwiera Google Maps https://…"). Two paths, because the
/// app may or may not be running when it arrives: [initial] fetches the one
/// the activity was launched with, [incoming] streams the ones delivered to
/// a running activity.
library;

import 'dart:async';

import 'package:flutter/services.dart';

class NavIntentBridge {
  static const _methods = MethodChannel('aabridge/nav.intent');
  static final StreamController<String> _incoming =
      StreamController<String>.broadcast();
  static bool _bound = false;

  static Stream<String> get incoming {
    _bind();
    return _incoming.stream;
  }

  /// The payload the app was started with, once; null otherwise.
  static Future<String?> initial() async {
    _bind();
    try {
      return await _methods.invokeMethod<String>('takeInitial');
    } on MissingPluginException {
      return null; // tests, or a platform without the channel
    } on PlatformException {
      return null;
    }
  }

  static void _bind() {
    if (_bound) return;
    _bound = true;
    _methods.setMethodCallHandler((MethodCall call) async {
      if (call.method != 'open') return;
      final s = call.arguments as String?;
      if (s != null && s.trim().isNotEmpty) _incoming.add(s);
    });
  }
}
