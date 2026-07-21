/// JSON-safe DTOs for LISP runtime stats, shared between the background BLE
/// isolate (which parses them off the wire) and the UI isolate (which shows
/// them). Crosses the flutter_foreground_task port as a plain Map.
library;

class LispBinding {
  final String name;
  final double value;
  const LispBinding(this.name, this.value);
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
