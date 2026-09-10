/// The loop that keeps the head unit's navigator screen fed.
library;

import 'dart:async';
import 'dart:typed_data';

import 'package:aa_bridge/ble/nav_stream.dart';
import 'package:aa_bridge/nav/frame_codec.dart';
import 'package:aa_bridge/nav/frame_streamer.dart';
import 'package:flutter_test/flutter_test.dart';

class _FakeSink implements FrameSink {
  _FakeSink();

  @override
  bool available = true;

  NavDisplayState _state =
      const NavDisplayState(navMode: true, visible: true, maxChunk: 509);
  final _states = StreamController<NavDisplayState>.broadcast();

  final sent = <int>[];
  int hellos = 0;
  int stops = 0;
  int ack = NavAck.ok;

  @override
  NavDisplayState get displayState => _state;

  @override
  Stream<NavDisplayState> get displayStates => _states.stream;

  void setVisible(bool v) {
    _state = NavDisplayState(navMode: v, visible: v, maxChunk: 509);
    _states.add(_state);
  }

  @override
  Future<NavFrameResult> send(int w, int h, Uint8List jpeg) async {
    sent.add(jpeg.length);
    return NavFrameResult(ack, sent.length, 11);
  }

  @override
  void hello() => hellos++;

  @override
  void stop() => stops++;

  Future<void> close() => _states.close();
}

RawFrame _frame(int seed) {
  final rgba = Uint8List(16 * 16 * 4);
  for (var i = 0; i < rgba.length; i++) {
    rgba[i] = (i + seed) & 0xFF;
  }
  return RawFrame(16, 16, rgba);
}

void main() {
  late _FakeSink sink;
  var captureSeed = 1;
  RawFrame? nextFrame;

  NavFrameStreamer build() => NavFrameStreamer(
        capture: () async => nextFrame,
        sink: sink,
        // Encoding is the codec's business; here it only has to produce bytes.
        encode: (f) async => Uint8List.fromList(List<int>.filled(1234, 7)),
        sleep: (_) async {},
      );

  setUp(() {
    sink = _FakeSink();
    captureSeed = 1;
    nextFrame = _frame(captureSeed);
  });

  tearDown(() => sink.close());

  test('nothing is sent while the head unit is absent', () async {
    sink.available = false;
    final s = build();
    expect(await s.tick(), NavTick.unavailable);
    expect(sink.sent, isEmpty);
  });

  test('nothing is sent while the rider looks at another screen', () async {
    sink.setVisible(false);
    final s = build();
    expect(await s.tick(), NavTick.hidden);
    expect(sink.sent, isEmpty);
    // ...and we re-ask, in case we missed the screen coming back.
    expect(sink.hellos, 1);
  });

  test('a frame goes out once the navigator screen is up', () async {
    final s = build();
    expect(await s.tick(), NavTick.sent);
    expect(sink.sent, <int>[1234]);
    expect(s.status.value.framesSent, 1);
    expect(s.status.value.lastBytes, 1234);
    expect(s.status.value.streaming, isTrue);
  });

  test('an unchanged picture is not sent again', () async {
    final s = build();
    expect(await s.tick(), NavTick.sent);
    expect(await s.tick(), NavTick.unchanged);
    expect(await s.tick(), NavTick.unchanged);
    expect(sink.sent.length, 1);
  });

  test('a changed picture is sent', () async {
    final s = build();
    await s.tick();
    nextFrame = _frame(++captureSeed);
    expect(await s.tick(), NavTick.sent);
    expect(sink.sent.length, 2);
  });

  test('a refused frame is retried rather than skipped as unchanged', () async {
    final s = build();
    sink.ack = NavAck.busy;
    expect(await s.tick(), NavTick.failed);
    expect(s.status.value.lastAck, NavAck.busy);
    // Same picture, but it never arrived — it must go out again.
    sink.ack = NavAck.ok;
    expect(await s.tick(), NavTick.sent);
    expect(sink.sent.length, 2);
  });

  test('nothing to capture is not an error', () async {
    nextFrame = null;
    final s = build();
    expect(await s.tick(), NavTick.noFrame);
    expect(sink.sent, isEmpty);
  });

  test('the screen going away forces a fresh frame when it returns', () async {
    final s = build();
    await s.tick();
    expect(sink.sent.length, 1);

    sink.setVisible(false);
    await Future<void>.delayed(Duration.zero);
    sink.setVisible(true);
    await Future<void>.delayed(Duration.zero);

    // Same pixels as before, but the head unit stopped showing them.
    expect(await s.tick(), NavTick.sent);
    expect(sink.sent.length, 2);
  });

  test('starting greets the head unit, stopping says goodbye', () async {
    final s = build();
    await s.tick();
    s.start();
    expect(sink.hellos, greaterThanOrEqualTo(1));
    await s.stop();
    expect(sink.stops, 1);
    expect(s.running, isFalse);
    expect(s.status.value.framesSent, 0);
  });
}
