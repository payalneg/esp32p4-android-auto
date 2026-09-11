/// The wire side of the navigator picture: framing, chunking and what the
/// head unit says back. Mirrors firmware main/ble_nav.c.
library;

import 'dart:async';
import 'dart:typed_data';

import 'package:aa_bridge/ble/nav_stream.dart';
import 'package:flutter_test/flutter_test.dart';

/// A stand-in head unit: records what was written and answers on demand.
class _FakeChannel implements NavChannel {
  _FakeChannel();

  final ctrl = <Uint8List>[];
  final data = <Uint8List>[];
  final withoutResponse = <bool>[];
  final _notify = StreamController<List<int>>.broadcast();

  /// Answers every FRAME_END with this result, unless null.
  int? autoAck = NavAck.ok;
  int autoAckMs = 12;

  /// Throws this many times from writeData before giving in, to exercise the
  /// congestion retry.
  int busyWrites = 0;

  @override
  int? mtu = 512;

  @override
  Stream<List<int>> get notifications => _notify.stream;

  @override
  Future<void> writeCtrl(Uint8List value) async {
    ctrl.add(Uint8List.fromList(value));
    if (value.isNotEmpty &&
        (value[0] == NavOp.frameEnd || value[0] == NavOp.tileEnd) &&
        autoAck != null) {
      final seq = value[1] | (value[2] << 8);
      ack(autoAck!, seq, autoAckMs,
          status: value[0] == NavOp.tileEnd
              ? NavStatus.tileAck
              : NavStatus.frameAck);
    }
  }

  @override
  Future<void> writeData(Uint8List value,
      {required bool withoutResponse}) async {
    if (busyWrites > 0) {
      busyWrites--;
      throw StateError('write failed: BUSY (201)');
    }
    data.add(Uint8List.fromList(value));
    this.withoutResponse.add(withoutResponse);
  }

  void ack(int result, int seq, int ms, {int status = NavStatus.frameAck}) =>
      _notify.add(<int>[
        status,
        result,
        seq & 0xFF,
        seq >> 8,
        ms & 0xFF,
        ms >> 8,
      ]);

  /// Somewhere the rider tapped on the head unit's own map.
  void destination(double lat, double lon) {
    final b = Uint8List(9);
    final bd = ByteData.sublistView(b);
    b[0] = NavStatus.destination;
    bd.setInt32(1, (lat * 1e7).round(), Endian.little);
    bd.setInt32(5, (lon * 1e7).round(), Endian.little);
    _notify.add(b);
  }

  void state({required bool nav, required bool visible, int maxChunk = 509}) =>
      _notify.add(<int>[
        NavStatus.state,
        nav ? 1 : 0,
        visible ? 1 : 0,
        0,
        maxChunk & 0xFF,
        maxChunk >> 8,
      ]);

  Future<void> close() => _notify.close();
}

Uint8List _jpeg(int n) =>
    Uint8List.fromList(List<int>.generate(n, (i) => i & 0xFF));

