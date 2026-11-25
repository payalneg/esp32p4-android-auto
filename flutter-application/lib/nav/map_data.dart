/// Owns the downloaded map data: the routing graph, the search index and the
/// tile cache.
///
/// There are two ways data gets here. Either the phone downloads a regional
/// extract from Geofabrik and builds the graph itself (pbf_graph.dart +
/// graph_builder.dart), or the user supplies files built on a desktop by
/// scripts/mapgen. Both produce the same RGF2 format, and both are the same
/// input pyosmium reads — an .osm.pbf.
///
/// Files land in the app's support directory — not the cache directory, which
/// the OS is free to purge — and are parsed in an isolate, because a 17 MB
/// graph takes long enough to drop frames.
///
/// Messages surface as i18n keys plus arguments rather than English text, the
/// same convention as firmware/firmware_updater.dart, so the UI localises them.
library;

import 'dart:async';
import 'dart:convert';
import 'dart:io';
import 'dart:math' as math;

import 'package:flutter/foundation.dart';
import 'package:package_info_plus/package_info_plus.dart';
import 'package:path_provider/path_provider.dart';

import 'geofabrik.dart';
import 'geo.dart';
import 'krakow_tiles.dart';
import 'pbf_graph.dart';
import 'rgf2.dart';
import 'router.dart';
import 'search_index.dart';
import 'tile_cache.dart';
import 'way_classes.dart';
import '../log/app_log.dart';
import '../settings/nav_settings.dart';

enum MapDataState { absent, downloading, loading, ready, error }

/// One map kept on disk so the rider can switch back without downloading it
/// again. The active map is a copy; these are the originals.
class ArchivedRegion {
  const ArchivedRegion({
    required this.key,
    required this.id,
    required this.name,
    required this.bytes,
    required this.builtAt,
    this.bounds,
  });

  /// Directory name under `nav/regions`.
  final String key;

  /// Catalogue id, or null for an imported desktop build.
  final String? id;
  final String name;
  final int bytes;
  final DateTime builtAt;

  /// The region's envelope, for coverage and for deciding which saved maps a
  /// route might need. Null when the sidecar is missing and the graph could
  /// not be read.
  final LatLonBounds? bounds;

  bool get isImported => id == null || id!.isEmpty;
}

class MapData extends ChangeNotifier {
  MapData._();
  static final MapData instance = MapData._();

  /// A ready instance around an in-memory graph, for controller tests that
  /// need a router without files or isolates.
  @visibleForTesting
  MapData.forTest(RouteGraph graph,
      {String? regionId, String? regionName, Directory? navDir})
      : _graph = graph,
        _regionId = regionId,
        _regionName = regionName,
        _dir = navDir,
        _router = Router(graph),
        _checkedDisk = true,
        _state = MapDataState.ready;

  static const graphFileName = 'route_graph.bin';
  static const indexFileName = 'search_index.tsv';
  static const sourceFileName = 'graph_source.json';
  static const catalogueFileName = 'geofabrik-index.json';
  static const geometryCatalogueFileName = 'geofabrik-geometry.json';
  static const regionsDirName = 'regions';
  static const activePointerName = 'active_region';
  static const boundsFileName = 'bounds.json';

  /// Folder name used for a graph that did not come from the catalogue: a
  /// desktop build imported through "add the files yourself", or a graph from
  /// before saved maps existed.
  static const importedKey = 'imported';

  /// A catalogue id made safe for a directory name. Ids are path-like
  /// (`us/alabama`), which is not a legal single path component on every
  /// platform.
  static String regionKey(String id) =>
      id.replaceAll(RegExp(r'[^A-Za-z0-9._-]'), '_');

  Directory? _dir;
  MapDataState _state = MapDataState.absent;
  bool _checkedDisk = false;
  double _progress = 0;
  String? _progressFile;
  String? _stageKey;
  double _stageFraction = 0;
  int _stageCount = 0;
  String? _messageKey;
  Map<String, String>? _messageArgs;

  RouteGraph? _graph;
  SearchIndex? _index;
  Router? _router;
  TileCache? _tiles;
  KrakowTileSource? _krakow;

  /// Whether the visible map is drawn from [_krakow] rather than raster OSM
  /// tiles. Read from [NavSettings] on start, toggled from the map screen.
  bool _useGraphTiles = NavSettings.defaultUseGraphTiles;
  String _userAgent = 'AaBridgeNavigator/dev';

  /// The catalogue region the current graph was built from, when it came from
  /// Geofabrik rather than a desktop build. Remembered so the app does not
  /// keep offering the download it is already running on.
  String? _regionId;
  String? _regionName;

  /// Which saved map the active graph came from, when it is one of them.
  String? _activeKey;
  GeofabrikIndex? _geometryCatalogue;

  /// Bounds of every map on the phone, keyed like the saved maps. Coverage is
  /// the union of them; only the active one is checked against its roads,
  /// because the others are not in memory.
  final Map<String, LatLonBounds> _coverage = <String, LatLonBounds>{};

  /// The merged graph built on demand for a route that leaves the active map.
  RouteGraph? _worldGraph;
  Router? _worldRouter;
  String? _worldSignature;

  int _indexGeneration = 0;
  String? _indexSignature;
  bool _mergedIndexActive = false;

  MapDataState get state => _state;

  /// Whether the first look at the disk has finished. Until it has, "no map"
  /// and "not read yet" are the same state, and offering a download for a
  /// region the phone is about to load anyway would be wrong.
  bool get settled => _checkedDisk;
  double get progress => _progress;
  String? get progressFile => _progressFile;

