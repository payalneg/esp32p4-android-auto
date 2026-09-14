/// The journal is read at the side of a road, so what matters is that it keeps
/// the newest lines, survives a flood, and hands over text that can be pasted
/// into a message.
library;

import 'package:aa_bridge/log/app_log.dart';
import 'package:flutter_test/flutter_test.dart';

void main() {
  setUp(AppLog.instance.clear);
  tearDown(() => AppLog.forwarder = null);

  test('keeps the last lines and drops the oldest', () {
    for (var i = 0; i < AppLog.capacity + 50; i++) {
      AppLog.i('t', 'line $i');
    }
    final entries = AppLog.instance.entries;
    expect(entries.length, AppLog.capacity);
    expect(entries.first.message, 'line 50');
    expect(entries.last.message, 'line ${AppLog.capacity + 49}');
  });

  test('a line survives the trip between isolates', () {
    final wire = <Map<String, Object?>>[];
    AppLog.forwarder = (e) => wire.add(e.toJson());
    AppLog.w('ble', 'link lost');

    AppLog.instance.clear();
    AppLog.instance.adopt(LogEntry.fromJson(wire.single));

    final got = AppLog.instance.entries.single;
    expect(got.tag, 'ble');
    expect(got.message, 'link lost');
    expect(got.level, LogLevel.warn);
  });

  test('text for the clipboard is oldest first, one line each', () {
    AppLog.i('a', 'first');
    AppLog.e('b', 'second');
    final lines = AppLog.instance.asText().split('\n');
    expect(lines.length, 2);
    expect(lines.first, contains('first'));
    expect(lines.last, contains('second'));
    expect(lines.last, contains('E'));
  });

  test('listeners are told, so an open screen repaints', () {
    var notifications = 0;
    void bump() => notifications++;
    AppLog.instance.addListener(bump);
    AppLog.i('t', 'one');
    AppLog.instance.clear();
    AppLog.instance.removeListener(bump);
    expect(notifications, 2);
  });
}
