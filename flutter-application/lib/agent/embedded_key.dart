/// Optional API key baked into the build.
///
/// **This is obfuscation, not protection.** Anything shipped inside an APK can
/// be recovered — the client holds both the code and the data. What the XOR
/// below actually buys is that the key is not a plaintext `sk-...` string in
/// the binary, so the automated scanners that trawl APKs (and any casual
/// `strings | grep`) come up empty. Someone who sits down with jadx or frida
/// will get it out in half an hour.
///
/// Therefore: build with a DEDICATED key that has a spend limit, never publish
/// an APK built this way, and treat the key as compromised the moment the APK
/// leaves your phone. The only real fix is to keep the key on a server and
/// have the app talk to that.
///
/// Supplied at build time by `scripts/build_app.sh`, which passes
/// `--dart-define=LLM_KEY_OBF=` followed by the masked, base64-encoded key.
/// With no define, this returns null and the app falls back to the key the
/// user types into settings (which lives in the system keystore).
library;

import 'dart:convert';

/// Base64 of the XOR-masked key. Empty in a plain `flutter build`.
const _obfuscated = String.fromEnvironment('LLM_KEY_OBF');

/// Mask phrase. Deliberately mundane — it must not read as "the key is over
/// here". Keep in sync with scripts/build_app.sh.
const _mask = 'aa-bridge/vesc-display/2026';

/// The build-time key, or null when none was baked in.
String? get embeddedApiKey {
  if (_obfuscated.isEmpty) return null;
  try {
    final data = base64Decode(_obfuscated);
    final maskBytes = utf8.encode(_mask);
    final out = List<int>.generate(
        data.length, (i) => data[i] ^ maskBytes[i % maskBytes.length]);
    final key = utf8.decode(out).trim();
    return key.isEmpty ? null : key;
  } catch (_) {
    // A malformed define must not take the app down — fall back to settings.
    return null;
  }
}