  /// The stage a build is on, as an i18n key, and how far it has come. Null
  /// while nothing is being built.
  String? get stageKey => _stageKey;
  double get stageFraction => _stageFraction;
  int get stageCount => _stageCount;
  String? get messageKey => _messageKey;
  Map<String, String>? get messageArgs => _messageArgs;
  RouteGraph? get graph => _graph;
  SearchIndex? get index => _index;
  Router? get router => _router;
  TileCache? get tiles => _tiles;
  String get userAgent => _userAgent;

  /// Whether this phone holds geometry the map can be drawn from offline.
  /// The switch is only offered when there is something to switch to.
  bool get hasGraphTiles => _krakow != null;

  /// Whether the map is currently drawn from that geometry instead of OSM
  /// raster tiles.
  bool get useGraphTiles => _useGraphTiles;

  /// Switches the map style, and repaints by replacing the cache.
  ///
  /// A new [TileCache] object is deliberate: flutter_map keys tile images by
  /// the cache instance, so reusing it would leave the previous style's
  /// renders on screen until the camera moved. Both styles live in the same
  /// root — graph tiles under `krakow-v2/` — so switching costs no download
  /// of the other one and never overwrites it.
  void setUseGraphTiles(bool value) {
    if (_useGraphTiles == value) return;
    _useGraphTiles = value;
    final cache = _tiles;
    if (cache != null) {
      _tiles = TileCache(cache.root,
          userAgent: cache.userAgent,
          krakow: _krakow,
          useGraphTiles: value);
    }
    notifyListeners();
  }

  /// The settings object is the single source of truth for the map style; the
  /// switch lives there so the service isolate can read it too. This keeps a
  /// toggle anywhere in the UI reflected in the cache the screen is drawing
  /// from.
  void _onNavSettings() {
    final wanted = NavSettings.instance.useGraphTiles;
    if (wanted != _useGraphTiles) setUseGraphTiles(wanted);
  }

  /// The region this graph was downloaded from, if any.
  String? get regionId => _regionId;
  String? get regionName => _regionName;
  bool get isReady => _state == MapDataState.ready && _router != null;

  /// Whether any map on the phone has roads where [p] is.
  ///
  /// The active graph is checked precisely: its envelope and a widened snap
  /// radius. Saved maps are only envelopes — they are not in memory — so a
  /// point inside one is taken to be covered. That is the right trade for an
  /// unsolicited offer: being quiet inside a saved region beats spending a
  /// graph load to tell a forest from a street.
  bool covers(LatLon p) {
    final world = _worldGraph;
    final worldBox = world?.bounds;
    if (world != null &&
        worldBox != null &&
        worldBox.contains(p, marginM: 300) &&
        world.nearestNode(p, maxRadiusM: 3000) != null) {
      return true;
    }
    final g = _graph;
    final box = g?.bounds;
    final active = _activeKey ?? importedKey;
    if (g != null &&
        box != null &&
        box.contains(p, marginM: 300) &&
        g.nearestNode(p, maxRadiusM: 3000) != null) {
      return true;
    }
    for (final entry in _coverage.entries) {
      if (entry.key == active) continue; // already checked against its roads
      if (entry.value.contains(p, marginM: 300)) return true;
    }
    return false;
  }

  /// Whether there is anything at all to route on: the active graph, or a
  /// saved map that a cross-border route could use.
  bool get hasAnyGraph => _router != null || _coverage.isNotEmpty;

  /// Routes between two points, merging saved maps when they are not both on
  /// the active one.
  ///
  /// The active graph is tried first: a town trip must not pay for a merge.
  /// A null answer is cheap there — the component check in the router returns
  /// before A* when the ends cannot be on the same network. Only then are the
  /// saved maps stitched together, off the UI isolate, and the route retried.
  Future<RouteResult?> route(
      LatLon start, LatLon goal, RideProfile profile) async {
    // Snapping reaches 500 m, so a route can "succeed" on the active graph by
    // ending at a border road when the real destination is in the next saved
    // region. The envelope, not the snap, decides whether a merge is needed.
    final graph = _graph;
    final box = graph?.bounds;
    final bothActive = graph != null &&
        box != null &&
        box.contains(start, marginM: 50) &&
        box.contains(goal, marginM: 50);
    if (bothActive) {
      final direct = await _router!.route(start, goal, profile);
      if (direct != null) return direct;
    }

    final world = await _mergedRouter(start, goal);
    if (world != null && !identical(world.graph, _graph)) {
      try {
        final merged = await world.route(start, goal, profile);
        if (merged != null) return merged;
      } on Object catch (e) {
        AppLog.w('nav', 'route over merged maps failed: $e');
      }
    }
    // No saved map to merge, or none that connects these two: the active
    // graph's own answer, even if it is a snap to a border road, is the best
    // there is.
    return _router?.route(start, goal, profile);
  }

