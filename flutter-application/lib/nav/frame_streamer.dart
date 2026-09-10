/// Keeps the head unit's navigator screen fed with pictures.
///
/// One frame at a time, at most one a second, and only while the head unit
/// says its navigator screen is the one being looked at. A picture identical
/// to the last one is not sent at all — a parked bike costs no air time.
///
/// The loop is a sequence of [tick]s so it can be tested without waiting on
/// real clocks: capture, hash, encode off the UI thread, send, wait out the
/// rest of the frame period.
library;

import 'dart:async';

import 'package:flutter/foundation.dart';

import '../ble/nav_stream.dart';
import 'frame_codec.dart';

/// One frame a second. The link carries tens of kilobytes a second at best,
/// and a 400x240 frame is 15-25 KB of it — asking for more would just queue.
const Duration kNavFramePeriod = Duration(seconds: 1);

/// How long to wait before looking again when there is nothing to do (no head
/// unit, or the rider is looking at something else).
const Duration kNavIdlePoll = Duration(seconds: 1);

/// After a failed send, back off before trying again.
const Duration kNavErrorBackoff = Duration(seconds: 2);

/// What the last [NavFrameStreamer.tick] did.
enum NavTick {
  /// No head unit, or its firmware has no navigator screen.
  unavailable,

  /// The head unit is there but showing something else.
  hidden,

  /// Nothing to capture — the view is not on screen yet.
  noFrame,

  /// The picture had not changed, so nothing went out.
  unchanged,

  /// A frame reached the head unit.
  sent,

  /// A frame was built but the head unit did not take it.
  failed,
}

/// What to show the rider about the stream.
@immutable
class NavStreamStatus {
  const NavStreamStatus({
    this.tick = NavTick.unavailable,
    this.framesSent = 0,
    this.lastBytes = 0,
    this.lastRoundTripMs = 0,
    this.lastAck = NavAck.ok,
  });

  final NavTick tick;
  final int framesSent;
  final int lastBytes;
  final int lastRoundTripMs;
  final int lastAck;

  bool get streaming => tick == NavTick.sent || tick == NavTick.unchanged;

  @override
  bool operator ==(Object other) =>
      other is NavStreamStatus &&
      other.tick == tick &&
      other.framesSent == framesSent &&
      other.lastBytes == lastBytes &&
      other.lastRoundTripMs == lastRoundTripMs &&
      other.lastAck == lastAck;

  @override
  int get hashCode =>
      Object.hash(tick, framesSent, lastBytes, lastRoundTripMs, lastAck);
}

/// Where frames come from: the rendered head-unit view, as raw pixels.
typedef FrameCapture = Future<RawFrame?> Function();

/// Where frames go. An interface rather than [BleProxy] directly, so the loop
/// can be driven by a fake in tests.
abstract class FrameSink {
  /// A head unit is connected and its firmware has the navigator screen.
  bool get available;

  /// What it last said about that screen.
  NavDisplayState get displayState;

  /// Changes to it.
  Stream<NavDisplayState> get displayStates;

  Future<NavFrameResult> send(int width, int height, Uint8List jpeg);

  /// Ask it to describe its screen.
  void hello();

  /// Tell it we stopped rendering.
  void stop();
}

class NavFrameStreamer {
  NavFrameStreamer({
    required FrameCapture capture,
    required FrameSink sink,
    Future<Uint8List> Function(RawFrame)? encode,
    Future<void> Function(Duration)? sleep,
  })  : _capture = capture,
        _sink = sink,
        _encode = encode ?? _encodeInIsolate,
        _sleep = sleep ?? _realSleep {
    // Watch the head unit's screen from the moment we exist, not from the
    // moment we start: a screen that went away and came back must get a fresh
    // picture, not be skipped as "unchanged".
    _stateSub = _sink.displayStates.listen((st) {
      if (!st.visible) _lastHash = 0;
    });
  }

  final FrameCapture _capture;
  final FrameSink _sink;
  final Future<Uint8List> Function(RawFrame) _encode;
  final Future<void> Function(Duration) _sleep;

  final status = ValueNotifier<NavStreamStatus>(const NavStreamStatus());

  bool _running = false;
  int _lastHash = 0;
  int _framesSent = 0;
  late final StreamSubscription<NavDisplayState>? _stateSub;

  static Future<Uint8List> _encodeInIsolate(RawFrame f) =>
      compute(encodeNavFrame, f);

  static Future<void> _realSleep(Duration d) => Future<void>.delayed(d);

  bool get running => _running;

  /// Begin streaming. Returns immediately; the loop runs until [stop].
  void start() {
    if (_running) return;
    _running = true;
    _sink.hello();
    unawaited(_loop());
  }

  Future<void> stop() async {
    if (!_running) return;
    _running = false;
    _lastHash = 0;
    _sink.stop();
    _set(const NavStreamStatus());
  }

  Future<void> dispose() async {
    await stop();
    await _stateSub?.cancel();
    _stateSub = null;
    status.dispose();
  }

  Future<void> _loop() async {
    while (_running) {
      final started = DateTime.now();
      final outcome = await tick();
      if (!_running) return;
      Duration wait;
      switch (outcome) {
        case NavTick.unavailable:
        case NavTick.hidden:
        case NavTick.noFrame:
          wait = kNavIdlePoll;
        case NavTick.failed:
          wait = kNavErrorBackoff;
        case NavTick.sent:
        case NavTick.unchanged:
          final spent = DateTime.now().difference(started);
          wait = kNavFramePeriod - spent;
      }
      if (wait > Duration.zero) await _sleep(wait);
    }
  }

  /// One pass of the loop. Public for tests, which drive it directly rather
  /// than waiting on wall-clock time.
  @visibleForTesting
  Future<NavTick> tick() async {
    if (!_sink.available) return _record(NavTick.unavailable);
    if (!_sink.displayState.visible) {
      // The head unit may have changed screens without us hearing; a HELLO
      // costs one packet and resyncs a stale view.
      _sink.hello();
      return _record(NavTick.hidden);
    }

    final frame = await _capture();
    if (frame == null || frame.rgba.isEmpty) return _record(NavTick.noFrame);

    final hash = frameHash(frame.rgba);
    if (hash == _lastHash) return _record(NavTick.unchanged);

    final jpeg = await _encode(frame);
    // No bail-out on a stop that landed mid-encode: one extra picture is
    // harmless, and the loop checks after every tick anyway.
    final result = await _sink.send(frame.width, frame.height, jpeg);
    if (result.ok) {
      _lastHash = hash;
      _framesSent++;
      return _record(NavTick.sent, bytes: jpeg.length, ack: result);
    }
    // Anything else means this picture never made it: forget the hash so the
    // same view is retried rather than skipped as "unchanged".
    _lastHash = 0;
    return _record(NavTick.failed, bytes: jpeg.length, ack: result);
  }

  NavTick _record(NavTick tick, {int bytes = 0, NavFrameResult? ack}) {
    _set(NavStreamStatus(
      tick: tick,
      framesSent: _framesSent,
      lastBytes: bytes != 0 ? bytes : status.value.lastBytes,
      lastRoundTripMs: ack?.decodeMs ?? status.value.lastRoundTripMs,
      lastAck: ack?.ack ?? status.value.lastAck,
    ));
    return tick;
  }

  void _set(NavStreamStatus s) {
    if (status.value != s) status.value = s;
  }
}
