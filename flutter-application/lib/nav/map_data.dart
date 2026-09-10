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

import 'geofabrik.dart';
import 'graph_builder.dart';
import 'overpass.dart';
import 'pbf_graph.dart';
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
      final clock = Stopwatch()..start();
      final result = await OverpassClient().fetch(
        bounds,
        userAgent: _userAgent,
        onProgress: (cell, cells, bytes) {
          // Bytes and speed, not just a spinner: a road download is minutes
          // long and the only way to tell progress from a hang is numbers.
          final mb = bytes / (1 << 20);
          final seconds = clock.elapsedMilliseconds / 1000;
          final rate = seconds > 0 ? mb / seconds : 0;
          _progressFile = '${cells > 1 ? "$cell/$cells · " : ""}'
              '${mb.toStringAsFixed(1)} MB · '
              '${rate.toStringAsFixed(1)} MB/s';
          notifyListeners();
        },
      );
      _set(MapDataState.loading);
      final built = await Isolate.run(() => GraphBuilder.build(
            ways: result.ways,
            nodes: MapNodeSource(result.nodes),
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

  /// Downloads a Geofabrik extract and builds the graph from it — the same
  /// route scripts/mapgen takes.
  ///
  /// A whole province is one 190 MB file here against tens of gigabytes of
  /// Overpass JSON, which is why this is the only way to have routing over
  /// more than a few kilometres.
  Future<void> buildFromRegion(GeofabrikRegion region) async {
    await init();
    _messageKey = null;
    _messageArgs = null;
    _set(MapDataState.downloading);
    final pbf = File('${_dir!.path}/region.osm.pbf');
    final client = HttpClient()..userAgent = _userAgent;
    try {
      final clock = Stopwatch()..start();
      await _downloadTo(client, region.pbfUrl, pbf, (received, total) {
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
      final built = await Isolate.run(() => PbfGraphSource.buildAsync(pbf.path));
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
    } on Object catch (e) {
      _fail('mapdata.err.download', <String, String>{'err': '$e'});
    } finally {
      client.close(force: true);
      // The extract is only an input; keeping it would double the footprint.
      if (await pbf.exists()) await pbf.delete();
    }
  }

  Future<void> _downloadTo(HttpClient client, Uri url, File dest,
      void Function(int received, int total) onProgress) async {
    var target = url;
    HttpClientResponse resp;
    for (var hop = 0;; hop++) {
      resp = await (await client.getUrl(target)).close();
      final location = resp.headers.value(HttpHeaders.locationHeader);
      if (resp.statusCode >= 300 && resp.statusCode < 400 && location != null) {
        await resp.drain<void>();
        if (hop >= 5) throw HttpException('too many redirects', uri: url);
        target = target.resolve(location);
        continue;
      }
      break;
    }
    if (resp.statusCode != 200) {
      await resp.drain<void>();
      throw HttpException('HTTP ${resp.statusCode}', uri: target);
    }
    final total = resp.contentLength;
    final part = File('${dest.path}.part');
    final sink = part.openWrite();
    var received = 0;
    var lastReport = 0;
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
    await part.rename(dest.path);
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
