/// Navigator frames to the head unit — the phone's half of main/ble_nav.c.
///
/// The phone renders the map (it owns the route, the graph and the tiles) and
/// sends the head unit a picture of what to show. Frames go out one at a time:
/// BEGIN, the JPEG bytes, END, then wait for the head unit's acknowledgement
/// before starting the next. That makes a slow decode throttle us instead of
/// piling stale pictures into the link, and it gives us a decode time to
/// report.
///
/// The head unit says whether its navigator screen is the one the rider is
/// looking at. While it is not, nothing is sent at all.
///
/// This file is plain Dart over two abstract characteristics so it can be
/// tested without a radio; [BleService] supplies the real ones.
library;

import 'dart:async';
import 'dart:typed_data';

/// Control opcodes we write (see main/ble_nav.h).
class NavOp {
  static const frameBegin = 0x01;
  static const frameEnd = 0x02;
  static const stop = 0x03;
  static const hello = 0x04;
  static const tileBegin = 0x05;
  static const tileEnd = 0x06;
  static const view = 0x07;
  static const routeBegin = 0x08;
  static const routeEnd = 0x09;
  static const guide = 0x0A;
}

/// Turn kinds, in the order the head unit expects (nav_turn_t in
/// main/nav_route.h). Mirrors ManeuverType.
const List<String> kNavTurnOrder = <String>[
  'straight',
  'slightLeft',
  'slightRight',
  'turnLeft',
  'turnRight',
  'sharpLeft',
  'sharpRight',
  'uturn',
  'arrive',
];

/// Wire formats a map tile may travel in. PNG is what the tile cache already
/// holds, so forwarding costs nothing and keeps the labels crisp; JPEG is
/// smaller but has to be transcoded on the phone and blurs coloured text.
const int kTileFormatPng = 0;
const int kTileFormatJpeg = 1;

/// Notification kinds the head unit sends back.
class NavStatus {
  static const state = 0x10;
  static const frameAck = 0x11;
  static const tileAck = 0x12;

  /// Nine bytes, not six: somewhere the rider picked on the head unit's own
  /// map. See NAV_ST_DEST in main/ble_nav.h.
  static const destination = 0x13;

  /// Ten bytes: a tile the head unit had to drop to make room. See
  /// NAV_ST_DROPPED in main/ble_nav.h.
  static const dropped = 0x14;

  /// The rider changed the zoom with the buttons on the panel; the level is
  /// in the second byte. See NAV_ST_ZOOM in main/ble_nav.h.
  static const zoom = 0x15;
}

/// FRAME_ACK results.
class NavAck {
  static const ok = 0;
  static const badParam = 1;
  static const truncated = 2;
  static const decodeFailed = 3;

  /// The previous frame is still on its way to the panel.
  static const busy = 4;

  /// The rider is looking at something else — stop sending until STATE says
  /// otherwise.
  static const hidden = 5;

  /// No answer came back in time. Not a wire value; the sender's verdict.
  static const timeout = 0xFF;
}

/// Largest DATA write older firmware accepts when it does not say.
const int kNavFallbackChunk = 244;

/// How long to wait for FRAME_ACK before giving up on a frame. Generous: the
/// head unit decodes and scales before it answers, and the link may be busy
/// with a notification burst.
const Duration kNavAckTimeout = Duration(seconds: 3);

/// What the head unit says about its own screen.
class NavDisplayState {
  const NavDisplayState({
    required this.navMode,
    required this.visible,
    required this.maxChunk,
  });

  /// The head unit's full-screen mode is the navigator.
  final bool navMode;

  /// ...and it is the screen actually being shown.
  final bool visible;

  /// Largest DATA write it accepts.
  final int maxChunk;

  static const unknown = NavDisplayState(
    navMode: false,
    visible: false,
    maxChunk: 0,
  );

  /// Parses a 6-byte STATE notification, or null if it is not one.
  static NavDisplayState? parse(List<int> raw) {
    if (raw.length < 6 || raw[0] != NavStatus.state) return null;
    return NavDisplayState(
      navMode: raw[1] != 0,
      visible: (raw[2] | (raw[3] << 8)) != 0,
      maxChunk: raw[4] | (raw[5] << 8),
    );
  }

  @override
  String toString() =>
      'NavDisplayState(nav: $navMode, visible: $visible, chunk: $maxChunk)';
}