  Future<Router?> _mergedRouter(LatLon start, LatLon goal) async {
    final sources = await _worldSources(start, goal);
    if (sources.isEmpty) return null;
    // One kept source that is the active map means there is nothing to merge.
    // One kept source that is a *saved* map is a different story: a saved
    // country may be what covers a route whose active map is only a province.
    if (sources.length == 1 &&
        sources.first.key == (_activeKey ?? 'active')) {
      return null;
    }
    final signature = <String>[
      for (final s in sources) '${s.key}:${s.bytes}:${s.modified}',
    ].join('|');
    final cached = _worldRouter;
    if (cached != null && _worldSignature == signature) return cached;

    try {
      final graph = await RouteGraph.mergeFiles(
          <String>[for (final s in sources) s.path]);
      _worldGraph = graph;
      _worldRouter = Router(graph);
      _worldSignature = signature;
      AppLog.i('nav',
          'routing over ${sources.length} maps: ${graph.nodeCount} nodes, '
          '${graph.edgeCount} edges');
      return _worldRouter;
    } on Object catch (e) {
      AppLog.e('nav', 'could not merge saved maps: $e');
      return null;
    }
  }

  /// The active graph plus the saved maps whose envelopes meet the start–goal
  /// box. Including a region nowhere near the route only costs memory.
  Future<List<({String key, String path, int bytes, int modified})>>
      _worldSources(LatLon start, LatLon goal) async {
    final candidates =
        <({String key, String path, int bytes, int modified, LatLonBounds? bounds})>[];
    final active = graphFile;
    if (active != null && await active.exists()) {
      final stat = await active.stat();
      candidates.add((
        key: _activeKey ?? 'active',
        path: active.path,
        bytes: stat.size,
        modified: stat.modified.millisecondsSinceEpoch,
        bounds: _graph?.bounds,
      ));
    }

    final root = _regionsRoot;
    if (root != null) {
      final box = _routeBox(start, goal);
      for (final saved in await archivedRegions()) {
        if (saved.key == _activeKey) continue;
        final bounds = saved.bounds;
        if (bounds != null && !_intersects(bounds, box)) continue;
        final path = '${root.path}/${saved.key}/$graphFileName';
        if (!await File(path).exists()) continue;
        candidates.add((
          key: saved.key,
          path: path,
          bytes: saved.bytes,
          modified: saved.builtAt.millisecondsSinceEpoch,
          bounds: bounds,
        ));
      }
    }

    // A country beside its own province duplicates every road inside it, and
    // reading the country's file just to throw the province away is what
    // costs memory. Drop the contained graph here; the merge has the same
    // rule as a backstop.
    final kept =
        <({String key, String path, int bytes, int modified, LatLonBounds? bounds})>[];
    for (var i = 0; i < candidates.length; i++) {
      final inner = candidates[i].bounds;
      var redundant = false;
      if (inner != null) {
        for (var j = 0; j < candidates.length; j++) {
          if (i == j) continue;
          final outer = candidates[j].bounds;
          if (outer == null) continue;
          final outerSpan = (outer.north - outer.south) + (outer.east - outer.west);
          final innerSpan = (inner.north - inner.south) + (inner.east - inner.west);
          if (outerSpan <= innerSpan * 1.02) continue;
          if (_containsBounds(outer, inner)) {
            redundant = true;
            break;
          }
        }
      }
      if (!redundant) kept.add(candidates[i]);
    }

    kept.sort((a, b) => a.key.compareTo(b.key));
    return <({String key, String path, int bytes, int modified})>[
      for (final source in kept)
        (
          key: source.key,
          path: source.path,
          bytes: source.bytes,
          modified: source.modified,
        ),
    ];
  }

  static bool _containsBounds(LatLonBounds outer, LatLonBounds inner) =>
      inner.south >= outer.south - 1e-9 &&
      inner.north <= outer.north + 1e-9 &&
      inner.west >= outer.west - 1e-9 &&
      inner.east <= outer.east + 1e-9;

  static LatLonBounds _routeBox(LatLon start, LatLon goal) {
    const margin = 0.05; // ~5.5 km
    return LatLonBounds(
      south: math.min(start.lat, goal.lat) - margin,
      north: math.max(start.lat, goal.lat) + margin,
      west: math.min(start.lon, goal.lon) - margin,
      east: math.max(start.lon, goal.lon) + margin,
    );
  }

  static bool _intersects(LatLonBounds a, LatLonBounds b) =>
      a.north >= b.south &&
      a.south <= b.north &&
      a.east >= b.west &&
      a.west <= b.east;

  void _invalidateWorld() {
    _worldGraph = null;
    _worldRouter = null;
    _worldSignature = null;
  }

  /// The region catalogue, cached on disk so a picker or a position lookup
  /// does not re-download it. [geometry] selects the variant with outlines,
  /// which is what `GeofabrikIndex.regionAt` needs.
  Future<GeofabrikIndex> regionCatalogue(
      {bool geometry = false, bool refresh = false}) async {
    await init();
    if (geometry && !refresh && _geometryCatalogue != null) {
      return _geometryCatalogue!;
    }
    final index = await GeofabrikIndex.fetch(
      userAgent: _userAgent,
      geometry: geometry,
      cache: geometry ? geometryCatalogueFile : catalogueFile,
      refresh: refresh,
    );
    if (geometry) _geometryCatalogue = index;
    return index;
  }

  /// The geometry catalogue if one has already been fetched, without going to
  /// the network for it. The region picker uses this for its "near you" row:
  /// three and a half megabytes is too much to spend on a hint.
  Future<GeofabrikIndex?> cachedGeometryCatalogue() async {
    await init();
    final f = geometryCatalogueFile;
    if (f == null || !await f.exists()) return null;
    try {
      return await regionCatalogue(geometry: true);
    } on Object {
      return null;
    }
  }

