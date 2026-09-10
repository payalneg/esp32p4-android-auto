/// State behind the navigator screen: the two chosen points, the route, and
/// where the rider is on it.
///
/// A [ChangeNotifier] like the rest of the app — no state-management package.
/// GPS fixes and simulated fixes arrive through the same [attachFixes] entry,
/// so there is one guidance path to reason about and to test.
library;

import 'dart:async';

import 'package:flutter/foundation.dart';

import '../settings/nav_settings.dart';
import 'geo.dart';
import 'location_service.dart';
import 'map_data.dart';
import 'ride_simulator.dart';
import 'route_guide.dart';
import 'router.dart';
import 'way_classes.dart';

/// What the next tap on the map means.
enum TapMode { none, start, finish }

class NavController extends ChangeNotifier {
  NavController({MapData? mapData, NavSettings? settings})
      : _mapData = mapData ?? MapData.instance,
        _settings = settings ?? NavSettings.instance {
    _profile = _settings.profile;
  }

  final MapData _mapData;
  final NavSettings _settings;

  LatLon? start;
  LatLon? finish;
  RouteResult? route;
  RouteGuide? guide;
  Guidance? guidance;
  GeoFix? lastFix;

  TapMode tapMode = TapMode.none;
  bool routing = false;
  bool simulating = false;
  bool follow = false;

  /// i18n key for a transient hint or error under the map.
  String? messageKey;

  /// Wall time of the last A* call — printed in debug builds so the decision
  /// to keep routing on the UI isolate stays evidence-based.
  Duration lastRouteTime = Duration.zero;

  StreamSubscription<GeoFix>? _fixes;

  /// Seeded from the saved preference; [setProfile] persists changes back.
  late RideProfile _profile;
  RideProfile get profile => _profile;

  bool get hasRoute => route != null;
  bool get gpsActive => _fixes != null && !simulating;

  /// Cycles the tap mode: first tap sets the start, second the finish. With a
  /// live position we already know where the rider is, so one tap is enough.
  void toggleTapMode() {
    if (tapMode != TapMode.none) {
      tapMode = TapMode.none;
      messageKey = null;
    } else {
      reset(keepFixes: true);
      tapMode = lastFix == null ? TapMode.start : TapMode.finish;
      messageKey = tapMode == TapMode.start
          ? 'nav.route.tapStart'
          : 'nav.route.tapFinish';
    }
    notifyListeners();
  }

  void onMapTap(LatLon p) {
    switch (tapMode) {
      case TapMode.none:
        return;
      case TapMode.start:
        start = p;
        tapMode = TapMode.finish;
        messageKey = 'nav.route.tapFinish';
        notifyListeners();
      case TapMode.finish:
        start ??= lastFix?.position;
        finish = p;
        tapMode = TapMode.none;
        unawaited(recalc());
    }
  }

  /// Sets one end of the route. Routes as soon as both ends are known,
  /// otherwise asks for the missing one.
  Future<void> setStart(LatLon p) async {
    start = p;
    if (finish != null) {
      tapMode = TapMode.none;
      await recalc();
    } else {
      tapMode = TapMode.finish;
      messageKey = 'nav.route.tapFinish';
      notifyListeners();
    }
  }

  Future<void> setFinish(LatLon p) async {
    finish = p;
    if (start != null) {
      tapMode = TapMode.none;
      await recalc();
    } else {
      tapMode = TapMode.start;
      messageKey = 'nav.route.tapStart';
      notifyListeners();
    }
  }

  /// Routes from the current position (or the chosen start) to [target].
  Future<void> routeTo(LatLon target) async {
    start = lastFix?.position ?? start;
    finish = target;
    if (start == null) {
      tapMode = TapMode.start;
      messageKey = 'nav.route.tapStart';
      notifyListeners();
      return;
    }
    await recalc();
  }

  Future<void> setProfile(RideProfile p) async {
    if (p == _profile) return;
    _profile = p;
    notifyListeners();
    await _settings.setProfile(p);
    if (start != null && finish != null) await recalc();
  }

  Future<void> recalc() async {
    final router = _mapData.router;
    final from = start;
    final to = finish;
    if (router == null) {
      messageKey = 'nav.data.missing';
      notifyListeners();
      return;
    }
    if (from == null || to == null) return;

    stopSim();
    routing = true;
    messageKey = 'nav.route.routing';
    notifyListeners();

    final sw = Stopwatch()..start();
    final result = await router.route(from, to, _profile);
    lastRouteTime = sw.elapsed;
    if (kDebugMode) {
      debugPrint('nav: route in ${sw.elapsedMilliseconds} ms '
          '(${result == null ? "none" : "${result.lengthM.round()} m"})');
    }

    routing = false;
    route = result;
    if (result == null) {
      guide = null;
      guidance = null;
      messageKey = 'nav.route.notFound';
    } else {
      guide = RouteGuide(result);
      messageKey = null;
      // Show guidance immediately, from wherever we are on the new route.
      final at = lastFix?.position ?? from;
      guidance = guide!.update(at);
    }
    notifyListeners();
  }

  /// Clears the route. [keepFixes] leaves the position feed running, which is
  /// what a "plan another trip" tap wants.
  void reset({bool keepFixes = false}) {
    if (!keepFixes) detachFixes();
    stopSim();
    start = null;
    finish = null;
    route = null;
    guide = null;
    guidance = null;
    tapMode = TapMode.none;
    messageKey = null;
    notifyListeners();
  }

  // --- position feed ---

  /// Subscribes to a fix stream, replacing any current one.
  void attachFixes(Stream<GeoFix> stream) {
    _fixes?.cancel();
    _fixes = stream.listen(_onFix, onDone: () {
      simulating = false;
      notifyListeners();
    });
    notifyListeners();
  }

  void detachFixes() {
    _fixes?.cancel();
    _fixes = null;
    follow = false;
    notifyListeners();
  }

  void _onFix(GeoFix fix) {
    lastFix = fix;
    final g = guide;
    if (g != null) {
      guidance = g.update(fix.position);
      if (guidance!.arrived) {
        messageKey = 'nav.guide.arrived';
        stopSim();
      }
    }
    notifyListeners();
  }

  void setFollow(bool value) {
    follow = value;
    notifyListeners();
  }

  // --- simulation ---

  void startSim() {
    final g = guide;
    if (g == null) {
      messageKey = 'nav.sim.needRoute';
      notifyListeners();
      return;
    }
    simulating = true;
    follow = true;
    messageKey = null;
    attachFixes(simulateRide(g));
  }

  void stopSim() {
    if (!simulating) return;
    simulating = false;
    _fixes?.cancel();
    _fixes = null;
  }

  @override
  void dispose() {
    _fixes?.cancel();
    super.dispose();
  }
}
