/// Spoken guidance, behind an interface so the rest of the navigator never
/// touches the TTS plugin and tests can hand in a recorder.
library;

import 'package:flutter_tts/flutter_tts.dart';

abstract class Voice {
  /// Speaks [text], cutting off whatever was still being said — during a
  /// ride the newest instruction is the only one that matters.
  Future<void> say(String text, {required String language});

  Future<void> stop();
}

class TtsVoice implements Voice {
  final FlutterTts _tts = FlutterTts();
  String? _language;

  @override
  Future<void> say(String text, {required String language}) async {
    try {
      if (_language != language) {
        await _tts.setLanguage(language);
        _language = language;
      }
      await _tts.speak(text);
    } on Object {
      // No engine, no voice data for the language, engine busy — none of it
      // is worth more than a missed phrase; the banner still shows the turn.
    }
  }

  @override
  Future<void> stop() async {
    try {
      await _tts.stop();
    } on Object {
      // as above
    }
  }
}

/// Says nothing. What the screen uses when voice guidance is switched off.
class SilentVoice implements Voice {
  const SilentVoice();

  @override
  Future<void> say(String text, {required String language}) async {}

  @override
  Future<void> stop() async {}
}