  File? get catalogueFile =>
      _dir == null ? null : File('${_dir!.path}/$catalogueFileName');
  File? get geometryCatalogueFile =>
      _dir == null ? null : File('${_dir!.path}/$geometryCatalogueFileName');
  File? get sourceFile =>
      _dir == null ? null : File('${_dir!.path}/$sourceFileName');

  File? get graphFile =>
      _dir == null ? null : File('${_dir!.path}/$graphFileName');
  File? get indexFile =>
      _dir == null ? null : File('${_dir!.path}/$indexFileName');

  Directory? get _regionsRoot =>
      _dir == null ? null : Directory('${_dir!.path}/$regionsDirName');
  File? get _activePointer =>
      _dir == null ? null : File('${_dir!.path}/$activePointerName');

  /// The saved map the active graph came from, if it is one of them.
  String? get activeKey => _activeKey;

  /// Resolves storage, builds the User-Agent and loads whatever is already
  /// downloaded. Safe to call without awaiting — the UI listens instead.
  Future<void> init() async {
    if (_dir != null) return;
    final support = await getApplicationSupportDirectory();
    _dir = Directory('${support.path}/nav');
    await _dir!.create(recursive: true);
    await _readActivePointer();
    try {
      final info = await PackageInfo.fromPlatform();
      _userAgent = 'AaBridgeNavigator/${info.version} '
          '(+https://github.com/payalneg/esp32p4-android-auto)';
    } on Object {
      // Not fatal: a generic-looking UA is worse than a versioned one, but
      // failing to start the navigator over it would be sillier.
    }
    final tileRoot = Directory('${support.path}/tiles/osm');
    _useGraphTiles = NavSettings.instance.useGraphTiles;
    NavSettings.instance.addListener(_onNavSettings);
    _krakow = await _readKrakowTiles();
    _tiles = TileCache(tileRoot,
        userAgent: _userAgent,
        krakow: _krakow,
        useGraphTiles: _useGraphTiles);
    await _loadFromDisk();
    // An install from before saved maps (or one whose files were replaced)
    // should still show its map in the list. The copy is skipped when the
    // archive is already there.
    try {
      await saveActiveMap();
    } on Object catch (e) {
      AppLog.w('map', 'could not keep the active map: $e');
    }
    unawaited(_refreshIndex());
  }

  /// Keeps the graph that is loaded now under `nav/regions`, so switching away
  /// from it later is a file copy rather than another download.
  ///
  /// [force] rewrites the copy even when the active graph is believed to be
  /// that same saved map — used right after rebuilding a region that was
  /// already saved, so the copy is not the previous version.
  Future<ArchivedRegion?> saveActiveMap({bool force = false}) async {
    await init();
    final graph = graphFile;
    final root = _regionsRoot;
    if (graph == null ||
        root == null ||
        _graph == null ||
        !await graph.exists()) {
      return null;
    }
    final id = _regionId;
    final key = id == null || id.isEmpty ? importedKey : regionKey(id);
    final savedGraph = File('${root.path}/$key/$graphFileName');
    final bounds = _graph?.bounds;
    if (!force && key == _activeKey && await savedGraph.exists()) {
      await _writeActivePointer(key);
      if (bounds != null) {
        _coverage[key] = bounds;
        await _writeBoundsFile(
            File('${root.path}/$key/$boundsFileName'), bounds);
      }
      final stat = await graph.stat();
      return ArchivedRegion(
        key: key,
        id: id,
        name: _regionName ?? '',
        bytes: stat.size,
        builtAt: stat.modified,
        bounds: bounds,
      );
    }

    final dir = Directory('${root.path}/$key');
    await dir.create(recursive: true);
    await _copyFile(graph, savedGraph);

    final idx = indexFile;
    final savedIndex = File('${dir.path}/$indexFileName');
    if (idx != null && await idx.exists()) {
      await _copyFile(idx, savedIndex);
    } else if (await savedIndex.exists()) {
      await savedIndex.delete();
    }
    final savedSource = File('${dir.path}/$sourceFileName');
    await _writeSourceFile(savedSource, id, _regionName);
    if (bounds != null) {
      await _writeBoundsFile(File('${dir.path}/$boundsFileName'), bounds);
      _coverage[key] = bounds;
    }
    _invalidateWorld();

    _activeKey = key;
    await _writeActivePointer(key);
    await _refreshIndex();

    final stat = await graph.stat();
    return ArchivedRegion(
      key: key,
      id: id,
      name: _regionName ?? '',
      bytes: stat.size,
      builtAt: stat.modified,
      bounds: bounds,
    );
  }

  /// Makes a saved map the active one. False when it is gone or unreadable.
  ///
  /// The map being replaced is saved first, so switching is always reversible
  /// even for a graph that was imported from the desktop and has no catalogue
  /// entry of its own.
  Future<bool> activateRegion(String key) async {
    await init();
    // A build in progress is writing the active files; switching under it
    // would be undone the moment the build finishes.
    if (_state == MapDataState.downloading || _state == MapDataState.loading) {
      return false;
    }
    final root = _regionsRoot;
    if (root == null) return false;
    final dir = Directory('${root.path}/$key');
    final savedGraph = File('${dir.path}/$graphFileName');
    if (!await savedGraph.exists()) return false;
    if (key == _activeKey && _state == MapDataState.ready) return true;

    await saveActiveMap();
    _set(MapDataState.loading);
    try {
      await _copyFile(savedGraph, graphFile!);

      final savedIndex = File('${dir.path}/$indexFileName');
      if (await savedIndex.exists()) {
        await _copyFile(savedIndex, indexFile!);
      } else if (await indexFile!.exists()) {
        await indexFile!.delete();
      }

      final savedSource = File('${dir.path}/$sourceFileName');
      if (await savedSource.exists()) {
        await _copyFile(savedSource, sourceFile!);
      } else if (await sourceFile!.exists()) {
        await sourceFile!.delete();
      }

      _activeKey = key;
      await _writeActivePointer(key);
      await _loadFromDisk();
      if (_state == MapDataState.ready) {
        await _refreshIndex();
      }
      return _state == MapDataState.ready;
    } on Object catch (e) {
      _fail('mapdata.err.load', <String, String>{'err': '$e'});
      return false;
    }
  }

