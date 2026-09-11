/// The app's home screen: an offline map you can route on.
///
/// Tiles are raster OpenStreetMap, cached on the phone; the route itself is
/// computed here from the downloaded graph, so building one needs no network
/// at all. Guidance runs off the phone's GPS, or off the ride simulator, which
/// feeds the very same code path.
library;

import 'dart:async';

import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:flutter_map/flutter_map.dart';
import 'package:latlong2/latlong.dart';

import '../ble/ble_proxy.dart';
import '../ble/nav_stream.dart';
import '../bridge/nav_intent_bridge.dart';
import '../bridge/screen_bridge.dart';
import '../i18n/strings.dart';
import '../nav/announcer.dart';
import '../nav/head_unit_feed.dart';
import '../nav/geo.dart';
import '../nav/link_resolver.dart';
import '../nav/location_service.dart';
import '../nav/map_data.dart';
import '../nav/nav_camera.dart';
import '../nav/nav_controller.dart';
import '../nav/nav_link.dart';
import '../nav/router.dart';
import '../nav/voice.dart';
import '../nav/search_index.dart';
import '../nav/tile_cache.dart';
import '../nav/tile_math.dart';
import '../nav/way_classes.dart';
import '../settings/nav_settings.dart';
import 'nav/cached_tile_provider.dart';
import 'nav/maneuver_banner.dart';
import 'nav/route_info_bar.dart';
import 'region_picker_screen.dart';
import 'settings_screen.dart';

/// Rynek Główny — the centre of the sample data, and a better first view than
/// the middle of the Atlantic.
const LatLon kFallbackCentre = LatLon(50.0619, 19.9368);

const String kOsmTileUrl = 'https://tile.openstreetmap.org/{z}/{x}/{y}.png';

/// Zooms a saved area is kept at. 19 is the sharpest OSM serves and is what
/// makes a junction readable; 17 is four times cheaper per square kilometre
/// and covers the ground around it, so zooming out does not hit holes.
const int kSharpZoom = 19;
const int kContextZoom = 17;

/// What the navigating camera shows at speed (zoom 16.5–17 → tiles at 18):
/// the level a whole route is kept at, so the ride never runs off the map,
/// with 19 layered on top from the start for as far as the budget goes.
const int kRouteZoom = 18;

/// How far around the rider each of those reaches. The sharp level is kept
/// tight on purpose: at zoom 19 a tile is about 50 m across, so a wide radius
/// there would be thousands of tiles.
const double kSharpRadiusM = 300;
const double kContextRadiusM = 1500;

/// Riding this far since the last top-up triggers the next one.
const double kPrefetchStepM = 200;


class NavigatorScreen extends StatefulWidget {
  const NavigatorScreen({super.key});

  @override
  State<NavigatorScreen> createState() => _NavigatorScreenState();
}

class _NavigatorScreenState extends State<NavigatorScreen> {
  final _map = MapController();

  /// The head unit's own camera. Its map is a separate widget at the size the
  /// picture is sent in, so it neither follows nor fights the rider's pans.

  /// Its own smoothing, fed the same fixes as the phone's camera — same
  /// algorithm, same input, so the two views agree without sharing state.
  HeadUnitFeed? _feed;
  /// Set once the head-unit map has been laid out, so a camera move before
  /// that does not throw.
  final _controller = NavController();
  final _location = LocationService();
  final _camera = NavCamera();
  final Voice _voice = TtsVoice();
  StreamSubscription<Announcement>? _announceSub;
  StreamSubscription<String>? _linkSub;
  StreamSubscription<LatLon>? _huDestSub;
  StreamSubscription<int>? _huZoomSub;

  /// A map link that arrived before the routing graph was loaded.
  NavLink? _pendingLink;

  /// Whether we hold FLAG_KEEP_SCREEN_ON; follows [NavController.navigating].
  bool _screenPinned = false;

  List<LatLng> _routeLine = const <LatLng>[];

  /// The route the line was built from. Identity, not length: two different
  /// routes can have the same number of points, and comparing the lists
  /// themselves on every GPS fix would be pointless work.
  Object? _lineSource;
  bool _corridorRunning = false;
  bool _cancelCorridor = false;

  /// The running tile job, so a newer one can wait for — or cancel — it.
  Future<void>? _corridorJob;

  /// True while the running job is an automatic route prefetch, which a new
  /// route may cancel; a download the rider started is left to finish.
  bool _autoCorridor = false;
  RouteResult? _prefetchedRoute;
  int _tilesDone = 0;
  int _tilesTotal = 0;
  int _tilesBytes = 0;
  int _tilesBytesAtStart = 0;
  int _tilesFailedAtStart = 0;
  Stopwatch? _tilesClock;
  bool _prefetching = false;
  LatLon? _lastPrefetchAt;

  /// Nudges the tile layer to ask for its tiles again.
  ///
  /// A tile that failed once — a tunnel, a dead second of signal — is marked
  /// errored and is not retried while the camera sits still, which is how the
  /// map ends up blank and stays blank. Firing this after tiles land makes
  /// them appear where they are, without the user having to pan to provoke it.
  final StreamController<void> _tileReset = StreamController<void>.broadcast();

  @override
  void initState() {
    super.initState();
    _controller.addListener(_onControllerChanged);
    _announceSub = _controller.announcements.listen(_onAnnouncement);
    _linkSub = NavIntentBridge.incoming.listen(_openLink);
    MapData.instance.addListener(_onMapDataChanged);
    unawaited(NavIntentBridge.initial().then((payload) {
      if (payload != null && mounted) _openLink(payload);
    }));
    _feed = HeadUnitFeed(
      tiles: () => MapData.instance.tiles,
      link: const _ProxyHeadUnitLink(),
    );
    // The rider can pick somewhere to go on the head unit's own map; the
    // phone is what routes and guides, so the tap comes back here.
    _huDestSub = BleProxy.instance.navDestinations.listen(_onHeadUnitDest);
    // ...and change the zoom there too. The head unit has already redrawn at
    // the new level from what it holds; we are the only one who can fetch the
    // tiles for it.
    _huZoomSub = BleProxy.instance.navZoom.listen((z) => _feed?.setZoom(z));
    _syncStreamer();
  }

  @override
  void dispose() {
    unawaited(_feed?.dispose());
    _feed = null;
    MapData.instance.removeListener(_onMapDataChanged);
    unawaited(_huDestSub?.cancel());
    unawaited(_huZoomSub?.cancel());
    _linkSub?.cancel();
    _announceSub?.cancel();
    unawaited(_voice.stop());
    if (_screenPinned) unawaited(ScreenBridge.keepOn(false));
    _controller.removeListener(_onControllerChanged);
    _controller.dispose();
    _tileReset.close();
    _map.dispose();
    super.dispose();
  }