/// The head unit's verdict on one frame.
class NavFrameResult {
  const NavFrameResult(this.ack, this.seq, this.decodeMs);

  final int ack;
  final int seq;
  final int decodeMs;

  bool get ok => ack == NavAck.ok;

  @override
  String toString() => 'NavFrameResult(ack: $ack, seq: $seq, ${decodeMs}ms)';
}

/// The two characteristics this needs, narrowed to what it uses. Keeps
/// flutter_blue_plus out of the protocol — and out of its tests.
abstract class NavChannel {
  /// Control writes, acknowledged.
  Future<void> writeCtrl(Uint8List value);

  /// Data writes. [withoutResponse] is a hint: the caller falls back to
  /// acknowledged writes when the platform reports congestion.
  Future<void> writeData(Uint8List value, {required bool withoutResponse});

  /// Notifications on the control characteristic.
  Stream<List<int>> get notifications;

  /// Negotiated ATT MTU, or null when it is not known yet.
  int? get mtu;
}

/// Sends frames, one at a time, and tracks what the head unit says.
class NavStream {
  NavStream(this._channel) {
    _sub = _channel.notifications.listen(_onNotify);
  }

  final NavChannel _channel;
  late final StreamSubscription<List<int>> _sub;

  final _stateCtrl = StreamController<NavDisplayState>.broadcast();
  final _ackCtrl = StreamController<NavFrameResult>.broadcast();
  final _destCtrl = StreamController<({double lat, double lon})>.broadcast();
  final _droppedCtrl = StreamController<({int z, int x, int y})>.broadcast();
  final _zoomCtrl = StreamController<int>.broadcast();

  NavDisplayState _state = NavDisplayState.unknown;
  int _seq = 0;
  bool _sending = false;

  /// What the head unit last told us about its screen.
  NavDisplayState get state => _state;
  Stream<NavDisplayState> get states => _stateCtrl.stream;

  /// Destinations the rider chose on the head unit itself.
  Stream<({double lat, double lon})> get destinations => _destCtrl.stream;

  /// Tiles the head unit dropped to make room. It keeps them in RAM, so a
  /// sender that never repeats itself has to hear about this.
  Stream<({int z, int x, int y})> get dropped => _droppedCtrl.stream;

  /// The zoom level the rider picked on the head unit's own map.
  Stream<int> get zooms => _zoomCtrl.stream;

  /// Whether a frame is on the wire right now.
  bool get busy => _sending;

  void _onNotify(List<int> raw) {
    if (raw.isEmpty) return;
    final st = NavDisplayState.parse(raw);
    if (st != null) {
      _state = st;
      if (!_stateCtrl.isClosed) _stateCtrl.add(st);
      return;
    }
    if (raw[0] == NavStatus.dropped && raw.length >= 10) {
      final bd = ByteData.sublistView(Uint8List.fromList(raw));
      if (!_droppedCtrl.isClosed) {
        _droppedCtrl.add((
          z: raw[1],
          x: bd.getUint32(2, Endian.little),
          y: bd.getUint32(6, Endian.little),
        ));
      }
      return;
    }
    if (raw[0] == NavStatus.zoom && raw.length >= 2) {
      if (!_zoomCtrl.isClosed) _zoomCtrl.add(raw[1]);
      return;
    }
    if (raw[0] == NavStatus.destination && raw.length >= 9) {
      final bd = ByteData.sublistView(Uint8List.fromList(raw));
      if (!_destCtrl.isClosed) {
        _destCtrl.add((
          lat: bd.getInt32(1, Endian.little) / 1e7,
          lon: bd.getInt32(5, Endian.little) / 1e7,
        ));
      }
      return;
    }
    if ((raw[0] == NavStatus.frameAck || raw[0] == NavStatus.tileAck) &&
        raw.length >= 6) {
      final r = NavFrameResult(
        raw[1],
        raw[2] | (raw[3] << 8),
        raw[4] | (raw[5] << 8),
      );
      if (!_ackCtrl.isClosed) _ackCtrl.add(r);
    }
  }

  /// Ask the head unit to describe its screen. The answer arrives on [states].
  Future<void> hello() =>
      _channel.writeCtrl(Uint8List.fromList(<int>[NavOp.hello]));