  /// Every map kept for switching back to, newest first.
  Future<List<ArchivedRegion>> archivedRegions() async {
    await init();
    final root = _regionsRoot;
    if (root == null || !await root.exists()) {
      return const <ArchivedRegion>[];
    }
    final out = <ArchivedRegion>[];
    await for (final entry in root.list(followLinks: false)) {
      if (entry is! Directory) continue;
      final key = entry.path
          .replaceAll('\\', '/')
          .split('/')
          .where((s) => s.isNotEmpty)
          .last;
      final graph = File('${entry.path}/$graphFileName');
      if (!await graph.exists()) continue;
      String? id;
      var name = '';
      final src = File('${entry.path}/$sourceFileName');
      if (await src.exists()) {
        try {
          final decoded = jsonDecode(await src.readAsString());
          if (decoded is Map) {
            final rawId = decoded['id'];
            id = rawId is String && rawId.isNotEmpty ? rawId : null;
            final rawName = decoded['name'];
            if (rawName is String) name = rawName;
          }
        } on Object {
          // A broken source file still describes a usable graph.
        }
      }
      final stat = await graph.stat();
      final boundsFile = File('${entry.path}/$boundsFileName');
      var bounds = await _readBoundsFile(boundsFile);
      if (bounds == null) {
        bounds = RouteGraph.readBounds(graph.path);
        if (bounds != null) await _writeBoundsFile(boundsFile, bounds);
      }
      out.add(ArchivedRegion(
        key: key,
        id: id,
        name: name,
        bytes: stat.size,
        builtAt: stat.modified,
        bounds: bounds,
      ));
    }
    out.sort((a, b) => b.builtAt.compareTo(a.builtAt));
    return out;
  }

  /// Whether [id]'s region is already kept on disk.
  Future<bool> hasArchivedRegion(String id) async {
    final root = _regionsRoot;
    if (root == null || id.isEmpty) return false;
    return File('${root.path}/${regionKey(id)}/$graphFileName').exists();
  }

  /// Drops a saved map. The active one is left alone: deleting the map being
  /// used is "Delete map data", with its own confirm.
  Future<void> deleteArchived(String key) async {
    await init();
    if (key == _activeKey) return;
    final root = _regionsRoot;
    if (root == null) return;
    final dir = Directory('${root.path}/$key');
    if (await dir.exists()) await dir.delete(recursive: true);
    _coverage.remove(key);
    _invalidateWorld();
    await _refreshIndex();
  }

  /// Rebuilds the in-memory coverage from the active graph and the saved
  /// sidecars. Called once at startup and after a saved map is added.
  Future<void> _loadCoverage() async {
    _coverage.clear();
    final graph = _graph;
    final active = _activeKey ?? importedKey;
    if (graph?.bounds != null) _coverage[active] = graph!.bounds!;
    for (final saved in await archivedRegions()) {
      if (saved.bounds != null) _coverage[saved.key] = saved.bounds!;
    }
  }

  /// Keeps [index] covering every saved map, so a place in the next province
  /// is found without switching maps first.
  ///
  /// The merge runs in an isolate and replaces the active index when it is
  /// ready; until then search works over the region in use. A generation
  /// counter drops a merge that a later switch made stale.
  Future<void> _refreshIndex() async {
    final generation = ++_indexGeneration;
    final sources = <({String key, String path, int size, int mtime})>[];

    final activeIndex = indexFile;
    if (activeIndex != null && await activeIndex.exists()) {
      final stat = await activeIndex.stat();
      sources.add((
        key: _activeKey ?? 'active',
        path: activeIndex.path,
        size: stat.size,
        mtime: stat.modified.millisecondsSinceEpoch,
      ));
    }
    final root = _regionsRoot;
    if (root != null) {
      for (final saved in await archivedRegions()) {
        if (saved.key == _activeKey) continue;
        final file = File('${root.path}/${saved.key}/$indexFileName');
        if (!await file.exists()) continue;
        final stat = await file.stat();
        sources.add((
          key: saved.key,
          path: file.path,
          size: stat.size,
          mtime: stat.modified.millisecondsSinceEpoch,
        ));
      }
    }
    sources.sort((a, b) => a.key.compareTo(b.key));
    final signature = <String>[
      for (final s in sources) '${s.key}:${s.path}:${s.size}:${s.mtime}',
    ].join('|');
    if (generation != _indexGeneration) return;
    if (signature == _indexSignature) return;

    if (sources.length < 2) {
      final wasMerged = _mergedIndexActive;
      _indexSignature = signature;
      _mergedIndexActive = false;
      final only = sources.isEmpty ? null : sources.first;
      if (only == null) {
        if (wasMerged) {
          _index = null;
          notifyListeners();
        }
        return;
      }
      if (wasMerged || only.key != (_activeKey ?? 'active')) {
        final single = await SearchIndex.load(only.path);
        if (generation != _indexGeneration) return;
        _index = single;
        notifyListeners();
      }
      return;
    }

    final merged = await SearchIndex.loadMerged(
        <String>[for (final s in sources) s.path]);
    if (generation != _indexGeneration) return;
    _index = merged;
    _mergedIndexActive = true;
    _indexSignature = signature;
    notifyListeners();
  }

