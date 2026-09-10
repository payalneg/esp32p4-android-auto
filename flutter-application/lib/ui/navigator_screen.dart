/// The app's home screen: an offline map you can route on.
///
/// Tiles are raster OpenStreetMap, cached on the phone; the route itself is
/// computed here from the downloaded graph, so building one needs no network
/// at all. Guidance runs off the phone's GPS, or off the ride simulator, which
/// feeds the very same code path.
library;

import 'dart:async';
import 'dart:math' as math;

import 'package:flutter/material.dart';
import 'package:flutter_map/flutter_map.dart';
import 'package:latlong2/latlong.dart';

import '../i18n/strings.dart';
import '../nav/geo.dart';
import '../nav/location_service.dart';
import '../nav/map_data.dart';
import '../nav/nav_controller.dart';
import '../nav/overpass.dart';
import '../nav/search_index.dart';
import '../nav/tile_math.dart';
import '../nav/way_classes.dart';
import '../settings/nav_settings.dart';
import 'nav/cached_tile_provider.dart';
import 'nav/maneuver_banner.dart';
import 'nav/route_info_bar.dart';
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

/// How far around the rider each of those reaches. The sharp level is kept
/// tight on purpose: at zoom 19 a tile is about 50 m across, so a wide radius
/// there would be thousands of tiles.
const double kSharpRadiusM = 300;
const double kContextRadiusM = 1500;

/// Riding this far since the last top-up triggers the next one.
const double kPrefetchStepM = 200;

/// How far the road network can reach in one go. Bounded by what Overpass
/// will serve — see kMaxAreaKm2 — not by what the map can show.
const double kMaxRoadRadiusM = 6000;

class NavigatorScreen extends StatefulWidget {
  const NavigatorScreen({super.key});

  @override
  State<NavigatorScreen> createState() => _NavigatorScreenState();
}

class _NavigatorScreenState extends State<NavigatorScreen> {
  final _map = MapController();
  final _controller = NavController();
  final _location = LocationService();

  List<LatLng> _routeLine = const <LatLng>[];