  /// Tell it we have stopped, so it can say so on screen instead of waiting.
  Future<void> stop() =>
      _channel.writeCtrl(Uint8List.fromList(<int>[NavOp.stop]));

  /// Send one frame and wait for the head unit's verdict.
  ///
  /// [jpeg] must be baseline 4:2:0 with both axes a multiple of 16 and the
  /// panel's proportions — the head unit rejects anything else, since it
  /// scales the picture to the whole screen.
  Future<NavFrameResult> sendFrame(
    int width,
    int height,
    Uint8List jpeg,
  ) async {
    if (_sending) {
      return const NavFrameResult(NavAck.busy, 0, 0);
    }
    _sending = true;
    final seq = _seq = (_seq + 1) & 0xFFFF;
    try {
      // Arm the answer before writing: on a fast link the acknowledgement can
      // land before an await further down would have started listening.
      final acked = _ackCtrl.stream
          .firstWhere((r) => r.seq == seq)
          .timeout(kNavAckTimeout);

      final begin = Uint8List(11);
      final bd = ByteData.sublistView(begin);
      begin[0] = NavOp.frameBegin;
      bd.setUint16(1, width, Endian.little);
      bd.setUint16(3, height, Endian.little);
      bd.setUint32(5, jpeg.length, Endian.little);
      bd.setUint16(9, seq, Endian.little);
      await _channel.writeCtrl(begin);

      final chunk = chunkSize;
      for (var off = 0; off < jpeg.length; off += chunk) {
        final end = (off + chunk < jpeg.length) ? off + chunk : jpeg.length;
        await _writeChunk(Uint8List.sublistView(jpeg, off, end));
      }

      final end = Uint8List(3);
      end[0] = NavOp.frameEnd;
      ByteData.sublistView(end).setUint16(1, seq, Endian.little);
      await _channel.writeCtrl(end);

      return await acked;
    } on TimeoutException {
      return NavFrameResult(NavAck.timeout, seq, 0);
    } finally {
      _sending = false;
    }
  }

  /// Send one map tile. The head unit keeps it, so a tile goes over the link
  /// once and is then drawn from its memory for the rest of the ride.
  Future<NavFrameResult> sendTile(
      int z, int x, int y, int format, Uint8List bytes) async {
    if (_sending) return const NavFrameResult(NavAck.busy, 0, 0);
    _sending = true;
    final seq = _seq = (_seq + 1) & 0xFFFF;
    try {
      final acked = _ackCtrl.stream
          .firstWhere((r) => r.seq == seq)
          .timeout(kNavAckTimeout);

      final begin = Uint8List(17);
      final bd = ByteData.sublistView(begin);
      begin[0] = NavOp.tileBegin;
      begin[1] = format;
      begin[2] = z;
      bd.setUint32(3, x, Endian.little);
      bd.setUint32(7, y, Endian.little);
      bd.setUint32(11, bytes.length, Endian.little);
      bd.setUint16(15, seq, Endian.little);
      await _channel.writeCtrl(begin);

      final chunk = chunkSize;
      for (var off = 0; off < bytes.length; off += chunk) {
        final end = (off + chunk < bytes.length) ? off + chunk : bytes.length;
        await _writeChunk(Uint8List.sublistView(bytes, off, end));
      }

      final end = Uint8List(3);
      end[0] = NavOp.tileEnd;
      ByteData.sublistView(end).setUint16(1, seq, Endian.little);
      await _channel.writeCtrl(end);

      return await acked;
    } on TimeoutException {
      return NavFrameResult(NavAck.timeout, seq, 0);
    } finally {
      _sending = false;
    }
  }

  /// Say where the rider is, and how fast. Fourteen bytes, unacknowledged, as
  /// often as the map should move — this is what replaces sending a picture.
  ///
  /// The speed is what lets the head unit carry the view forward between
  /// updates and redraw at its own rate, so the map glides rather than
  /// stepping twice a second.
  Future<void> sendView(double lat, double lon, int zoom, int headingDeg,
      {double speedMs = 0}) {
    final v = Uint8List(14);
    final bd = ByteData.sublistView(v);
    v[0] = NavOp.view;
    bd.setInt32(1, (lat * 1e7).round(), Endian.little);
    bd.setInt32(5, (lon * 1e7).round(), Endian.little);
    v[9] = zoom;
    bd.setUint16(10, headingDeg, Endian.little);
    final cms = (speedMs * 100).round().clamp(0, 0xFFFF);
    bd.setUint16(12, cms, Endian.little);
    return _channel.writeCtrl(v);
  }

