/// The build-time key round-trip.
///
/// `embedded_key.dart` reads a compile-time define, so the deobfuscation here
/// is re-implemented against the same mask to prove the two halves agree —
/// and to pin the property that actually matters: the masked blob does not
/// contain the key in plaintext, which is the whole (modest) point.
library;

import 'dart:convert';

import 'package:aa_bridge/agent/embedded_key.dart';
import 'package:flutter_test/flutter_test.dart';

/// Same transform scripts/build_app.sh applies.
const _mask = 'aa-bridge/vesc-display/2026';

String obfuscate(String key) {
  final k = utf8.encode(key);
  final m = utf8.encode(_mask);
  return base64Encode(
      List<int>.generate(k.length, (i) => k[i] ^ m[i % m.length]));
}

String deobfuscate(String blob) {
  final data = base64Decode(blob);
  final m = utf8.encode(_mask);
  return utf8.decode(
      List<int>.generate(data.length, (i) => data[i] ^ m[i % m.length]));
}

void main() {
  test('mask round-trips a realistic key', () {
    const key = 'sk-or-v1-0123456789abcdef0123456789abcdef';
    expect(deobfuscate(obfuscate(key)), key);
  });

  test('the masked blob does not contain the key', () {
    const key = 'sk-or-v1-deadbeefdeadbeefdeadbeefdeadbeef';
    final blob = obfuscate(key);
    // What a scanner greps for must not survive the transform.
    expect(blob, isNot(contains('sk-or')));
    expect(blob, isNot(contains(key)));
    expect(utf8.decode(base64Decode(blob), allowMalformed: true),
        isNot(contains('sk-or')));
  });

  test('handles a key longer than the mask', () {
    final key = 'sk-or-v1-${'a' * 200}';
    expect(deobfuscate(obfuscate(key)), key);
  });

  test('with no define compiled in, there is no embedded key', () {
    // Plain `flutter test` passes no --dart-define, which is the same state a
    // keyless build ships in: the app must fall back to settings.
    expect(embeddedApiKey, isNull);
  });
}
