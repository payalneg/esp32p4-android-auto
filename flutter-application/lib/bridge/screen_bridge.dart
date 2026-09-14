/// Keeps the display on while the rider needs it.
///
/// FLAG_KEEP_SCREEN_ON on the activity window, toggled over a method channel —
/// fifteen lines of Kotlin against a plugin dependency. It only works on the
/// UI engine, which is where the map lives.
library;

import 'package:flutter/services.dart';

class ScreenBridge {
  static const _methods = MethodChannel('aabridge/screen.cmd');

  /// Returns false when there is no window to pin (background engine, tests).
  static Future<bool> keepOn(bool on) async {
    try {
      return await _methods.invokeMethod<bool>('keepOn', <String, Object?>{'on': on}) ??
          false;
    } on MissingPluginException {
      return false;
    } on PlatformException {
      return false;
    }
  }
}
