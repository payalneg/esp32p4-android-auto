/// Failure type shared by the whole VESC/NUS stack (transport + protocol).
///
/// Lives in its own file so `nus_transport.dart` can throw it without importing
/// `vesc_link.dart` (which imports the transport). [key] is an i18n key from
/// lib/i18n/strings.dart; the UI localizes it via `t()`.
library;

class VescLispException implements Exception {
  final String key;
  const VescLispException(this.key);
  @override
  String toString() => 'VescLispException($key)';
}