  Future<void> _readActivePointer() async {
    final f = _activePointer;
    _activeKey = null;
    if (f == null || !await f.exists()) return;
    try {
      final key = (await f.readAsString()).trim();
      if (key.isNotEmpty) _activeKey = key;
    } on Object {
      // A lost pointer only means the next save re-registers the map.
    }
  }

  Future<void> _writeActivePointer(String key) async {
    final f = _activePointer;
    if (f == null) return;
    try {
      await f.writeAsString(key, flush: true);
    } on FileSystemException catch (e) {
      AppLog.w('map', 'could not record the active map: $e');
    }
  }

  static Future<void> _copyFile(File src, File dest) async {
    await dest.parent.create(recursive: true);
    final part = File('${dest.path}.part');
    await src.copy(part.path);
    if (await dest.exists()) await dest.delete();
    await part.rename(dest.path);
  }

  Future<void> _writeSourceFile(File f, String? id, String? name) async {
    try {
      final part = File('${f.path}.part');
      await part.writeAsString(
          jsonEncode(<String, String>{
            'id': id ?? '',
            'name': name ?? '',
          }),
          flush: true);
      if (await f.exists()) await f.delete();
      await part.rename(f.path);
    } on FileSystemException catch (e) {
      AppLog.w('map', 'could not record a map source: $e');
    }
  }

  Future<void> _writeBoundsFile(File f, LatLonBounds b) async {
    try {
      final part = File('${f.path}.part');
      await part.writeAsString(
          jsonEncode(<String, double>{
            'south': b.south,
            'west': b.west,
            'north': b.north,
            'east': b.east,
          }),
          flush: true);
      if (await f.exists()) await f.delete();
      await part.rename(f.path);
    } on FileSystemException catch (e) {
      AppLog.w('map', 'could not record a map envelope: $e');
    }
  }

  Future<LatLonBounds?> _readBoundsFile(File f) async {
    if (!await f.exists()) return null;
    try {
      final decoded = jsonDecode(await f.readAsString());
      if (decoded is Map) {
        final south = (decoded['south'] as num?)?.toDouble();
        final west = (decoded['west'] as num?)?.toDouble();
        final north = (decoded['north'] as num?)?.toDouble();
        final east = (decoded['east'] as num?)?.toDouble();
        if (south != null && west != null && north != null && east != null) {
          return LatLonBounds(
              south: south, west: west, north: north, east: east);
        }
      }
    } on Object {
      // A broken sidecar is repaired from the graph by the caller.
    }
    return null;
  }

  /// Reloads the active files from disk. Test-only entry point for a
  /// directory-backed instance.
  @visibleForTesting
  Future<void> reloadFromDisk() => _loadFromDisk();

  File get _krakowSelection => File('${_dir!.path}/krakow-v1/current');

  Future<KrakowTileSource?> _readKrakowTiles() async {
    final selection = _krakowSelection;
    if (!await selection.exists()) return null;
    final version = (await selection.readAsString()).trim();
    if (!RegExp(r'^\d+$').hasMatch(version)) return null;
    final path = '${selection.parent.path}/$version';
    return await Directory(path).exists()
        ? KrakowTileSource(path, version)
        : null;
  }

  Future<void> _useKrakowTiles(KrakowTileSource? source) async {
    final cache = _tiles!;
    final selection = _krakowSelection;
    if (source == null) {
      if (await selection.exists()) await selection.delete();
    } else {
      final staged = File('${selection.path}.part');
      await staged.writeAsString(source.version, flush: true);
      await staged.rename(selection.path);
    }
    _tiles = TileCache(cache.root,
        userAgent: _userAgent,
        krakow: source,
        useGraphTiles: _useGraphTiles);
    _krakow = source;
    final previous = cache.krakow;
    if (previous != null && previous.version != source?.version) {
      await _removeKrakowGeometry(previous);
    }
  }

  Future<void> _removeKrakowGeometry(KrakowTileSource source) async {
    try {
      final dir = Directory(source.path);
      if (await dir.exists()) await dir.delete(recursive: true);
    } on FileSystemException catch (e) {
      AppLog.e('map', 'could not remove old Krakow geometry: $e');
    }
  }