  void _onControllerChanged() {
    final route = _controller.route;
    // Rebuilding the LatLng list on every GPS fix would churn thousands of
    // objects a minute, so only do it when the route itself changed.
    if (!identical(route, _lineSource)) {
      _lineSource = route;
      _routeLine = route == null
          ? const <LatLng>[]
          : route.points
              .map((p) => LatLng(p.lat, p.lon))
              .toList(growable: false);
      // Not mid-ride: a reroute must not yank the camera out to the whole
      // route while the rider is looking at the next junction.
      if (_routeLine.isNotEmpty && !_controller.navigating) {
        _fitRoute(_routeLine);
      }
      if (route != null) unawaited(_prefetchRoute(route));
      // The head unit draws the line itself, so it gets the shape once.
      _feed?.setRoute(
          route,
          route == null
              ? null
              : <({double lat, double lon})>[
                  for (final p in route.points) (lat: p.lat, lon: p.lon),
                ]);
    }
    final fix = _controller.lastFix;
    if (fix != null) {
      if (_controller.follow) _followCamera(fix);
      _feed?.setPosition(fix.position,
          headingDeg: fix.headingDeg, speedMs: fix.speedMs);
      final g = _controller.guidance;
      if (g != null) {
        _feed?.setGuide(
          turn: kNavTurnOrder.indexOf(g.next.type.name).clamp(0, 8),
          distM: g.nextDistM.round(),
          remainingM: g.remainingM.round(),
          remainingS: g.remainingS.round(),
          offRoute: g.offRoute,
        );
      }
      unawaited(_topUpAroundPosition(fix.position));
    }
    if (_controller.navigating != _screenPinned) {
      _screenPinned = _controller.navigating;
      unawaited(ScreenBridge.keepOn(_screenPinned));
      if (!_screenPinned) {
        _camera.reset();
        _map.rotate(0); // back to north-up, where a paused map reads best
      }
    }
    setState(() {});
  }

