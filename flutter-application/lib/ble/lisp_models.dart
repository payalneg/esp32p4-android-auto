/// JSON-safe DTOs for the LISP editor, shared between the background BLE
/// isolate (which talks to the hardware) and the UI isolate (which shows the
/// result). Everything here crosses the flutter_foreground_task port as a plain
/// Map.
library;

/// Which NUS link the VESC protocol layer is pointed at.
///
/// [headUnit] — the head unit's built-in NUS bridge (it relays to the VESC over
/// CAN). [direct] — a stand-alone VESC BLE adapter the app connects to itself,
/// as a second, independent GATT link (works with no head unit at all).
enum VescTargetKind { headUnit, direct }

/// Link state of the selected target.
enum VescLinkState { idle, connecting, connected, failed }

/// Snapshot of the current VESC target, pushed to the UI on every change.
class VescTargetInfo {
  final VescTargetKind kind;

  /// Set for [VescTargetKind.direct]: the adapter's BLE remote id / name.
  final String? remoteId;
  final String? name;
  final VescLinkState state;

  /// Negotiated ATT MTU of the active link (informational, for the UI).
  final int mtu;

  /// Whether the head unit is connected AND exposes the NUS bridge, i.e.
  /// whether [VescTargetKind.headUnit] is selectable right now.
  final bool headUnitAvailable;

  const VescTargetInfo({
    required this.kind,
    this.remoteId,
    this.name,
    required this.state,
    this.mtu = 0,
    this.headUnitAvailable = false,
  });

  bool get connected => state == VescLinkState.connected;

  Map<String, dynamic> toMap() => {
        'kind': kind.name,
        'remoteId': remoteId,
        'name': name,
        'state': state.name,
        'mtu': mtu,
        'headUnitAvailable': headUnitAvailable,
      };

  factory VescTargetInfo.fromMap(Map<String, dynamic> m) => VescTargetInfo(
        kind: VescTargetKind.values.firstWhere(
            (e) => e.name == m['kind'],
            orElse: () => VescTargetKind.headUnit),
        remoteId: m['remoteId'] as String?,
        name: m['name'] as String?,
        state: VescLinkState.values.firstWhere(
            (e) => e.name == m['state'],
            orElse: () => VescLinkState.idle),
        mtu: (m['mtu'] as num?)?.toInt() ?? 0,
        headUnitAvailable: m['headUnitAvailable'] as bool? ?? false,
      );
}

/// Measurements of one code read/upload, so the cost of a transfer can be
/// looked at on real hardware (logcat line + a note in the editor) instead of
/// guessed at — the head-unit path adds a CAN hop that a direct adapter
/// doesn't, and per-chunk round-trip latency dominates either way.
class VescXferStats {
  final int bytes;
  final int ms;
  final int chunks;

  /// Requests that timed out and were re-sent.
  final int retries;

  /// Replies that arrived for the wrong offset (stale/misaligned).
  final int resyncs;
  final int maxChunkMs;

  /// Which link carried it (`head-unit` / `direct:<name>`) and its ATT MTU.
  final String link;
  final int mtu;

  const VescXferStats({
    required this.bytes,
    required this.ms,
    required this.chunks,
    required this.retries,
    required this.resyncs,
    required this.maxChunkMs,
    required this.link,
    required this.mtu,
  });

  double get kbs => ms > 0 ? bytes / 1024 * 1000 / ms : 0;
  int get avgChunkMs => chunks > 0 ? (ms / chunks).round() : 0;

  Map<String, dynamic> toMap() => {
        'bytes': bytes,
        'ms': ms,
        'chunks': chunks,
        'retries': retries,
        'resyncs': resyncs,
        'maxChunkMs': maxChunkMs,
        'link': link,
        'mtu': mtu,
      };

  factory VescXferStats.fromMap(Map<String, dynamic> m) => VescXferStats(
        bytes: (m['bytes'] as num?)?.toInt() ?? 0,
        ms: (m['ms'] as num?)?.toInt() ?? 0,
        chunks: (m['chunks'] as num?)?.toInt() ?? 0,
        retries: (m['retries'] as num?)?.toInt() ?? 0,
        resyncs: (m['resyncs'] as num?)?.toInt() ?? 0,
        maxChunkMs: (m['maxChunkMs'] as num?)?.toInt() ?? 0,
        link: m['link'] as String? ?? '-',
        mtu: (m['mtu'] as num?)?.toInt() ?? 0,
      );
}