void main() {
  late _FakeChannel ch;
  late NavStream nav;

  setUp(() {
    ch = _FakeChannel()..mtu = 512;
    nav = NavStream(ch);
  });

  tearDown(() async {
    await nav.dispose();
    await ch.close();
  });

  test('a frame is BEGIN, the bytes, then END', () async {
    ch.state(nav: true, visible: true);
    await Future<void>.delayed(Duration.zero);

    final r = await nav.sendFrame(400, 240, _jpeg(1200));

    expect(r.ok, isTrue);
    expect(r.decodeMs, 12);
    expect(ch.ctrl.length, 2);

    final begin = ByteData.sublistView(ch.ctrl.first);
    expect(ch.ctrl.first[0], NavOp.frameBegin);
    expect(ch.ctrl.first.length, 11);
    expect(begin.getUint16(1, Endian.little), 400);
    expect(begin.getUint16(3, Endian.little), 240);
    expect(begin.getUint32(5, Endian.little), 1200);
    final seq = begin.getUint16(9, Endian.little);

    expect(ch.ctrl.last[0], NavOp.frameEnd);
    expect(ch.ctrl.last.length, 3);
    expect(ByteData.sublistView(ch.ctrl.last).getUint16(1, Endian.little), seq);
    expect(r.seq, seq);

    final sent = ch.data.expand((c) => c).toList();
    expect(sent.length, 1200);
    expect(sent, _jpeg(1200));
  });

  test('chunks are capped by the head unit and by the MTU', () async {
    ch.state(nav: true, visible: true, maxChunk: 509);
    await Future<void>.delayed(Duration.zero);
    expect(nav.chunkSize, 509); // mtu 512 - 3

    await nav.sendFrame(400, 240, _jpeg(1100));
    expect(ch.data.map((c) => c.length).toList(), <int>[509, 509, 82]);

    ch.data.clear();
    ch.mtu = 247;
    expect(nav.chunkSize, 244);
    await nav.sendFrame(400, 240, _jpeg(500));
    expect(ch.data.map((c) => c.length).toList(), <int>[244, 244, 12]);
  });

  test('a head unit that does not declare a chunk size gets 244', () async {
    // No STATE seen yet — the pre-navigator firmware case.
    expect(nav.state.maxChunk, 0);
    expect(nav.chunkSize, kNavFallbackChunk);
  });

  test('STATE notifications are parsed and published', () async {
    final seen = <NavDisplayState>[];
    nav.states.listen(seen.add);

    ch.state(nav: true, visible: true, maxChunk: 509);
    ch.state(nav: false, visible: false, maxChunk: 509);
    await Future<void>.delayed(Duration.zero);

    expect(seen.length, 2);
    expect(seen.first.navMode, isTrue);
    expect(seen.first.visible, isTrue);
    expect(seen.first.maxChunk, 509);
    expect(seen.last.navMode, isFalse);
    expect(nav.state.visible, isFalse);
  });

  test('a rejected frame comes back with the head unit reason', () async {
    ch.autoAck = NavAck.hidden;
    final r = await nav.sendFrame(400, 240, _jpeg(100));
    expect(r.ok, isFalse);
    expect(r.ack, NavAck.hidden);
  });

  test('no answer times out instead of hanging', () async {
    ch.autoAck = null; // the head unit says nothing
    final r = await nav.sendFrame(400, 240, _jpeg(100));
    expect(r.ack, NavAck.timeout);
  }, timeout: const Timeout(Duration(seconds: 10)));

  test('a congested write is retried without falling back at once', () async {
    ch.busyWrites = 3;
    final r = await nav.sendFrame(400, 240, _jpeg(300));
    expect(r.ok, isTrue);
    // The first attempts stay unacknowledged — the fallback is for a run of
    // misses, not for the first hiccup.
    expect(ch.withoutResponse, everyElement(isTrue));
  });

  test('a second frame is refused while one is in flight', () async {
    ch.autoAck = null;
    final first = nav.sendFrame(400, 240, _jpeg(100));
    await Future<void>.delayed(Duration.zero);
    final second = await nav.sendFrame(400, 240, _jpeg(100));
    expect(second.ack, NavAck.busy);
    ch.ack(NavAck.ok, 1, 5);
    expect((await first).ok, isTrue);
  });

  test('a tile is BEGIN with its coordinates, the bytes, then END', () async {
    ch.state(nav: true, visible: true);
    await Future<void>.delayed(Duration.zero);

    final r = await nav.sendTile(17, 36409, 22228, kTileFormatPng, _jpeg(700));

    expect(r.ok, isTrue);
    expect(ch.ctrl.length, 2);
    final begin = ch.ctrl.first;
    final bd = ByteData.sublistView(begin);
    expect(begin[0], NavOp.tileBegin);
    expect(begin.length, 17);
    expect(begin[1], kTileFormatPng);
    expect(begin[2], 17);
    expect(bd.getUint32(3, Endian.little), 36409);
    expect(bd.getUint32(7, Endian.little), 22228);
    expect(bd.getUint32(11, Endian.little), 700);
    expect(ch.ctrl.last[0], NavOp.tileEnd);
    expect(ch.data.expand((c) => c).length, 700);
  });

  test('the view is a fourteen-byte control that needs no answer', () async {
    await nav.sendView(50.0619, 19.9368, 17, 271, speedMs: 5.5);
    expect(ch.ctrl.length, 1);
    final v = ch.ctrl.first;
    final bd = ByteData.sublistView(v);
    expect(v[0], NavOp.view);
    expect(v.length, 14);
    expect(bd.getInt32(1, Endian.little), 500619000);
    expect(bd.getInt32(5, Endian.little), 199368000);
    expect(v[9], 17);
    expect(bd.getUint16(10, Endian.little), 271);
    // Speed in centimetres a second, so the head unit can carry the view
    // forward between updates instead of stepping.
    expect(bd.getUint16(12, Endian.little), 550);
    // Nothing was waited on: the next position is a moment away anyway.
    expect(ch.data, isEmpty);
  });

  test('a destination picked on the head unit comes back', () async {
    final seen = <({double lat, double lon})>[];
    nav.destinations.listen(seen.add);

    ch.destination(50.0619, 19.9368);
    await Future<void>.delayed(Duration.zero);

    expect(seen.length, 1);
    expect(seen.first.lat, closeTo(50.0619, 1e-6));
    expect(seen.first.lon, closeTo(19.9368, 1e-6));
  });

  test('a nine-byte destination is not mistaken for an acknowledgement',
      () async {
    // Both ride the same characteristic and are told apart by the status
    // byte; a destination must not resolve a frame that is in flight.
    ch.autoAck = null;
    final pending = nav.sendFrame(400, 240, _jpeg(100));
    await Future<void>.delayed(Duration.zero);
    ch.destination(50.0, 20.0);
    await Future<void>.delayed(Duration.zero);
    ch.ack(NavAck.ok, 1, 4);
    expect((await pending).ok, isTrue);
  });

  test('stop and hello are single-byte controls', () async {
    await nav.hello();
    await nav.stop();
    expect(ch.ctrl.map((c) => c.first).toList(),
        <int>[NavOp.hello, NavOp.stop]);
  });

  test('an empty route clears the line, and carries no points', () async {
    // The head unit has no other way of learning that a ride ended; without
    // this it kept drawing the last route for ever.
    final f = nav.sendRoute(const <({double lat, double lon})>[]);
    await Future<void>.delayed(Duration.zero);

    expect(ch.ctrl.length, 1, reason: 'BEGIN only — no body, no END');
    expect(ch.ctrl.single[0], NavOp.routeBegin);
    expect(ch.ctrl.single[1] | (ch.ctrl.single[2] << 8), 0);
    expect(ch.data, isEmpty);

    final seq = ch.ctrl.single[3] | (ch.ctrl.single[4] << 8);
    ch.ack(NavAck.ok, seq, 0);
    expect((await f).ok, isTrue);
  });
}