  /// Somewhere the rider tapped on the head unit. Treated exactly like a
  /// "navigate" link from another app: route there and start guiding.
  Future<void> _onHeadUnitDest(LatLon at) async {
    if (!mounted) return;
    if (!MapData.instance.isReady) {
      ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(content: Text(t(context, 'nav.data.missing'))));
      return;
    }
    if (await _ensureGps()) await _awaitFix(const Duration(seconds: 3));
    if (!mounted) return;
    await _controller.routeTo(at);
    if (_controller.hasRoute) await _startNavigation();
  }

  /// A link held back for want of a graph is acted on once there is one.
  void _onMapDataChanged() {
    final link = _pendingLink;
    if (link == null || !MapData.instance.isReady) return;
    _pendingLink = null;
    unawaited(_actOnLink(link));
  }

  /// Another app handed us a place — show it, route to it, or go, whichever
  /// the sender asked for.
  void _openLink(String payload) {
    final link = parseNavText(payload);
    if (link == null) {
      ScaffoldMessenger.of(context)
          .showSnackBar(SnackBar(content: Text(t(context, 'nav.link.bad'))));
      return;
    }
    if (!MapData.instance.isReady) {
      _pendingLink = link; // routing and search both need the data
      return;
    }
    unawaited(_actOnLink(link));
  }

  Future<void> _actOnLink(NavLink link) async {
    final short = link.expand;
    if (short != null) {
      final messenger = ScaffoldMessenger.of(context);
      messenger.showSnackBar(SnackBar(
          duration: const Duration(seconds: 10),
          content: Text(t(context, 'nav.link.resolving'))));
      final full = await expandShortLink(short);
      if (!mounted) return;
      messenger.hideCurrentSnackBar();
      final resolved = full == null ? null : parseMapUrl(full);
      if (resolved == null || resolved.expand != null) {
        messenger.showSnackBar(
            SnackBar(content: Text(t(context, 'nav.link.bad'))));
        return;
      }
      return _actOnLink(resolved);
    }
    var point = link.point;
    var display = point == null ? '' : formatCoordinates(point);
    final query = link.query;
    if (point == null && query != null) {
      final hit = MapData.instance.index?.search(query).firstOrNull;
      if (hit == null) {
        ScaffoldMessenger.of(context).showSnackBar(SnackBar(
            content: Text(tf(context, 'nav.link.notFound',
                <String, Object?>{'q': query}))));
        return;
      }
      point = hit.position;
      display = hit.display;
    }
    if (point == null) return;
    if (link.intent != LinkIntent.show) {
      // A route needs a start. Bring the position up first — the cached fix
      // lands within the frame — instead of asking the rider to tap one.
      if (await _ensureGps()) await _awaitFix(const Duration(seconds: 3));
      if (!mounted) return;
    }
    switch (link.intent) {
      case LinkIntent.navigate:
        await _controller.routeTo(point);
        if (_controller.hasRoute) await _startNavigation();
      case LinkIntent.route:
        await _controller.routeTo(point);
      case LinkIntent.show:
        _map.move(LatLng(point.lat, point.lon), 16);
        await _offerDestination(
            SearchHit(display, t(context, 'nav.link.kind'), point));
    }
  }

  /// Resolves once a position is known, or after [limit] without one.
  Future<void> _awaitFix(Duration limit) {
    if (_controller.lastFix != null) return Future<void>.value();
    final done = Completer<void>();
    void check() {
      if (_controller.lastFix != null && !done.isCompleted) done.complete();
    }
    _controller.addListener(check);
    return done.future.timeout(limit, onTimeout: () {}).whenComplete(() {
      _controller.removeListener(check);
    });
  }

  /// Keeps the rider in view. Plain recentring when merely following;
  /// while navigating, the full pose — heading up, ahead-biased, zoom by
  /// speed and by how close the next turn is.
  void _followCamera(GeoFix fix) {
    if (!_controller.navigating) {
      _map.move(LatLng(fix.position.lat, fix.position.lon), _map.camera.zoom);
      return;
    }
    final pose = _camera.pose(fix, _controller.guidance,
        trackUp: _controller.trackUp,
        viewportHeightPx: _map.camera.nonRotatedSize.height);
    _map.moveAndRotate(
        LatLng(pose.center.lat, pose.center.lon), pose.zoom, pose.rotationDeg);
  }

  /// Run the stream only while the rider has asked for it. Whether a head unit
  /// is there, and whether it is showing its navigator screen, is the
  /// streamer's own business — it polls cheaply and sends nothing until both.
  void _syncStreamer() {
    final feed = _feed;
    if (feed == null) return;
    if (NavSettings.instance.streamToDisplay) {
      _seedFeedPosition();
      feed.start();
    } else {
      unawaited(feed.stop());
    }
  }

  /// Point the head unit at whatever the rider's map is showing.
  ///
  /// Without a position the feed has nothing to send, and a phone that has
  /// not got a fix yet — or is indoors — would otherwise leave the display
  /// empty for as long as nobody panned the map.
  void _seedFeedPosition() {
    final feed = _feed;
    if (feed == null || _controller.lastFix != null) return;
    try {
      final c = _map.camera.center;
      feed.setPosition(LatLon(c.latitude, c.longitude));
    } on Object {
      // The map is not laid out yet; onMapReady comes back to this.
    }
  }

  /// A turn coming up is felt and heard — each only if switched on.
  Future<void> _onAnnouncement(Announcement a) async {
    final prefs = NavSettings.instance;
    if (prefs.haptics) {
      switch (a.kind) {
        case AnnouncementKind.now:
        case AnnouncementKind.arrived:
          unawaited(HapticFeedback.heavyImpact());
        case AnnouncementKind.prepare:
        case AnnouncementKind.arriveSoon:
          unawaited(HapticFeedback.mediumImpact());
        case AnnouncementKind.rerouting:
          unawaited(HapticFeedback.vibrate());
      }
    }
    if (!mounted || !prefs.voice) return;
    final text = _phrase(a);
    final lang = LocaleScope.of(context).locale.languageCode == 'ru'
        ? 'ru-RU'
        : 'en-US';
    await _voice.say(text, language: lang);
  }

  String _phrase(Announcement a) {
    String metres() =>
        tf(context, 'nav.say.metres', <String, Object?>{'n': a.distM ?? 0});
    String action() => t(context, 'nav.say.act.${a.maneuver!.name}');
    switch (a.kind) {
      case AnnouncementKind.prepare:
        return tf(context, 'nav.say.prepare',
            <String, Object?>{'dist': metres(), 'action': action()});
      case AnnouncementKind.now:
        return action();
      case AnnouncementKind.arriveSoon:
        return tf(context, 'nav.say.arriveSoon',
            <String, Object?>{'dist': metres()});
      case AnnouncementKind.arrived:
        return t(context, 'nav.say.arrived');
      case AnnouncementKind.rerouting:
        return t(context, 'nav.say.rerouting');
    }
  }

  void _fitRoute(List<LatLng> line) {
    // After the frame: the controller may fire before the map has a size.
    WidgetsBinding.instance.addPostFrameCallback((_) {
      if (!mounted) return;
      _map.fitCamera(CameraFit.bounds(
        bounds: LatLngBounds.fromPoints(line),
        padding: const EdgeInsets.fromLTRB(48, 96, 48, 120),
      ));
    });
  }

  @override
  Widget build(BuildContext context) {
    return ListenableBuilder(
      listenable: Listenable.merge(
          <Listenable>[MapData.instance, NavSettings.instance, _controller]),
      child: const SizedBox.shrink(),
      builder: (context, _) => Scaffold(
        appBar: AppBar(
          title: Text(t(context, 'nav.title')),
          actions: <Widget>[
            IconButton(
              icon: const Icon(Icons.search),
              tooltip: t(context, 'nav.search.hint'),
              onPressed: _openSearch,
            ),
            PopupMenuButton<RideProfile>(
              icon: const Icon(Icons.route_outlined),
              tooltip: t(context, 'nav.profile'),
              initialValue: _controller.profile,
              onSelected: _controller.setProfile,
              itemBuilder: (ctx) => RideProfile.values
                  .map((p) => PopupMenuItem<RideProfile>(
                      value: p, child: Text(t(ctx, p.i18nKey))))
                  .toList(),
            ),
            // Reachable whether or not data is already loaded: riding into the
            // next town is exactly when you need another area.
            PopupMenuButton<String>(
              icon: const Icon(Icons.more_vert),
              onSelected: (choice) {
                switch (choice) {
                  case 'region':
                    Navigator.push(
                        context,
                        MaterialPageRoute(
                            builder: (_) => const RegionPickerScreen()));
                  case 'tiles':
                    _saveMapAroundMe();
                  case 'settings':
                    Navigator.push(
                        context,
                        MaterialPageRoute(
                            builder: (_) => const SettingsScreen()));
                  case 'voice':
                    NavSettings.instance.setVoice(!NavSettings.instance.voice);
                  case 'haptics':
                    NavSettings.instance
                        .setHaptics(!NavSettings.instance.haptics);
                  case 'trackUp':
                    _controller.setTrackUp(!_controller.trackUp);
                  case 'simulator':
                    NavSettings.instance
                        .setSimulator(!NavSettings.instance.simulator);
                  case 'stream':
                    NavSettings.instance.setStreamToDisplay(
                        !NavSettings.instance.streamToDisplay);
                    _syncStreamer();
                }
              },
              itemBuilder: (ctx) => <PopupMenuEntry<String>>[
                CheckedPopupMenuItem<String>(
                  value: 'voice',
                  checked: NavSettings.instance.voice,
                  child: Text(t(ctx, 'nav.voice')),
                ),
                CheckedPopupMenuItem<String>(
                  value: 'haptics',
                  checked: NavSettings.instance.haptics,
                  child: Text(t(ctx, 'nav.haptics')),
                ),
                CheckedPopupMenuItem<String>(
                  value: 'trackUp',
                  checked: _controller.trackUp,
                  child: Text(t(ctx, 'nav.trackUp')),
                ),
                CheckedPopupMenuItem<String>(
                  value: 'stream',
                  checked: NavSettings.instance.streamToDisplay,
                  child: Text(t(ctx, 'nav.stream')),
                ),
                CheckedPopupMenuItem<String>(
                  value: 'simulator',
                  checked: NavSettings.instance.simulator,
                  child: Text(t(ctx, 'nav.simulator')),
                ),
                const PopupMenuDivider(),
                PopupMenuItem<String>(
                  value: 'region',
                  child: ListTile(
                    leading: const Icon(Icons.public),
                    title: Text(t(ctx, 'mapdata.region.title')),
                    contentPadding: EdgeInsets.zero,
                  ),
                ),
                PopupMenuItem<String>(
                  value: 'tiles',
                  child: ListTile(
                    leading: const Icon(Icons.download_for_offline_outlined),
                    title: Text(t(ctx, 'mapdata.area.save')),
                    contentPadding: EdgeInsets.zero,
                  ),
                ),
                PopupMenuItem<String>(
                  value: 'settings',
                  child: ListTile(
                    leading: const Icon(Icons.settings),
                    title: Text(t(ctx, 'settings.title')),
                    contentPadding: EdgeInsets.zero,
                  ),
                ),
              ],
            ),
          ],
        ),
        // The map runs edge to edge — it looks better and the gesture bar sits
        // over ground, not over controls — but everything readable or tappable
        // is inset, or the navigation bar eats it.
        extendBody: true,
        body: Stack(
          children: <Widget>[
            _buildMap(context),
            if (MapData.instance.state == MapDataState.absent ||
                MapData.instance.state == MapDataState.error)
              _dataBanner(context),
            if (_controller.guidance != null && _controller.hasRoute)
              Positioned(
                top: 12,
                left: 0,
                right: 0,
                child: Center(
                    child: ManeuverBanner(guidance: _controller.guidance!)),
              ),
            Positioned(
              left: 12,
              right: 12,
              bottom: 12 + MediaQuery.paddingOf(context).bottom,
              child: Center(
                child: RouteInfoBar(
                  controller: _controller,
                  onSaveOffline: (_controller.hasRoute ||
                          _controller.lastFix != null) &&
                          !_corridorRunning
                      ? _saveOffline
                      : null,
                ),
              ),
            ),
            _headUnitBadge(context),
            Positioned(
                right: 12,
                bottom: 72 + MediaQuery.paddingOf(context).bottom,
                child: _controls(context)),
            if (MapData.instance.state == MapDataState.downloading ||
                MapData.instance.state == MapDataState.loading)
              _busyOverlay(context)
            // Below the turn plate when there is one; never over it mid-ride.
            else if (_corridorRunning && !_controller.navigating)
              _tilesOverlay(context,
                  top: _controller.guidance != null && _controller.hasRoute
                      ? 92
                      : 12),
          ],
        ),
      ),
    );
  }

  /// The head unit used to get a thumbnail here — a second, smaller map of
  /// its own, rendered on the phone and streamed over as pictures. It draws
  /// its own map from the tiles now, so all that is left to say is when it
  /// cannot: an old firmware that does not speak the tile protocol, or a
  /// display showing something else entirely.
  Widget _headUnitBadge(BuildContext context) {
    if (!NavSettings.instance.streamToDisplay) return const SizedBox.shrink();
    return Positioned(
      left: 12,
      bottom: 72 + MediaQuery.paddingOf(context).bottom,
      child: StreamBuilder<BleConnState>(
        stream: BleProxy.instance.state,
        initialData: BleProxy.instance.currentState,
        builder: (ctx, snap) {
          final ble = BleProxy.instance;
          if (snap.data != BleConnState.connected) {
            return const SizedBox.shrink();
          }
          final String? text = !ble.supportsNavStream
              ? t(ctx, 'nav.hu.badge.oldFirmware')
              : !ble.navState.visible
                  ? t(ctx, 'nav.hu.badge.other')
                  : null;
          if (text == null) return const SizedBox.shrink();
          return DecoratedBox(
            decoration: BoxDecoration(
              color:
                  Theme.of(ctx).colorScheme.surface.withValues(alpha: 0.9),
              borderRadius: BorderRadius.circular(8),
            ),
            child: Padding(
              padding: const EdgeInsets.symmetric(horizontal: 10, vertical: 6),
              child: Row(mainAxisSize: MainAxisSize.min, children: <Widget>[
                const Icon(Icons.tv_off, size: 16),
                const SizedBox(width: 6),
                Text(text, style: Theme.of(ctx).textTheme.labelMedium),
              ]),
            ),
          );
        },
      ),
    );
  }

  Widget _buildMap(BuildContext context) {
    final saved = NavSettings.instance.lastView;
    final tiles = MapData.instance.tiles;
    final fix = _controller.lastFix;
    return FlutterMap(
      mapController: _map,
      options: MapOptions(
        initialCenter: saved == null
            ? LatLng(kFallbackCentre.lat, kFallbackCentre.lon)
            : LatLng(saved.$1, saved.$2),
        initialZoom: saved?.$3 ?? 13,
        minZoom: 3,
        maxZoom: 19,
        backgroundColor: Theme.of(context).colorScheme.surfaceContainerLowest,
        onTap: (_, p) => _controller.onMapTap(LatLon(p.latitude, p.longitude)),
        // Press and hold anywhere: the same choices a search result gets.
        onLongPress: (_, p) => _offerPoint(LatLon(p.latitude, p.longitude)),
        onPositionChanged: (camera, hasGesture) {
          if (hasGesture && _controller.follow) _controller.setFollow(false);
          // With no fix yet, the head unit follows what the rider is looking
          // at — otherwise it would have nowhere to point its own map.
          if (_controller.lastFix == null) {
            _feed?.setPosition(
                LatLon(camera.center.latitude, camera.center.longitude));
          }
          unawaited(NavSettings.instance.saveLastView(
              camera.center.latitude, camera.center.longitude, camera.zoom));
        },
        onMapReady: _seedFeedPosition,
        // Rotation would put the labels of a raster tile on their side.
        interactionOptions: const InteractionOptions(
            flags: InteractiveFlag.all & ~InteractiveFlag.rotate),
      ),
      children: <Widget>[
        if (tiles != null)
          TileLayer(
            urlTemplate: kOsmTileUrl,
            tileProvider: CachedTileProvider(tiles),
            reset: _tileReset.stream,
            userAgentPackageName: 'com.aabridge.aa_bridge',
            // Retina simulation shifts the layer down a level: flutter_map
            // takes one off both of these and adds it back when it asks for a
            // tile. maxZoom 20 therefore lets the camera go to 19 — left at 19
            // the map would stop drawing above 18, blank exactly where you
            // zoom in to read a junction. maxNativeZoom 19 keeps the deepest
            // tile actually requested at OSM's real limit of 19; 20 asked for
            // a zoom that does not exist and came back 400.
            maxZoom: 20,
            maxNativeZoom: 19,
            // OSM serves 256 px tiles at 1x; on a phone at 2.6x they are
            // stretched and the labels turn to mush. There are no @2x tiles to
            // ask for, so flutter_map's simulation is the way: fetch one zoom
            // deeper and draw it at half size. Costs four times the tiles,
            // buys a readable map.
            retinaMode: RetinaMode.isHighDensity(context),
            errorImage: MemoryImage(kTransparentPng),
            evictErrorTileStrategy: EvictErrorTileStrategy.notVisibleRespectMargin,
          ),
        if (_routeLine.isNotEmpty)
          PolylineLayer<Object>(
            polylines: <Polyline<Object>>[
              Polyline<Object>(
                points: _routeLine,
                color: const Color(0xFF1E64DC),
                strokeWidth: 6,
                borderColor: Colors.white,
                borderStrokeWidth: 2,
              ),
            ],
          ),
        MarkerLayer(
          markers: <Marker>[
            if (_controller.start != null)
              _pin(_controller.start!, Colors.green, Icons.trip_origin),
            if (_controller.finish != null)
              _pin(_controller.finish!, Colors.redAccent, Icons.place),
            if (fix != null)
              Marker(
                point: LatLng(fix.position.lat, fix.position.lon),
                width: 28,
                height: 28,
                child: _PositionDot(headingDeg: fix.headingDeg),
              ),
          ],
        ),
        RichAttributionWidget(
          attributions: <SourceAttribution>[
            TextSourceAttribution(t(context, 'nav.attribution')),
          ],
        ),
      ],
    );
  }

  Marker _pin(LatLon at, Color color, IconData icon) => Marker(
        point: LatLng(at.lat, at.lon),
        width: 34,
        height: 34,
        alignment: Alignment.topCenter,
        child: Icon(icon, color: color, size: 30, shadows: const <Shadow>[
          Shadow(color: Colors.black54, blurRadius: 4),
        ]),
      );

  /// Fetching an area takes seconds and blocks nothing else, but the map must
  /// say so — otherwise a tap on "download" looks like it did nothing.
  Widget _busyOverlay(BuildContext context) {
    final data = MapData.instance;
    final text = data.state == MapDataState.downloading
        ? tf(context, 'mapdata.downloading',
            <String, Object?>{'file': data.progressFile ?? ''})
        : t(context, 'mapdata.status.loading');
    return _statusCard(context, text, null);
  }

  /// Tiles are the long half of a download — thousands of them for a city at
  /// every scale — so they get a count, a bar and a way out, not a spinner
  /// that says nothing.
  Widget _tilesOverlay(BuildContext context, {double top = 12}) {
    final mb = _tilesBytes / (1 << 20);
    final seconds = (_tilesClock?.elapsedMilliseconds ?? 0) / 1000;
    final cache = MapData.instance.tiles;
    final failed = (cache?.failures ?? 0) - _tilesFailedAtStart;
    // Tiles come in at tens of kilobytes a second; in MB/s that reads 0.0.
    final mbps = seconds > 0 ? mb / seconds : 0.0;
    final rate = mbps >= 0.1
        ? tf(context, 'mapdata.tiles.rateMb',
            <String, Object?>{'n': mbps.toStringAsFixed(1)})
        : tf(context, 'mapdata.tiles.rateKb',
            <String, Object?>{'n': (mbps * 1024).round()});
    return _statusCard(
      context,
      tf(context, 'mapdata.tiles.progress', <String, Object?>{
            'done': _tilesDone,
            'total': _tilesTotal,
            'mb': mb.toStringAsFixed(1),
            'rate': rate,
          }) +
          // A count that sits at zero means one of two very different things;
          // the failure tally is what tells them apart at a glance.
          (failed > 0
              ? tf(context, 'mapdata.tiles.failing', <String, Object?>{
                  'n': failed,
                  'err': cache?.lastError ?? '',
                })
              : ''),
      _tilesTotal > 0 ? _tilesDone / _tilesTotal : null,
      onCancel: () => setState(() => _cancelCorridor = true),
      top: top,
    );
  }

  Widget _statusCard(BuildContext context, String text, double? progress,
      {VoidCallback? onCancel, double top = 12}) {
    return Positioned(
      top: top,
      left: 12,
      right: 12,
      child: Card(
        color: Theme.of(context).colorScheme.surfaceContainerHigh,
        child: Padding(
          padding: const EdgeInsets.all(16),
          child: Column(
            mainAxisSize: MainAxisSize.min,
            children: <Widget>[
              Row(
                children: <Widget>[
                  const SizedBox(
                      width: 20,
                      height: 20,
                      child: CircularProgressIndicator(strokeWidth: 2)),
                  const SizedBox(width: 16),
                  Expanded(child: Text(text)),
                  if (onCancel != null)
                    IconButton(
                      icon: const Icon(Icons.close),
                      tooltip: t(context, 'nav.route.reset'),
                      onPressed: onCancel,
                    ),
                ],
              ),
              if (progress != null) ...<Widget>[
                const SizedBox(height: 10),
                LinearProgressIndicator(value: progress),
              ],
            ],
          ),
        ),
      ),
    );
  }

  /// Without a graph there is nothing to route on, so this leads straight to
  /// the one place that provides one.
  Widget _dataBanner(BuildContext context) => Positioned(
        top: 12,
        left: 12,
        right: 12,
        child: Card(
          color: Theme.of(context).colorScheme.secondaryContainer,
          child: ListTile(
            leading: const Icon(Icons.map_outlined),
            title: Text(MapData.instance.state == MapDataState.error
                ? t(context, 'settings.mapdata.subtitle.error')
                : t(context, 'nav.data.missing')),
            // An error used to leave this screen blank: no banner, no
            // progress, nothing to act on. Say what happened here, where the
            // user is, not only on the map-data screen.
            subtitle: Text(
                MapData.instance.messageKey == null
                    ? t(context, 'mapdata.area.source')
                    : tf(context, MapData.instance.messageKey!,
                        MapData.instance.messageArgs ?? const {}),
                maxLines: 3,
                overflow: TextOverflow.ellipsis,
                style: Theme.of(context).textTheme.bodySmall),
            trailing: FilledButton(
              onPressed: () => Navigator.push(context,
                  MaterialPageRoute(builder: (_) => const RegionPickerScreen())),
              child: Text(t(context, 'nav.data.missing.action')),
            ),
          ),
        ),
      );

  /// Saves the picture around the rider at every scale.
  ///
  /// Roads come from a regional extract now; this is only the map behind
  /// them, and the two are worth keeping separate — a region is a rare,
  /// deliberate download, while the tiles you want are wherever you happen
  /// to be standing.
  Future<void> _saveMapAroundMe() =>
      _saveAreaTiles(
          _controller.lastFix?.position ??
              LatLon(_map.camera.center.latitude, _map.camera.center.longitude),
          NavSettings.instance.areaRadiusKm * 1000.0);

  /// One zoom step on our own map. Clamped to what the tile source has, and
  /// around the camera's centre so it behaves the same whether the map is
  /// following the rider or has been dragged away.
  void _zoomBy(double delta) {
    final cam = _map.camera;
    final z = (cam.zoom + delta).clamp(3.0, 19.0);
    if (z == cam.zoom) return;
    _map.move(cam.center, z);
  }

  Widget _controls(BuildContext context) {
    final hasRoute = _controller.hasRoute;
    final navigating = _controller.navigating;
    final scheme = Theme.of(context).colorScheme;
    return Column(
      mainAxisSize: MainAxisSize.min,
      crossAxisAlignment: CrossAxisAlignment.end,
      children: <Widget>[
        // Zoom, for a rider in gloves: the pinch gesture is there, but two
        // thumb-sized buttons are what works one-handed on a bike.
        FloatingActionButton.small(
          heroTag: 'nav-zoom-in',
          tooltip: t(context, 'nav.zoomIn'),
          onPressed: () => _zoomBy(1),
          child: const Icon(Icons.add),
        ),
        const SizedBox(height: 8),
        FloatingActionButton.small(
          heroTag: 'nav-zoom-out',
          tooltip: t(context, 'nav.zoomOut'),
          onPressed: () => _zoomBy(-1),
          child: const Icon(Icons.remove),
        ),
        const SizedBox(height: 8),
        FloatingActionButton.small(
          heroTag: 'nav-follow',
          tooltip: t(context, 'nav.follow'),
          backgroundColor: _controller.follow ? scheme.primaryContainer : null,
          onPressed: _toggleFollow,
          child: Icon(_controller.gpsActive
              ? Icons.my_location
              : Icons.location_searching),
        ),
        if (navigating) ...<Widget>[
          const SizedBox(height: 8),
          FloatingActionButton.small(
            heroTag: 'nav-compass',
            tooltip: t(context, 'nav.trackUp'),
            backgroundColor:
                _controller.trackUp ? scheme.primaryContainer : null,
            onPressed: () => _controller.setTrackUp(!_controller.trackUp),
            child: Icon(_controller.trackUp
                ? Icons.navigation
                : Icons.explore_outlined),
          ),
        ] else ...<Widget>[
          const SizedBox(height: 8),
          FloatingActionButton.small(
            heroTag: 'nav-pick',
            tooltip: t(context, 'nav.route.mode'),
            backgroundColor: _controller.tapMode != TapMode.none
                ? scheme.primaryContainer
                : null,
            onPressed: _controller.toggleTapMode,
            child: const Icon(Icons.add_location_alt_outlined),
          ),
        ],
        if (hasRoute) ...<Widget>[
          // A developer's ride-along; off the screen unless asked for.
          if (NavSettings.instance.simulator || _controller.simulating) ...<Widget>[
            const SizedBox(height: 8),
            FloatingActionButton.small(
              heroTag: 'nav-sim',
              tooltip: t(context,
                  _controller.simulating ? 'nav.sim.stop' : 'nav.sim.start'),
              onPressed: () => _controller.simulating
                  ? _controller.stopSim()
                  : _controller.startSim(),
              child: Icon(
                  _controller.simulating ? Icons.stop : Icons.play_arrow),
            ),
          ],
          if (!navigating) ...<Widget>[
            const SizedBox(height: 8),
            FloatingActionButton.small(
              heroTag: 'nav-reset',
              tooltip: t(context, 'nav.route.reset'),
              onPressed: () => _controller.reset(keepFixes: true),
              child: const Icon(Icons.close),
            ),
          ],
          const SizedBox(height: 8),
          // The one big button: what you press when the route looks right.
          navigating
              ? FloatingActionButton.extended(
                  heroTag: 'nav-go',
                  backgroundColor: scheme.errorContainer,
                  foregroundColor: scheme.onErrorContainer,
                  onPressed: _controller.stopNavigation,
                  icon: const Icon(Icons.stop),
                  label: Text(t(context, 'nav.stop')),
                )
              : FloatingActionButton.extended(
                  heroTag: 'nav-go',
                  onPressed: _startNavigation,
                  icon: const Icon(Icons.navigation),
                  label: Text(t(context, 'nav.start')),
                ),
        ],
      ],
    );
  }

  Future<void> _startNavigation() async {
    // The simulator is a position feed too; otherwise we need the GPS.
    if (!_controller.simulating && !await _ensureGps()) return;
    if (!mounted) return;
    _camera.reset();
    _controller.startNavigation();
  }

  Future<void> _toggleFollow() async {
    if (_controller.gpsActive) {
      _controller.setFollow(!_controller.follow);
      return;
    }
    if (await _ensureGps()) _controller.setFollow(true);
  }

  /// Starts the GPS feed if it is not running. False — with the remedy on a
  /// snackbar — when it cannot.
  Future<bool> _ensureGps() async {
    if (_controller.gpsActive) return true;
    final status = await _location.ensurePermission();
    if (!mounted) return false;
    if (status != LocationStatus.ok) {
      final key = switch (status) {
        LocationStatus.serviceOff => 'nav.gps.off',
        LocationStatus.deniedForever => 'nav.gps.deniedForever',
        _ => 'nav.gps.denied',
      };
      ScaffoldMessenger.of(context).showSnackBar(SnackBar(
        content: Text(t(context, key)),
        action: SnackBarAction(
          label: t(context, 'nav.gps.openSettings'),
          onPressed: () => _location.openSystemSettings(status),
        ),
      ));
      return false;
    }
    _controller.attachFixes(_location.fixes());
    // Only say "waiting" if there is actually a wait: a cached position
    // normally lands within the frame.
    unawaited(Future<void>.delayed(const Duration(seconds: 3), () {
      if (!mounted || _controller.lastFix != null || !_controller.gpsActive) {
        return;
      }
      ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(content: Text(t(context, 'nav.gps.waiting'))));
    }));
    return true;
  }

  Future<void> _openSearch() async {
    // No index is no reason not to open it: coordinates need none.
    final hit = await showSearch<SearchHit?>(
        context: context,
        delegate: _PlaceSearchDelegate(MapData.instance.index, context));
    if (hit == null || !mounted) return;
    _map.move(LatLng(hit.position.lat, hit.position.lon), 16);
    await _offerDestination(hit);
  }

  /// A spot on the map, held down.
  Future<void> _offerPoint(LatLon p) => _offerDestination(
      SearchHit(formatCoordinates(p), t(context, 'nav.search.coordinates'), p));

  Future<void> _offerDestination(SearchHit hit) async {
    await showModalBottomSheet<void>(
      context: context,
      builder: (ctx) => SafeArea(
        child: Column(
          mainAxisSize: MainAxisSize.min,
          children: <Widget>[
            ListTile(title: Text(hit.display), subtitle: Text(hit.kind)),
            const Divider(height: 1),
            ListTile(
              leading: const Icon(Icons.navigation_outlined),
              title: Text(t(ctx, 'nav.route.fromHere')),
              onTap: () {
                Navigator.pop(ctx);
                _controller.routeTo(hit.position);
              },
            ),
            ListTile(
              leading: const Icon(Icons.place_outlined),
              title: Text(t(ctx, 'nav.route.setFinish')),
              onTap: () {
                Navigator.pop(ctx);
                _controller.setFinish(hit.position);
              },
            ),
            ListTile(
              leading: const Icon(Icons.trip_origin),
              title: Text(t(ctx, 'nav.route.setStart')),
              onTap: () {
                Navigator.pop(ctx);
                _controller.setStart(hit.position);
              },
            ),
          ],
        ),
      ),
    );
  }

  /// Saves the map around the rider, along the route and at its far end, so
  /// the ride survives losing signal.
  ///
  /// Three places matter and they are not the same place: where you are now,
  /// the line you will follow, and the streets around the destination where
  /// you will be looking for a door.
  Future<void> _saveOffline({bool quiet = false}) =>
      _corridorJob = _runSaveOffline(quiet: quiet);

  /// Tiles along a new route, fetched as soon as it exists — quietly, behind
  /// the map's own requests — so the ride survives a dead patch of signal
  /// without anyone remembering to press "save". A route that is replaced
  /// (a reroute, a new destination) cancels the download for the old one; a
  /// download the rider started themselves is left to finish first.
  Future<void> _prefetchRoute(RouteResult route) async {
    if (identical(_prefetchedRoute, route)) return;
    _prefetchedRoute = route;
    if (_corridorRunning && _autoCorridor) _cancelCorridor = true;
    await _corridorJob;
    if (!mounted || !identical(_prefetchedRoute, route)) return; // superseded
    _autoCorridor = true;
    try {
      await _saveOffline(quiet: true);
    } finally {
      _autoCorridor = false;
    }
  }

  Future<void> _runSaveOffline({required bool quiet}) async {
    final cache = MapData.instance.tiles;
    if (cache == null) return;
    final route = _controller.route;
    final here = _controller.lastFix?.position;
    final finish = _controller.finish;

    final wanted = <TileId>[];
    final seen = <TileId>{};
    void add(Iterable<TileId> tiles) {
      for (final t in tiles) {
        if (seen.add(t)) wanted.add(t);
      }
    }

    // Order is what survives the cap. The whole route at the riding zoom
    // first — cheap, and it means the map never runs out under the line —
    // then full detail: where you stand, then along the route from its
    // start, then the far end where you will be looking for a door.
    if (route != null) {
      add(corridorTiles(route.points,
          zooms: const <int>[kRouteZoom], maxTiles: 1 << 20));
    }
    if (here != null) {
      add(tilesAround(here,
          radiusM: kSharpRadiusM,
          zooms: const <int>[kSharpZoom],
          maxTiles: 1 << 20));
    }
    if (route != null) {
      add(corridorTiles(route.points,
          zooms: const <int>[kSharpZoom], maxTiles: 1 << 20));
    }
    if (finish != null) {
      add(tilesAround(finish,
          radiusM: kSharpRadiusM,
          zooms: const <int>[kSharpZoom],
          maxTiles: 1 << 20));
    }
    // Context last: it is the part worth dropping when the cap bites.
    for (final centre in <LatLon?>[here, finish]) {
      if (centre == null) continue;
      add(tilesAround(centre,
          radiusM: kContextRadiusM,
          zooms: const <int>[kContextZoom],
          maxTiles: 1 << 20));
    }
    if (wanted.isEmpty) {
      if (!quiet) {
        ScaffoldMessenger.of(context).showSnackBar(SnackBar(
            content: Text(t(context, 'mapdata.corridor.needRoute'))));
      }
      return;
    }

    final cap = NavSettings.instance.corridorMaxTiles;
    final tiles = wanted.length <= cap ? wanted : wanted.sublist(0, cap);
    setState(() {
      _corridorRunning = true;
      _cancelCorridor = false;
      _tilesDone = 0;
      _tilesTotal = tiles.length;
      _tilesBytes = 0;
      _tilesBytesAtStart = cache.bytesFetched;
      _tilesFailedAtStart = cache.failures;
      _tilesClock = Stopwatch()..start();
    });
    final messenger = ScaffoldMessenger.of(context);
    final report = await cache.downloadCorridor(
      tiles,
      _tileUrls,
      onProgress: (done, total) {
        _refreshTilesEvery(done, total);
        if (!mounted) return;
        setState(() {
          _tilesDone = done;
          _tilesBytes = cache.bytesFetched - _tilesBytesAtStart;
        });
      },
      cancelled: () => _cancelCorridor || !mounted,
    );
    await cache.evictToCap(NavSettings.instance.tileCapMb << 20);
    if (!mounted) return;
    setState(() => _corridorRunning = false);
    _refreshTiles();
    // Quiet means quiet — except a refusal from the tile server, which the
    // rider should know about whoever started the download.
    if (!quiet || report.blocked) {
      messenger.showSnackBar(SnackBar(content: Text(_corridorMessage(report))));
    }
  }

  /// Saves the picture of an area at every scale the map can show.
  ///
  /// Offline has to survive zooming out to find your bearings as much as
  /// zooming in to read a house number, so this is a pyramid: coarse levels
  /// first — nearly free, and what keeps the map legible if the tile budget
  /// cuts the download short — then finer ones, outwards from the middle.
  Future<void> _saveAreaTiles(LatLon centre, double radiusM) =>
      _corridorJob = _runSaveAreaTiles(centre, radiusM);

  Future<void> _runSaveAreaTiles(LatLon centre, double radiusM) async {
    final cache = MapData.instance.tiles;
    if (cache == null) return;
    final tiles = areaPyramid(centre,
        radiusM: radiusM, maxTiles: NavSettings.instance.tileBudget);
    setState(() {
      _corridorRunning = true;
      _cancelCorridor = false;
      _tilesDone = 0;
      _tilesTotal = tiles.length;
      _tilesBytes = 0;
      _tilesBytesAtStart = cache.bytesFetched;
      _tilesFailedAtStart = cache.failures;
      _tilesClock = Stopwatch()..start();
    });
    final messenger = ScaffoldMessenger.of(context);
    final report = await cache.downloadCorridor(tiles, _tileUrls,
        onProgress: (done, total) {
          _refreshTilesEvery(done, total);
          if (mounted) {
            setState(() {
              _tilesDone = done;
              // Delta, not the cache's lifetime total: this download is what
              // the user is watching.
              _tilesBytes = cache.bytesFetched - _tilesBytesAtStart;
            });
          }
        },
        cancelled: () => _cancelCorridor || !mounted);
    await cache.evictToCap(NavSettings.instance.tileCapMb << 20);
    if (!mounted) return;
    setState(() => _corridorRunning = false);
    _refreshTiles();
    messenger.showSnackBar(SnackBar(content: Text(_corridorMessage(report))));
  }

  /// What to say when a tile job ends — refused, given up on, or finished.
  String _corridorMessage(CorridorReport report) {
    if (report.blocked) return t(context, 'mapdata.corridor.blocked');
    if (report.stalled) {
      return tf(context, 'mapdata.corridor.stalled', <String, Object?>{
        'n': report.downloaded + report.skipped,
        'err': report.lastError ?? '',
      });
    }
    return tf(context, 'mapdata.corridor.done',
        <String, Object?>{'n': report.downloaded + report.skipped});
  }

  /// Asks the layer to re-read its tiles; cached ones then paint immediately.
  void _refreshTiles() {
    if (mounted && !_tileReset.isClosed) _tileReset.add(null);
  }

  /// Same, but rarely during a long download.
  ///
  /// A reset drops and re-creates every tile on screen, not only the missing
  /// ones. Tiles the rider can see are fetched ahead of the download anyway
  /// (TilePriority.view), so this only has to catch the odd tile that failed
  /// on a bad second of signal and has since landed on disk — every hundred
  /// is plenty for that, and every ten made the map churn.
  void _refreshTilesEvery(int done, int total) {
    if (done % 100 == 0 || done == total) _refreshTiles();
  }

  /// Addresses to try for one tile: the OSMF server first, then its mirrors,
  /// so a refusal or a flaky host does not leave a hole in the map.
  static List<Uri> _tileUrls(TileId t) => tileUrls(t);

  /// Quietly keeps the ground around the rider cached while moving.
  ///
  /// Only a tight ring at the sharp zoom, only once the rider has actually
  /// travelled, and never two at a time — this rides along with a journey, it
  /// is not a download job.
  Future<void> _topUpAroundPosition(LatLon at) async {
    final cache = MapData.instance.tiles;
    if (cache == null || _prefetching || _corridorRunning) return;
    final last = _lastPrefetchAt;
    if (last != null && haversineM(last, at) < kPrefetchStepM) return;
    _prefetching = true;
    _lastPrefetchAt = at;
    try {
      final report = await cache.downloadCorridor(
        tilesAround(at,
            radiusM: kSharpRadiusM,
            zooms: const <int>[kSharpZoom],
            maxTiles: 60),
        _tileUrls,
        onProgress: _refreshTilesEvery,
        cancelled: () => !mounted,
      );
      if (report.downloaded > 0) _refreshTiles();
    } on Object {
      // Offline is the normal case here; the map simply stays as it was.
    } finally {
      _prefetching = false;
    }
  }
}

