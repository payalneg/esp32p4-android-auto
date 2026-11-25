/// Messages on the head unit's firmware-prompt characteristic (NotifBridge
/// ...000E).
///
/// The app knows which firmware image ships in the APK and reads the running
/// version over BLE; when the bundled one is newer it offers it. The head unit
/// shows an Update / Later / Skip dialog and answers with one byte.
///
/// Wire form is newline-separated UTF-8:
///   OFFER
///   CURRENT
///   AVAILABLE
///   TITLE
///   BODY
///   UPDATE-BUTTON
///   LATER-BUTTON
///   SKIP-BUTTON
///   UPDATING-LABEL
///   PROGRESS
///   PERCENT (0..100)
///   FAILED
///   TEXT
library;

import 'dart:convert';

/// What the rider tapped on the head unit.
enum FwPromptChoice {
  /// Run the update now.
  update,

  /// Not now — ask again next time the head unit connects.
  later,

  /// Never offer this version again (persisted by the phone).
  skip,
}

/// Decode the head unit's answer: one byte (0 later, 1 update, 2 skip), or a
/// text word from firmware that chose the friendlier form.
FwPromptChoice? parseFwPromptChoice(List<int> raw) {
  if (raw.isEmpty) return null;
  if (raw.length == 1 && raw[0] <= 2) {
    return switch (raw[0]) {
      1 => FwPromptChoice.update,
      2 => FwPromptChoice.skip,
      _ => FwPromptChoice.later,
    };
  }
  final s = utf8.decode(raw, allowMalformed: true).trim().toUpperCase();
  return switch (s) {
    'NOW' || 'UPDATE' || 'Y' => FwPromptChoice.update,
    'LATER' || 'N' => FwPromptChoice.later,
    'SKIP' || 'S' => FwPromptChoice.skip,
    _ => null,
  };
}

/// Everything the head unit needs to render its dialog. Labels are sent from
/// the phone so the prompt follows the app's language; the firmware has no
/// translation table of its own.
class FwPromptOffer {
  const FwPromptOffer({
    required this.current,
    required this.available,
    required this.title,
    required this.body,
    required this.now,
    required this.later,
    required this.skip,
    required this.updating,
  });

  final String current;
  final String available;
  final String title;
  final String body;
  final String now;
  final String later;
  final String skip;
  final String updating;

  String encode() => <String>[
        'OFFER',
        current,
        available,
        title,
        body,
        now,
        later,
        skip,
        updating,
      ].map(_clean).join('\n');

  static String progress(int percent) =>
      'PROGRESS\n${percent.clamp(0, 100)}';

  static String failed(String text) => 'FAILED\n${_clean(text)}';

  /// The wire format is line-based; a newline inside a label would be parsed
  /// as the next field.
  static String _clean(String s) => s.replaceAll(RegExp(r'[\r\n]+'), ' ').trim();
}