  Future<void> _loadFromDisk() async {
    // Anything derived from the previous graph is stale the moment a new one
    // is picked up.
    _indexGeneration++;
    _indexSignature = null;
    _mergedIndexActive = false;
    _invalidateWorld();
    final g = graphFile;
    if (g == null || !await g.exists()) {
      _regionId = null;
      _regionName = null;
      await _loadCoverage();
      _checkedDisk = true;
      _set(MapDataState.absent);
      return;
    }
    _set(MapDataState.loading);
    try {
      _graph = await RouteGraph.load(g.path);
      _router = Router(_graph!);
      final idx = indexFile;
      _index = (idx != null && await idx.exists())
          ? await SearchIndex.load(idx.path)
          : null;
      await _readSource();
      // The pointer names the saved map the active graph came from. A source
      // that disagrees means the files were replaced outside the library
      // (a test, a manual copy); trust the graph in front of us.
      if (_regionId != null) {
        final active = regionKey(_regionId!);
        if (active != _activeKey) {
          _activeKey = active;
          await _writeActivePointer(active);
        }
      }
      await _loadCoverage();
      _set(MapDataState.ready);
    } on FormatException catch (e) {
      _graph = null;
      _router = null;
      await _loadCoverage();
      _fail('mapdata.err.badGraph', <String, String>{'err': e.message});
    } on Object catch (e) {
      _graph = null;
      _router = null;
      await _loadCoverage();
      _fail('mapdata.err.load', <String, String>{'err': '$e'});
    } finally {
      _checkedDisk = true;
    }
  }

  Future<void> _readSource() async {
    _regionId = null;
    _regionName = null;
    final f = sourceFile;
    if (f == null || !await f.exists()) return;
    try {
      final decoded = jsonDecode(await f.readAsString());
      if (decoded is Map) {
        final id = decoded['id'];
        _regionId = id is String && id.isNotEmpty ? id : null;
        final name = decoded['name'];
        _regionName = name is String && name.isNotEmpty ? name : null;
      }
    } on Object catch (e) {
      AppLog.w('map', 'unreadable graph source: $e');
    }
  }

  Future<void> _writeSource(GeofabrikRegion region) async {
    final f = sourceFile;
    _regionId = region.id;
    _regionName = region.name;
    if (f == null) return;
    await _writeSourceFile(f, region.id, region.name);
  }

  /// Downloads a Geofabrik extract and builds the graph from it — the same
  /// route scripts/mapgen takes.
  ///
  /// A whole province is one 190 MB file, which is what makes routing over
  /// more than a neighbourhood practical at all.
  Future<void> buildFromRegion(GeofabrikRegion region) async {
    await init();
    // Keep what is on the phone now; the new region replaces it only as the
    // active map, not as the only map.
    await saveActiveMap();
    _messageKey = null;
    _messageArgs = null;
    _set(MapDataState.downloading);
    AppLog.i('map', 'region ${region.name}: downloading ${region.pbfUrl}');
    final pbf = File('${_dir!.path}/region.osm.pbf');
    final client = HttpClient()..userAgent = _userAgent;
    final version = '${DateTime.now().microsecondsSinceEpoch}';
    final krakow = region.pbfUrl.path.contains('/malopolskie-')
        ? KrakowTileSource('${_krakowSelection.parent.path}/$version', version)
        : null;
    try {
      final clock = Stopwatch()..start();
      await downloadTo(client, region.pbfUrl, pbf, (received, total) {
        final mb = received / (1 << 20);
        final seconds = clock.elapsedMilliseconds / 1000;
        final rate = seconds > 0 ? mb / seconds : 0;
        _progress = total > 0 ? received / total : 0;
        _progressFile = '${mb.toStringAsFixed(0)}'
            '${total > 0 ? " / ${(total / (1 << 20)).toStringAsFixed(0)}" : ""}'
            ' MB · ${rate.toStringAsFixed(1)} MB/s';
        notifyListeners();
      });

      _set(MapDataState.loading);
      // Parsing a province is tens of seconds and hundreds of megabytes at
      // its peak; an isolate keeps both off the UI and frees them on exit.
      //
      // Started from a static method on purpose. Dart allocates one closure
      // context per function, and this one also holds the progress callback
      // above, which touches `this` — so a closure written here would carry
      // MapData, its listeners and the whole widget tree across the isolate
      // boundary, which is not allowed. A static entry point has a context of
      // its own, holding nothing but the path.
      final built = await PbfGraphSource.buildInIsolate(pbf.path,
          krakowTilePath: krakow?.path,
          onProgress: (p) {
        _stageKey = p.stageKey;
        _stageFraction = p.fraction;
        _stageCount = p.count;
        notifyListeners();
      });
      AppLog.i(
          'map',
          'built ${built.nodeCount} nodes, ${built.edgeCount} edges, '
              '${built.placeCount} places');
      if (built.edgeCount == 0) {
        _fail('mapdata.err.emptyArea', null);
        return;
      }
      await graphFile!.writeAsBytes(built.graphBytes, flush: true);
      final index = indexFile!;
      if (built.searchIndexTsv.isEmpty) {
        if (await index.exists()) await index.delete();
      } else {
        await index.writeAsString(built.searchIndexTsv, flush: true);
      }
      if (krakow != null) await _useKrakowTiles(krakow);
      await _writeSource(region);
      await _loadFromDisk();
    } on Object catch (e) {
      AppLog.e('map', 'region build failed: $e');
      _fail('mapdata.err.download', <String, String>{'err': '$e'});
    } finally {
      _stageKey = null;
      client.close(force: true);
      if (krakow != null && _tiles?.krakow != krakow) {
        await _removeKrakowGeometry(krakow);
      }
      // The extract is only an input; keeping it would double the footprint.
      if (await pbf.exists()) await pbf.delete();
    }
    // A graph that never became ready has nothing worth keeping; one that did
    // becomes a saved map straight away, so the next border crossing can
    // switch back to it for free.
    if (isReady) {
      try {
        await saveActiveMap(force: true);
      } on Object catch (e) {
        AppLog.w('map', 'could not keep the built region: $e');
      }
    }
  }

