/// Owns the downloaded map data: the routing graph, the search index and the
/// tile cache.
///
/// There are two ways data gets here. Either the phone downloads raw OSM data
/// for an area from Overpass and builds the graph itself (overpass.dart +
/// graph_builder.dart), or the user supplies files built on a desktop by
/// scripts/mapgen. Both produce the same RGF2 format.
///
/// Files land in the app's support directory — not the cache directory, which
/// the OS is free to purge — and are parsed in an isolate, because a 17 MB
/// graph takes long enough to drop frames.
///
/// Messages surface as i18n keys plus arguments rather than English text, the
/// same convention as firmware/firmware_updater.dart, so the UI localises them.
library;

import 'dart:async';
import 'dart:io';
import 'dart:isolate';

import 'package:flutter/foundation.dart';
import 'package:package_info_plus/package_info_plus.dart';
import 'package:path_provider/path_provider.dart';

import 'graph_builder.dart';
import 'overpass.dart';
import 'rgf2.dart';
import 'router.dart';
import 'search_index.dart';
import 'tile_cache.dart';

enum MapDataState { absent, downloading, loading, ready, error }

class MapData extends ChangeNotifier {
  MapData._();
  static final MapData instance = MapData._();

  static const graphFileName = 'route_graph.bin';
  static const indexFileName = 'search_index.tsv';

  Directory? _dir;
  MapDataState _state = MapDataState.absent;
  double _progress = 0;
  String? _progressFile;
  String? _messageKey;
  Map<String, String>? _messageArgs;

  RouteGraph? _graph;
  SearchIndex? _index;
  Router? _router;
  TileCache? _tiles;
  String _userAgent = 'AaBridgeNavigator/dev';

  MapDataState get state => _state;
  double get progress => _progress;
  String? get progressFile => _progressFile;
  String? get messageKey => _messageKey;
  Map<String, String>? get messageArgs => _messageArgs;
  RouteGraph? get graph => _graph;
  SearchIndex? get index => _index;
  Router? get router => _router;
  TileCache? get tiles => _tiles;
  String get userAgent => _userAgent;
  bool get isReady => _state == MapDataState.ready && _router != null;

  File? get graphFile =>
      _dir == null ? null : File('${_dir!.path}/$graphFileName');
  File? get indexFile =>
      _dir == null ? null : File('${_dir!.path}/$indexFileName');

  /// Resolves storage, builds the User-Agent and loads whatever is already
  /// downloaded. Safe to call without awaiting — the UI listens instead.
  Future<void> init() async {
    if (_dir != null) return;
    final support = await getApplicationSupportDirectory();
    _dir = Directory('${support.path}/nav');
    await _dir!.create(recursive: true);
    try {
      final info = await PackageInfo.fromPlatform();
      _userAgent = 'AaBridgeNavigator/${info.version} '
          '(+https://github.com/payalneg/esp32p4-android-auto)';
    } on Object {
      // Not fatal: a generic-looking UA is worse than a versioned one, but
      // failing to start the navigator over it would be sillier.
    }
    _tiles = TileCache(Directory('${support.path}/tiles/osm'),
        userAgent: _userAgent);
    await _loadFromDisk();
  }

  Future<void> _loadFromDisk() async {
    final g = graphFile;
    if (g == null || !await g.exists()) {
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
      _set(MapDataState.ready);
    } on FormatException catch (e) {
      _graph = null;
      _router = null;
      _fail('mapdata.err.badGraph', <String, String>{'err': e.message});
    } on Object catch (e) {
      _graph = null;
      _router = null;
      _fail('mapdata.err.load', <String, String>{'err': '$e'});
    }
  }

  /// Downloads the road network for [bounds] from Overpass and builds the
  /// routing graph on the phone.
  ///
  /// The heavy half — parsing tens of megabytes of JSON and walking every way
  /// — runs in an isolate; only the finished bytes come back.
  Future<void> buildFromOverpass(GeoBounds bounds) async {
    await init();
    _messageKey = null;
    _messageArgs = null;
    _set(MapDataState.downloading);
    try {
      final result = await OverpassClient().fetch(
        bounds,
        userAgent: _userAgent,
        onProgress: (bytes) {
          _progressFile = '${(bytes / (1 << 20)).toStringAsFixed(1)} MB';
          notifyListeners();
        },
      );
      _set(MapDataState.loading);
      final built = await Isolate.run(() => GraphBuilder.build(
            ways: result.ways,
            nodes: result.nodes,
            places: result.places,
          ));
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
      await _loadFromDisk();
    } on OverpassException catch (e) {
      _fail(e.messageKey, e.args);
    } on Object catch (e) {
      _fail('mapdata.err.download', <String, String>{'err': '$e'});
    }
  }

  /// Copies a file the user picked into place and loads it.
  Future<void> importGraph(String srcPath) => _import(srcPath, graphFile!);
  Future<void> importIndex(String srcPath) => _import(srcPath, indexFile!);

  Future<void> _import(String srcPath, File dest) async {
    await init();
    _set(MapDataState.loading);
    try {
      await File(srcPath).copy(dest.path);
      await _loadFromDisk();
    } on Object catch (e) {
      _fail('mapdata.err.import', <String, String>{'err': '$e'});
    }
  }

  Future<void> deleteAll() async {
    await init();
    for (final f in <File?>[graphFile, indexFile]) {
      if (f != null && await f.exists()) await f.delete();
    }
    _graph = null;
    _index = null;
    _router = null;
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