  /// The route the line was built from. Identity, not length: two different
  /// routes can have the same number of points, and comparing the lists
  /// themselves on every GPS fix would be pointless work.
  Object? _lineSource;
  bool _corridorRunning = false;
  bool _cancelCorridor = false;
  int _tilesDone = 0;
  int _tilesTotal = 0;
  int _tilesBytes = 0;
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
  }

  @override
  void dispose() {
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
      if (_routeLine.isNotEmpty) _fitRoute(_routeLine);
    }
    final fix = _controller.lastFix;
    if (fix != null) {
      if (_controller.follow) {
        _map.move(LatLng(fix.position.lat, fix.position.lon), _map.camera.zoom);
      }
      unawaited(_topUpAroundPosition(fix.position));
    }
    setState(() {});
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
                  case 'area':
                    _downloadAreaHere();
                  case 'settings':
                    Navigator.push(
                        context,
                        MaterialPageRoute(
                            builder: (_) => const SettingsScreen()));
                }
              },
              itemBuilder: (ctx) => <PopupMenuEntry<String>>[
                PopupMenuItem<String>(
                  value: 'area',
                  child: ListTile(
                    leading: const Icon(Icons.travel_explore),
                    title: Text(t(ctx, 'nav.data.missing.action')),
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
            if (MapData.instance.state == MapDataState.absent)
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
            Positioned(
                right: 12,
                bottom: 72 + MediaQuery.paddingOf(context).bottom,
                child: _controls(context)),
            if (MapData.instance.state == MapDataState.downloading ||
                MapData.instance.state == MapDataState.loading)
              _busyOverlay(context)
            else if (_corridorRunning)
              _tilesOverlay(context),
          ],
        ),
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
        onPositionChanged: (camera, hasGesture) {
          if (hasGesture && _controller.follow) _controller.setFollow(false);
          unawaited(NavSettings.instance.saveLastView(
              camera.center.latitude, camera.center.longitude, camera.zoom));
        },
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
            // Retina simulation shifts the layer down a level, so these are
            // stated one higher than OSM's real limit of 19. Left at 19 the
            // layer would stop drawing above zoom 18 — a blank map exactly
            // where you zoom in to read a junction.
            maxZoom: 20,
            maxNativeZoom: 20,
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
  Widget _tilesOverlay(BuildContext context) {
    final mb = _tilesBytes / (1 << 20);
    final seconds = (_tilesClock?.elapsedMilliseconds ?? 0) / 1000;
    return _statusCard(
      context,
      tf(context, 'mapdata.tiles.progress', <String, Object?>{
        'done': _tilesDone,
        'total': _tilesTotal,
        'mb': mb.toStringAsFixed(1),
        'rate': (seconds > 0 ? mb / seconds : 0).toStringAsFixed(1),
      }),
      _tilesTotal > 0 ? _tilesDone / _tilesTotal : null,
      onCancel: () => setState(() => _cancelCorridor = true),
    );
  }

  Widget _statusCard(BuildContext context, String text, double? progress,
      {VoidCallback? onCancel}) {
    return Positioned(
      top: 12,
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

  /// Offers to fetch exactly what is on screen — the area you can see is the
  /// area you get, which beats picking a region off a list and hoping.
  Widget _dataBanner(BuildContext context) => Positioned(
        top: 12,
        left: 12,
        right: 12,
        child: Card(
          color: Theme.of(context).colorScheme.secondaryContainer,
          child: ListTile(
            leading: const Icon(Icons.map_outlined),
            title: Text(t(context, 'nav.data.missing')),
            subtitle: Text(t(context, 'mapdata.area.source'),
                style: Theme.of(context).textTheme.bodySmall),
            trailing: FilledButton(
              onPressed: _downloadAreaHere,
              child: Text(t(context, 'nav.data.missing.action')),
            ),
          ),
        ),
      );

  /// Downloads the roads around where we are, and builds the graph.
  ///
  /// Centred on the rider, not on the viewport: what has to work offline is
  /// the ground you are standing on, and a map that happens to be scrolled to
  /// another city is not a request to download that city.
  ///
  /// Replaces whatever area was loaded before — one area at a time keeps the
  /// memory budget honest, and riding two cities at once is not a thing.
  Future<void> _downloadAreaHere() async {
    final centre = _controller.lastFix?.position ??
        LatLon(_map.camera.center.latitude, _map.camera.center.longitude);
    final radiusM = NavSettings.instance.areaRadiusKm * 1000.0;
    final messenger = ScaffoldMessenger.of(context);
    // The picture can go as wide as the user likes; the road network cannot.
    // Overpass answers with roughly a megabyte per square kilometre, so a
    // hundred-kilometre disc would be tens of gigabytes. Roads therefore cover
    // the middle of the area and the tiles cover all of it.
    final roadRadiusM = math.min(radiusM, kMaxRoadRadiusM);
    final dLat = roadRadiusM / 111320.0;
    final dLon = dLat / math.cos(centre.lat * math.pi / 180.0);
    await MapData.instance.buildFromOverpass(GeoBounds(
      south: centre.lat - dLat,
      west: centre.lon - dLon,
      north: centre.lat + dLat,
      east: centre.lon + dLon,
    ));
    if (!mounted) return;
    final data = MapData.instance;
    if (data.state == MapDataState.error && data.messageKey != null) {
      messenger.showSnackBar(SnackBar(
        content: Text(
            tf(context, data.messageKey!, data.messageArgs ?? const {})),
      ));
    } else if (data.isReady) {
      // Roads alone leave a grey map the moment the phone loses signal, so the
      // picture of the same area comes down with them, sharpest first.
      unawaited(_saveAreaTiles(centre, radiusM));
      messenger.showSnackBar(SnackBar(
        content: Text(tf(context, 'mapdata.status.ready', <String, Object?>{
          'nodes': data.graph?.nodeCount ?? 0,
          'edges': data.graph?.edgeCount ?? 0,
          'mb': ((data.graphFile?.existsSync() ?? false)
                  ? data.graphFile!.lengthSync() / (1 << 20)
                  : 0)
              .toStringAsFixed(1),
        })),
      ));
    }
  }

  Widget _controls(BuildContext context) {
    final hasRoute = _controller.hasRoute;
    return Column(
      mainAxisSize: MainAxisSize.min,
      children: <Widget>[
        FloatingActionButton.small(
          heroTag: 'nav-follow',
          tooltip: t(context, 'nav.follow'),
          backgroundColor: _controller.follow
              ? Theme.of(context).colorScheme.primaryContainer
              : null,
          onPressed: _toggleFollow,
          child: Icon(_controller.gpsActive
              ? Icons.my_location
              : Icons.location_searching),
        ),
        const SizedBox(height: 8),
        FloatingActionButton.small(
          heroTag: 'nav-pick',
          tooltip: t(context, 'nav.route.mode'),
          backgroundColor: _controller.tapMode != TapMode.none
              ? Theme.of(context).colorScheme.primaryContainer
              : null,
          onPressed: _controller.toggleTapMode,
          child: const Icon(Icons.add_location_alt_outlined),
        ),
        if (hasRoute) ...<Widget>[
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
          const SizedBox(height: 8),
          FloatingActionButton.small(
            heroTag: 'nav-reset',
            tooltip: t(context, 'nav.route.reset'),
            onPressed: () => _controller.reset(keepFixes: true),
            child: const Icon(Icons.close),
          ),
        ],
      ],
    );
  }

  Future<void> _toggleFollow() async {
    if (_controller.gpsActive) {
      _controller.setFollow(!_controller.follow);
      return;
    }
    final status = await _location.ensurePermission();
    if (!mounted) return;
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
      return;
    }
    _controller.attachFixes(_location.fixes());
    _controller.setFollow(true);
    if (mounted) {
      ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(content: Text(t(context, 'nav.gps.waiting'))));
    }
  }

  Future<void> _openSearch() async {
    final index = MapData.instance.index;
    if (index == null) {
      ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(content: Text(t(context, 'nav.search.noindex'))));
      return;
    }
    final hit = await showSearch<SearchHit?>(
        context: context, delegate: _PlaceSearchDelegate(index, context));
    if (hit == null || !mounted) return;
    _map.move(LatLng(hit.position.lat, hit.position.lon), 16);
    await _offerDestination(hit);
  }

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
  Future<void> _saveOffline() async {
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

    // Nearest first, so a capped download still covers the ground underfoot.
    if (here != null) {
      add(tilesAround(here,
          radiusM: kSharpRadiusM,
          zooms: const <int>[kSharpZoom],
          maxTiles: 1 << 20));
    }
    if (finish != null) {
      add(tilesAround(finish,
          radiusM: kSharpRadiusM,
          zooms: const <int>[kSharpZoom],
          maxTiles: 1 << 20));
    }
    if (route != null) {
      add(corridorTiles(route.points,
          zooms: const <int>[kSharpZoom], maxTiles: 1 << 20));
    }
    // Context last: it is the part worth dropping when the cap bites.
    for (final centre in <LatLon?>[here, finish]) {
      if (centre == null) continue;
      add(tilesAround(centre,
          radiusM: kContextRadiusM,
          zooms: const <int>[kContextZoom],
          maxTiles: 1 << 20));
    }
    if (route != null) {
      add(corridorTiles(route.points,
          zooms: const <int>[kContextZoom], maxTiles: 1 << 20));
    }
    if (wanted.isEmpty) {
      ScaffoldMessenger.of(context).showSnackBar(SnackBar(
          content: Text(t(context, 'mapdata.corridor.needRoute'))));
      return;
    }

    final cap = NavSettings.instance.corridorMaxTiles;
    final tiles = wanted.length <= cap ? wanted : wanted.sublist(0, cap);
    setState(() {
      _corridorRunning = true;
      _cancelCorridor = false;
    });
    final messenger = ScaffoldMessenger.of(context);
    final report = await cache.downloadCorridor(
      tiles,
      _tileUrls,
      onProgress: (done, total) {
        _refreshTilesEvery(done, total);
        if (!mounted || done % 25 != 0) return;
        messenger.showSnackBar(SnackBar(
          duration: const Duration(seconds: 2),
          content: Text(tf(context, 'mapdata.corridor.progress',
              <String, Object?>{'done': done, 'total': total})),
        ));
      },
      cancelled: () => _cancelCorridor,
    );
    await cache.evictToCap(NavSettings.instance.tileCapMb << 20);
    if (!mounted) return;
    setState(() => _corridorRunning = false);
    _refreshTiles();
    messenger.showSnackBar(SnackBar(
      content: Text(report.blocked
          ? t(context, 'mapdata.corridor.blocked')
          : tf(context, 'mapdata.corridor.done', <String, Object?>{
              'n': report.downloaded + report.skipped,
            })),
    ));
  }

  /// Saves the picture of an area at every scale the map can show.
  ///
  /// Offline has to survive zooming out to find your bearings as much as
  /// zooming in to read a house number, so this is a pyramid: coarse levels
  /// first — nearly free, and what keeps the map legible if the tile budget
  /// cuts the download short — then finer ones, outwards from the middle.
  Future<void> _saveAreaTiles(LatLon centre, double radiusM) async {
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
      _tilesClock = Stopwatch()..start();
    });
    final messenger = ScaffoldMessenger.of(context);
    final report = await cache.downloadCorridor(tiles, _tileUrls,
        onProgress: (done, total) {
          _refreshTilesEvery(done, total);
          if (mounted) {
            setState(() {
              _tilesDone = done;
              _tilesBytes = cache.bytesFetched;
            });
          }
        },
        cancelled: () => _cancelCorridor || !mounted);
    await cache.evictToCap(NavSettings.instance.tileCapMb << 20);
    if (!mounted) return;
    setState(() => _corridorRunning = false);
    _refreshTiles();
    messenger.showSnackBar(SnackBar(
      content: Text(report.blocked
          ? t(context, 'mapdata.corridor.blocked')
          : tf(context, 'mapdata.corridor.done',
              <String, Object?>{'n': report.downloaded + report.skipped})),
    ));
  }

  /// Asks the layer to re-read its tiles; cached ones then paint immediately.
  void _refreshTiles() {
    if (mounted && !_tileReset.isClosed) _tileReset.add(null);
  }

  /// Same, but only every so often during a long download, so the map fills in
  /// as it goes without a rebuild per tile.
  void _refreshTilesEvery(int done, int total) {
    if (done % 10 == 0 || done == total) _refreshTiles();
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

  final SearchIndex index;

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
    if (query.trim().isEmpty) return const SizedBox.shrink();
    final hits = index.search(query);
    if (hits.isEmpty) {
      return Center(child: Text(t(context, 'nav.search.empty')));
    }
    return ListView.builder(
      itemCount: hits.length,
      itemBuilder: (ctx, i) => ListTile(
        leading: const Icon(Icons.place_outlined),
        title: Text(hits[i].display),
        subtitle: Text(hits[i].kind),
        onTap: () => close(context, hits[i]),
      ),
    );
  }
}