/// A blue dot with a heading arrow, like every other map app's "you".
class _PositionDot extends StatelessWidget {
  const _PositionDot({this.headingDeg});

  final double? headingDeg;

  @override
  Widget build(BuildContext context) {
    final dot = Container(
      decoration: BoxDecoration(
        color: const Color(0xFF1B6CFF),
        shape: BoxShape.circle,
        border: Border.all(color: Colors.white, width: 3),
        boxShadow: const <BoxShadow>[
          BoxShadow(color: Colors.black38, blurRadius: 4)
        ],
      ),
    );
    final h = headingDeg;
    if (h == null || h.isNaN || h < 0) return dot;
    return Transform.rotate(
      angle: h * 3.1415926535 / 180.0,
      child: Stack(
        alignment: Alignment.center,
        children: <Widget>[
          const Positioned(
              top: -2,
              child: Icon(Icons.arrow_drop_up,
                  color: Color(0xFF1B6CFF), size: 22)),
          dot,
        ],
      ),
    );
  }
}

class _PlaceSearchDelegate extends SearchDelegate<SearchHit?> {
  _PlaceSearchDelegate(this.index, BuildContext context)
      : super(searchFieldLabel: t(context, 'nav.search.hint'));

  /// Null until a region is downloaded; coordinates still work.
  final SearchIndex? index;