  /// Fetches [url] into [dest], resuming a part file left by an earlier try.
  ///
  /// A province is 190 MB over whatever connection a phone has at the time,
  /// and losing all of it to one dropped connection is what made people give
  /// up on the download. The bytes already on disk are asked for with a Range
  /// header; If-Range carries the validator the server gave us last time, so
  /// an extract rebuilt overnight answers 200 with the whole file instead of
  /// 206 with a tail that would not match what we have.
  @visibleForTesting
  Future<void> downloadTo(HttpClient client, Uri url, File dest,
      void Function(int received, int total) onProgress) async {
    final part = File('${dest.path}.part');
    final tag = File('${dest.path}.part.tag');
    var have = await part.exists() ? await part.length() : 0;
    final validator = have > 0 && await tag.exists()
        ? (await tag.readAsString()).trim()
        : '';
    if (have > 0 && validator.isEmpty) {
      // Nothing to check the tail against: start over rather than splice two
      // different files together.
      await part.delete();
      have = 0;
    }

    var target = url;
    HttpClientResponse resp;
    for (var hop = 0;; hop++) {
      final req = await client.getUrl(target);
      if (have > 0) {
        req.headers.set(HttpHeaders.rangeHeader, 'bytes=$have-');
        req.headers.set(HttpHeaders.ifRangeHeader, validator);
      }
      resp = await req.close();
      final location = resp.headers.value(HttpHeaders.locationHeader);
      if (resp.statusCode >= 300 && resp.statusCode < 400 && location != null) {
        await resp.drain<void>();
        if (hop >= 5) throw HttpException('too many redirects', uri: url);
        target = target.resolve(location);
        continue;
      }
      break;
    }
    if (resp.statusCode != 200 && resp.statusCode != 206) {
      await resp.drain<void>();
      throw HttpException('HTTP ${resp.statusCode}', uri: target);
    }

    // 200 to a ranged request means the server would not resume — the file
    // moved on, or it does not do ranges at all. Either way what is on disk
    // is no use.
    final resuming = resp.statusCode == 206;
    if (!resuming) have = 0;

    final newTag = resp.headers.value(HttpHeaders.etagHeader) ??
        resp.headers.value(HttpHeaders.lastModifiedHeader) ??
        '';
    if (!resuming) {
      if (newTag.isEmpty) {
        if (await tag.exists()) await tag.delete();
      } else {
        await tag.writeAsString(newTag, flush: true);
      }
    }

    final total = resp.contentLength < 0 ? -1 : resp.contentLength + have;
    final sink = part.openWrite(
        mode: resuming ? FileMode.writeOnlyAppend : FileMode.writeOnly);
    var received = have;
    var lastReport = received;
    try {
      await for (final chunk in resp) {
        sink.add(chunk);
        received += chunk.length;
        if (received - lastReport > 1 << 20) {
          lastReport = received;
          onProgress(received, total);
        }
      }
      await sink.flush();
    } finally {
      await sink.close();
    }
    if (await dest.exists()) await dest.delete();
    await part.rename(dest.path);
    if (await tag.exists()) await tag.delete();
  }

  /// Copies a file the user picked into place and loads it.
  Future<void> importGraph(String srcPath) => _import(srcPath, graphFile!);
  Future<void> importIndex(String srcPath) => _import(srcPath, indexFile!);

  Future<void> _import(String srcPath, File dest) async {
    await init();
    final graphImport = dest.path == graphFile!.path;
    if (graphImport) await saveActiveMap();
    _set(MapDataState.loading);
    try {
      await File(srcPath).copy(dest.path);
      // Imported files say nothing about where they came from, so an old
      // region id must not survive to suppress a suggestion later.
      if (graphImport) {
        final source = sourceFile;
        if (source != null && await source.exists()) await source.delete();
        _regionId = null;
        _regionName = null;
      }
      await _loadFromDisk();
      if (graphImport) {
        // An imported graph is a map too; keep it so a region download does
        // not silently destroy the file that was just picked.
        await saveActiveMap(force: true);
      }
    } on Object catch (e) {
      _fail('mapdata.err.import', <String, String>{'err': '$e'});
    }
  }

  Future<void> deleteAll() async {
    await init();
    if (_tiles?.krakow != null) await _useKrakowTiles(null);
    for (final f in <File?>[graphFile, indexFile, sourceFile]) {
      if (f != null && await f.exists()) await f.delete();
    }
    final root = _regionsRoot;
    if (root != null && await root.exists()) {
      await root.delete(recursive: true);
    }
    final pointer = _activePointer;
    if (pointer != null && await pointer.exists()) await pointer.delete();
    _activeKey = null;
    _graph = null;
    _index = null;
    _router = null;
    _regionId = null;
    _regionName = null;
    _coverage.clear();
    _invalidateWorld();
    _indexGeneration++;
    _indexSignature = null;
    _mergedIndexActive = false;
    _set(MapDataState.absent);
  }

  void _set(MapDataState s) {
    _state = s;
    if (s != MapDataState.error) {
      _messageKey = null;
      _messageArgs = null;
    }
    if (s != MapDataState.downloading) {
      _progress = 0;
      _progressFile = null;
    }
    notifyListeners();
  }

  void _fail(String key, Map<String, String>? args) {
    _messageKey = key;
    _messageArgs = args;
    _state = MapDataState.error;
    _progress = 0;
    _progressFile = null;
    notifyListeners();
  }
}
