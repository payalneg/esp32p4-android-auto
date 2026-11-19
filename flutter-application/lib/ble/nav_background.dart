/// The navigator feed that lives in the foreground-service isolate.
///
/// The UI isolate is the half of the app that sleeps: Android suspends it the
/// moment the screen goes dark, and with it the [HeadUnitFeed] the navigator
/// screen used to own. The service isolate is the half that stays awake — it
/// already pumps the clock, media and notifications — so the job of keeping the
/// head unit's map alive belongs here.
///
/// Ownership is handed over explicitly. While the rider is looking at the app
/// the UI feed runs and this one stands down (`setUiActive(true)`); when the app
/// goes away the UI sends a handoff (route / guide / zoom / look) and
/// `setUiActive(false)`, and this feed takes over with its own GPS so the head
/// unit keeps moving even with the phone face-down in a pocket.
library;

import 'dart:async';
import 'dart:io';
import 'dart:typed_data';

import 'package:package_info_plus/package_info_plus.dart';
import 'package:path_provider/path_provider.dart';

import '../log/app_log.dart';
import '../nav/geo.dart';
import '../nav/head_unit_feed.dart';
import '../nav/krakow_tiles.dart';
import '../nav/location_service.dart';
import '../nav/tile_cache.dart';
import 'nav_stream.dart';

/// A [HeadUnitLink] straight over the service's [NavStream] — no IPC hop, the
/// radio is in this isolate.
class _DirectLink implements HeadUnitLink {
  _DirectLink(this._nav);
  final NavStream _nav;

  @override
  bool get available => true;

  @override
  NavDisplayState get displayState => _nav.state;

  @override
  Stream<NavDisplayState> get displayStates => _nav.states;

  @override
  bool get localVector => _nav.state.localVector;

  @override
  Stream<({int z, int x, int y})> get dropped => _nav.dropped;

  @override
  Stream<void> get emptied => _nav.emptied;

  @override
  Future<NavFrameResult> sendTile(
          int z, int x, int y, int format, Uint8List bytes) =>
      _nav.sendTile(z, x, y, format, bytes);

  @override
  Future<void> sendView(double lat, double lon, int zoom, int headingDeg,
          {double speedMs = 0}) =>
      _nav.sendView(lat, lon, zoom, headingDeg, speedMs: speedMs);

  @override
  Future<NavFrameResult> sendRoute(List<({double lat, double lon})> pts) =>
      _nav.sendRoute(pts);

  @override
  Future<void> sendGuide({
    required int turn,
    required int distM,
    required int remainingM,
    required int remainingS,
    required bool offRoute,
  }) =>
      _nav.sendGuide(
        turn: turn,
        distM: distM,
        remainingM: remainingM,
        remainingS: remainingS,
        offRoute: offRoute,
      );
}

class NavBackground {
  NavBackground(NavStream nav) : _feed = HeadUnitFeed(
          tiles: () => _cache,
          link: _DirectLink(nav),
        );

  final HeadUnitFeed _feed;

  static TileCache? _cache;
  static Future<void>? _cacheJob;

  StreamSubscription<GeoFix>? _fixSub;
  bool _visible = false;
  // Assume the UI is NOT feeding until it says so: a service that starts on
  // boot (or after the UI was killed) must feed the head unit on its own.
  bool _uiActive = false;
  bool _streamEnabled = true;
  bool _disposed = false;

  HeadUnitFeed get feed => _feed;

  /// The head unit's navigator screen came up / went away.
  void setVisible(bool v) {
    if (_visible == v) return;
    _visible = v;
    _sync();
  }

  /// The UI isolate is awake and feeding (true) or asleep (false).
  void setUiActive(bool active) {
    if (_uiActive == active) return;
    _uiActive = active;
    _sync();
  }

  /// The rider's "stream to display" switch, forwarded from the UI.
  void setStreamEnabled(bool on) {
    if (_streamEnabled == on) return;
    _streamEnabled = on;
    _sync();
  }

  /// Continue a ride the UI was guiding when it went to sleep.
  void handoff({
    List<({double lat, double lon})>? route,
    ({int turn, int distM, int remainingM, int remainingS, bool offRoute})?
        guide,
    int? zoom,
    LatLon? lookAt,
  }) {
    if (zoom != null) _feed.setZoom(zoom);
    _feed.setLookAt(lookAt);
    _feed.setRoute(route, route);
    if (guide != null) {
      _feed.setGuide(
        turn: guide.turn,
        distM: guide.distM,
        remainingM: guide.remainingM,
        remainingS: guide.remainingS,
        offRoute: guide.offRoute,
      );
    }
  }

  bool get _shouldRun => _visible && !_uiActive && _streamEnabled && !_disposed;

  void _sync() {
    if (_shouldRun) {
      unawaited(_start());
    } else {
      unawaited(_stop());
    }
  }

  Future<void> _start() async {
    if (_disposed || _feed.running) return;
    await _ensureCache();
    if (_disposed || !_shouldRun) return;
    // Only the receiver while we are actually the ones feeding.
    if (_fixSub == null && await LocationService().hasPermission()) {
      _fixSub = LocationService().fixes().listen((f) {
        _feed.setPosition(f.position,
            headingDeg: f.headingDeg, speedMs: f.speedMs);
      });
    }
    _feed.start();
    AppLog.i('nav', 'background feed on (head unit visible, UI asleep)');
  }

  Future<void> _stop() async {
    if (_feed.running) await _feed.stop();
    await _fixSub?.cancel();
    _fixSub = null;
  }

  Future<void> _ensureCache() async {
    if (_cache != null) return;
    _cacheJob ??= _buildCache();
    await _cacheJob;
  }

  static Future<void> _buildCache() async {
    try {
      final support = await getApplicationSupportDirectory();
      final info = await PackageInfo.fromPlatform();
      final ua = 'AaBridgeNavigator/${info.version} '
          '(+https://github.com/payalneg/esp32p4-android-auto)';
      _cache = TileCache(
        Directory('${support.path}/tiles/osm'),
        userAgent: ua,
        krakow: await _readKrakow(support),
      );
    } on Object catch (e) {
      AppLog.w('nav', 'background tile cache unavailable: $e');
    }
  }

  static Future<KrakowTileSource?> _readKrakow(Directory support) async {
    try {
      final sel = File('${support.path}/nav/krakow-v1/current');
      if (!await sel.exists()) return null;
      final version = (await sel.readAsString()).trim();
      if (!RegExp(r'^\d+$').hasMatch(version)) return null;
      final path = '${sel.parent.path}/$version';
      return await Directory(path).exists()
          ? KrakowTileSource(path, version)
          : null;
    } on Object {
      return null;
    }
  }

  Future<void> dispose() async {
    _disposed = true;
    await _stop();
    await _feed.dispose();
  }
}