  @override
  List<Widget> buildActions(BuildContext context) => <Widget>[
        if (query.isNotEmpty)
          IconButton(
              icon: const Icon(Icons.clear), onPressed: () => query = ''),
      ];

  @override
  Widget buildLeading(BuildContext context) => IconButton(
      icon: const Icon(Icons.arrow_back),
      onPressed: () => close(context, null));

  @override
  Widget buildResults(BuildContext context) => buildSuggestions(context);

  @override
  Widget buildSuggestions(BuildContext context) {
    final q = query.trim();
    if (q.isEmpty) return const SizedBox.shrink();
    final coord = parseCoordinates(q);
    final hits = index?.search(q) ?? const <SearchHit>[];
    if (coord == null && hits.isEmpty) {
      return Center(
          child: Text(t(
              context, index == null ? 'nav.search.noindex' : 'nav.search.empty')));
    }
    final kind = t(context, 'nav.search.coordinates');
    return ListView(
      children: <Widget>[
        // A pasted pair of numbers is a place too — first, since it is what
        // was typed rather than a guess at it.
        if (coord != null)
          ListTile(
            leading: const Icon(Icons.my_location),
            title: Text(formatCoordinates(coord)),
            subtitle: Text(kind),
            onTap: () =>
                close(context, SearchHit(formatCoordinates(coord), kind, coord)),
          ),
        for (final h in hits)
          ListTile(
            leading: const Icon(Icons.place_outlined),
            title: Text(h.display),
            subtitle: Text(h.kind),
            onTap: () => close(context, h),
          ),
      ],
    );
  }
}