class LispBinding {
  final String name;
  final double value;
  const LispBinding(this.name, this.value);
}

/// One line of asynchronous output from the VESC.
///
/// [kind] is `print` for script output, `marker` for locally-generated notes
/// (e.g. "link rebound", so a reader can tell silence from a lost link), and
/// `raw` for the unrecognised-packet dump the debug toggle produces.
class LispConsoleLine {
  final int seq;

  /// Milliseconds since the ring was created — monotonic, comparable within a
  /// session, meaningless across sessions.
  final int tMs;
  final String text;
  final String kind;

  const LispConsoleLine({
    required this.seq,
    required this.tMs,
    required this.text,
    this.kind = 'print',
  });

  Map<String, dynamic> toMap() =>
      {'seq': seq, 'tMs': tMs, 'text': text, 'kind': kind};

  factory LispConsoleLine.fromMap(Map<String, dynamic> m) => LispConsoleLine(
        seq: (m['seq'] as num?)?.toInt() ?? 0,
        tMs: (m['tMs'] as num?)?.toInt() ?? 0,
        text: m['text'] as String? ?? '',
        kind: m['kind'] as String? ?? 'print',
      );
}

/// A window of console lines plus the state a reader needs to keep up:
/// [nextSeq] to resume from, [dropped] lines evicted before being read, and
/// [alive] — whether any print has EVER been seen on this link. `alive: false`
/// means the channel may not exist at all on this path, which is a different
/// thing from a script that simply hasn't printed yet.
class LispConsoleChunk {
  final List<LispConsoleLine> lines;
  final int nextSeq;
  final int dropped;
  final bool alive;

  const LispConsoleChunk({
    required this.lines,
    required this.nextSeq,
    required this.dropped,
    required this.alive,
  });

  Map<String, dynamic> toMap() => {
        'lines': [for (final l in lines) l.toMap()],
        'nextSeq': nextSeq,
        'dropped': dropped,
        'alive': alive,
      };

  factory LispConsoleChunk.fromMap(Map<String, dynamic> m) => LispConsoleChunk(
        lines: [
          for (final l in (m['lines'] as List?) ?? const [])
            LispConsoleLine.fromMap((l as Map).cast<String, dynamic>())
        ],
        nextSeq: (m['nextSeq'] as num?)?.toInt() ?? 0,
        dropped: (m['dropped'] as num?)?.toInt() ?? 0,
        alive: m['alive'] as bool? ?? false,
      );
}

/// Mirrors VESC Tool's LISP_STATS (COMM_LISP_GET_STATS reply). Percentages are
/// 0..100; [bindings] are the runtime global variables and their values.
class LispStats {
  final double cpu;
  final double heap;
  final double mem;
  final double stack;
  final String doneCtx;
  final List<LispBinding> bindings;

  const LispStats({
    required this.cpu,
    required this.heap,
    required this.mem,
    required this.stack,
    required this.doneCtx,
    required this.bindings,
  });

  Map<String, dynamic> toMap() => {
        'cpu': cpu,
        'heap': heap,
        'mem': mem,
        'stack': stack,
        'doneCtx': doneCtx,
        'bindings': [
          for (final b in bindings) {'name': b.name, 'value': b.value}
        ],
      };

  factory LispStats.fromMap(Map<String, dynamic> m) {
    final raw = (m['bindings'] as List?) ?? const [];
    return LispStats(
      cpu: (m['cpu'] as num?)?.toDouble() ?? 0,
      heap: (m['heap'] as num?)?.toDouble() ?? 0,
      mem: (m['mem'] as num?)?.toDouble() ?? 0,
      stack: (m['stack'] as num?)?.toDouble() ?? 0,
      doneCtx: m['doneCtx'] as String? ?? '',
      bindings: [
        for (final b in raw)
          LispBinding(
            (b as Map)['name'] as String? ?? '',
            (b['value'] as num?)?.toDouble() ?? 0,
          )
      ],
    );
  }
}
