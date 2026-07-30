/// Bounded buffer of asynchronous VESC output, living next to [VescLink] in
/// the background isolate.
///
/// The VESC pushes `(print ...)` output at whatever rate the running script
/// feels like — a print inside a 100 Hz loop is entirely possible — so this is
/// capped by BOTH line count and total bytes and evicts from the front.
/// Sequence numbers are monotonic and never reset, so a reader that missed a
/// window can tell how far behind it is ([dropped]) instead of silently
/// resyncing.
library;

import 'dart:async';
import 'dart:typed_data';

import '../lisp_models.dart';

class LispConsoleRing {
  LispConsoleRing({this.maxLines = 300, this.maxBytes = 32 * 1024});

  final int maxLines;
  final int maxBytes;

  final _lines = <LispConsoleLine>[];
  final _ctrl = StreamController<LispConsoleLine>.broadcast();
  final _clock = Stopwatch()..start();

  int _seq = 0;
  int _bytes = 0;
  int _dropped = 0;
  bool _everSawPrint = false;

  /// Next sequence number that will be handed out.
  int get seq => _seq;

  /// Lines evicted from the ring since it was created.
  int get dropped => _dropped;

  /// Whether a real print packet has ever arrived. False means the channel may
  /// not exist on this link at all (see the direct-adapter caveat in
  /// [VescLink]), which callers must not confuse with "the script is quiet".
  bool get everSawPrint => _everSawPrint;

  Stream<LispConsoleLine> get stream => _ctrl.stream;

  void add(String text, {String kind = 'print'}) {
    if (kind == 'print') _everSawPrint = true;
    final line = LispConsoleLine(
      seq: _seq++,
      tMs: _clock.elapsedMilliseconds,
      text: text,
      kind: kind,
    );
    _lines.add(line);
    _bytes += text.length;
    _evict();
    if (!_ctrl.isClosed) _ctrl.add(line);
  }

  /// Hex dump of a packet the link didn't recognise — only used while the
  /// debug toggle is on, to work out the framing of an unknown command id.
  void addRaw(int cmdId, Uint8List payload) {
    final n = payload.length < 32 ? payload.length : 32;
    final hex = [
      for (var i = 0; i < n; i++)
        payload[i].toRadixString(16).padLeft(2, '0'),
    ].join(' ');
    add('cmd $cmdId, ${payload.length} B: $hex'
        '${payload.length > n ? ' …' : ''}', kind: 'raw');
  }

  void _evict() {
    while (_lines.length > maxLines || (_bytes > maxBytes && _lines.length > 1)) {
      final gone = _lines.removeAt(0);
      _bytes -= gone.text.length;
      _dropped++;
    }
  }

  /// Lines with `seq >= sinceSeq`, oldest first, at most [maxLines] of them.
  LispConsoleChunk read({int sinceSeq = 0, int maxLines = 200}) {
    final out = <LispConsoleLine>[];
    for (final l in _lines) {
      if (l.seq < sinceSeq) continue;
      out.add(l);
      if (out.length >= maxLines) break;
    }
    return LispConsoleChunk(
      lines: out,
      nextSeq: out.isEmpty ? (sinceSeq > _seq ? _seq : sinceSeq) : out.last.seq + 1,
      dropped: _dropped,
      alive: _everSawPrint,
    );
  }

  /// Drop the buffered lines. [seq] keeps counting — a reader holding an old
  /// cursor must not silently re-read cleared content.
  void clear() {
    _lines.clear();
    _bytes = 0;
  }

  Future<void> dispose() async {
    await _ctrl.close();
  }
}
