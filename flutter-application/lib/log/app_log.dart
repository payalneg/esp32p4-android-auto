/// A journal of what the app has been doing, kept in memory and shown in
/// Settings.
///
/// Two isolates write to it. The UI has one of these; the foreground service
/// — where all the BLE lives — has its own, and forwards every line it writes
/// over the same port the rest of the IPC uses, so what the screen shows is
/// both halves of the app in one list. Nothing is written to disk: this is for
/// looking at a problem while it is happening, not an audit trail.
library;

import 'dart:collection';

import 'package:flutter/foundation.dart';

enum LogLevel { debug, info, warn, error }

@immutable
class LogEntry {
  const LogEntry(this.at, this.level, this.tag, this.message);

  final DateTime at;
  final LogLevel level;

  /// Where it came from: 'ble', 'nav', 'map', 'ota'… Short, lower case.
  final String tag;
  final String message;

  /// The line as it goes to the clipboard: '22:41:07.913 W ble  message'.
  @override
  String toString() {
    final t = at.toIso8601String().substring(11, 23);
    const marks = <LogLevel, String>{
      LogLevel.debug: 'D',
      LogLevel.info: 'I',
      LogLevel.warn: 'W',
      LogLevel.error: 'E',
    };
    return '$t ${marks[level]} ${tag.padRight(5)} $message';
  }

  Map<String, Object?> toJson() => <String, Object?>{
        'ms': at.millisecondsSinceEpoch,
        'lv': level.index,
        'tag': tag,
        'msg': message,
      };

  static LogEntry fromJson(Map<Object?, Object?> m) => LogEntry(
        DateTime.fromMillisecondsSinceEpoch((m['ms'] as num?)?.toInt() ?? 0),
        LogLevel.values[(m['lv'] as num?)?.toInt() ?? 1],
        (m['tag'] as String?) ?? '?',
        (m['msg'] as String?) ?? '',
      );
}

/// The app's log. `AppLog.i('ble', 'connected')` from anywhere.
class AppLog extends ChangeNotifier {
  AppLog._();

  static final AppLog instance = AppLog._();

  /// Enough to cover a session's worth of interesting moments without the
  /// list itself becoming the memory problem. At ~120 bytes a line this is
  /// well under a megabyte.
  static const int capacity = 1000;

  final Queue<LogEntry> _entries = Queue<LogEntry>();

  /// Where lines go when this log is the background service's: the UI cannot
  /// see this isolate's memory, so they are shipped over the port instead.
  static void Function(LogEntry)? forwarder;

  List<LogEntry> get entries => List<LogEntry>.unmodifiable(_entries);
  int get length => _entries.length;

  static void d(String tag, String message) =>
      instance.add(LogLevel.debug, tag, message);
  static void i(String tag, String message) =>
      instance.add(LogLevel.info, tag, message);
  static void w(String tag, String message) =>
      instance.add(LogLevel.warn, tag, message);
  static void e(String tag, String message) =>
      instance.add(LogLevel.error, tag, message);

  void add(LogLevel level, String tag, String message) {
    final entry = LogEntry(DateTime.now(), level, tag, message);
    _append(entry);
    // Still useful in a debug run, and free in release.
    if (kDebugMode) debugPrint(entry.toString());
    forwarder?.call(entry);
  }

  /// A line that came from the other isolate, already stamped there.
  void adopt(LogEntry entry) => _append(entry);

  void _append(LogEntry entry) {
    _entries.addLast(entry);
    while (_entries.length > capacity) {
      _entries.removeFirst();
    }
    notifyListeners();
  }

  void clear() {
    _entries.clear();
    notifyListeners();
  }

  /// The whole journal as text, oldest first — what the copy button puts on
  /// the clipboard.
  String asText() => _entries.map((e) => e.toString()).join('\n');
}