class _ProxyHeadUnitLink implements HeadUnitLink {
  const _ProxyHeadUnitLink();

  BleProxy get _ble => BleProxy.instance;

  @override
  bool get available =>
      _ble.currentState == BleConnState.connected && _ble.supportsNavStream;

  @override
  NavDisplayState get displayState => _ble.navState;

  @override
  Stream<NavDisplayState> get displayStates => _ble.navStates;

  @override
  Stream<({int z, int x, int y})> get dropped => _ble.navDropped;

  @override
  Future<NavFrameResult> sendTile(
          int z, int x, int y, int format, Uint8List bytes) =>
      _ble.sendNavTile(z, x, y, format, bytes);

  @override
  Future<NavFrameResult> sendRoute(List<({double lat, double lon})> pts) =>
      _ble.sendNavRoute(pts);

  @override
  Future<void> sendGuide({
    required int turn,
    required int distM,
    required int remainingM,
    required int remainingS,
    required bool offRoute,
  }) async =>
      _ble.sendNavGuide(
        turn: turn,
        distM: distM,
        remainingM: remainingM,
        remainingS: remainingS,
        offRoute: offRoute,
      );

  @override
  Future<void> sendView(double lat, double lon, int zoom, int headingDeg,
          {double speedMs = 0}) async =>
      _ble.sendNavView(lat, lon, zoom, headingDeg, speedMs: speedMs);
}
