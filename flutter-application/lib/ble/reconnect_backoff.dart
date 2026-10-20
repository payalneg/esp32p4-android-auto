/// Manual recovery is a fallback for a stalled OS autoConnect request.
/// Leave that request alive while waiting; do not repeatedly restart scanning
/// at a fixed, short interval for a head unit that may be away for days.
class ReconnectBackoff {
  int _failures = 0;

  Duration get delay => Duration(
    seconds: switch (_failures) {
      0 => 40,
      1 => 80,
      2 => 160,
      _ => 300,
    },
  );

  void didRetry() {
    if (_failures < 3) _failures++;
  }

  void reset() => _failures = 0;
}