  /// Send the route line. Points are whatever the phone routed, simplified
  /// for drawing before they get here; the head unit keeps them and draws the
  /// line over its own map, so this goes once per route rather than per frame.
  Future<NavFrameResult> sendRoute(List<({double lat, double lon})> pts) async {
    if (_sending) return const NavFrameResult(NavAck.busy, 0, 0);
    if (pts.length < 2) return const NavFrameResult(NavAck.badParam, 0, 0);
    _sending = true;
    final seq = _seq = (_seq + 1) & 0xFFFF;
    try {
      final acked = _ackCtrl.stream
          .firstWhere((r) => r.seq == seq)
          .timeout(kNavAckTimeout);

      final begin = Uint8List(5);
      begin[0] = NavOp.routeBegin;
      final bh = ByteData.sublistView(begin);
      bh.setUint16(1, pts.length, Endian.little);
      bh.setUint16(3, seq, Endian.little);
      await _channel.writeCtrl(begin);

      final body = Uint8List(pts.length * 8);
      final bd = ByteData.sublistView(body);
      for (var i = 0; i < pts.length; i++) {
        bd.setInt32(i * 8, (pts[i].lat * 1e7).round(), Endian.little);
        bd.setInt32(i * 8 + 4, (pts[i].lon * 1e7).round(), Endian.little);
      }
      final chunk = chunkSize;
      for (var off = 0; off < body.length; off += chunk) {
        final end = (off + chunk < body.length) ? off + chunk : body.length;
        await _writeChunk(Uint8List.sublistView(body, off, end));
      }

      final end = Uint8List(3);
      end[0] = NavOp.routeEnd;
      ByteData.sublistView(end).setUint16(1, seq, Endian.little);
      await _channel.writeCtrl(end);
      return await acked;
    } on TimeoutException {
      return NavFrameResult(NavAck.timeout, seq, 0);
    } finally {
      _sending = false;
    }
  }

  /// Where the next turn is and what is left of the ride. Eleven bytes,
  /// unacknowledged, sent whenever any of it changes.
  Future<void> sendGuide({
    required int turn,
    required int distM,
    required int remainingM,
    required int remainingS,
    required bool offRoute,
  }) {
    final g = Uint8List(11);
    final bd = ByteData.sublistView(g);
    g[0] = NavOp.guide;
    g[1] = turn;
    bd.setUint16(2, distM.clamp(0, 0xFFFF), Endian.little);
    bd.setUint32(4, remainingM.clamp(0, 0xFFFFFFFF), Endian.little);
    bd.setUint16(8, remainingS.clamp(0, 0xFFFF), Endian.little);
    g[10] = offRoute ? 1 : 0;
    return _channel.writeCtrl(g);
  }

  /// Largest DATA write to use: what the head unit accepts, capped by what the
  /// link can carry in one packet.
  int get chunkSize {
    final cap = _state.maxChunk > 0 ? _state.maxChunk : kNavFallbackChunk;
    final mtu = _channel.mtu ?? 247;
    final byMtu = mtu - 3;
    return byMtu.clamp(20, cap);
  }

  /// One data chunk, with the retry policy the firmware update learned.
  ///
  /// Android reports BUSY when the controller's write credits run out; they
  /// come back at the next connection event, so wait about one interval and
  /// try again without a response. Falling back to acknowledged writes too
  /// early costs a full round trip per chunk and roughly halves the frame rate.
  Future<void> _writeChunk(Uint8List chunk) async {
    const fastAttempts = 6;
    var attempt = 0;
    while (true) {
      try {
        await _channel.writeData(
          chunk,
          withoutResponse: attempt < fastAttempts,
        );
        return;
      } on Object catch (e) {
        final msg = e.toString().toLowerCase();
        final busy = msg.contains('busy') || msg.contains('201');
        if (!busy || attempt >= 12) rethrow;
        attempt++;
        await Future<void>.delayed(Duration(milliseconds: 10 * attempt));
      }
    }
  }

  Future<void> dispose() async {
    await _sub.cancel();
    await _stateCtrl.close();
    await _ackCtrl.close();
    await _destCtrl.close();
    await _droppedCtrl.close();
    await _zoomCtrl.close();
  }
}
