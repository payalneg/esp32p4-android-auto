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
import 'dart:io';

import 'package:flutter/foundation.dart';
import 'package:package_info_plus/package_info_plus.dart';
import 'package:path_provider/path_provider.dart';

import 'geofabrik.dart';
import 'pbf_graph.dart';
import 'rgf2.dart';
import 'router.dart';
import 'search_index.dart';
import 'tile_cache.dart';
import '../log/app_log.dart';

enum MapDataState { absent, downloading, loading, ready, error }

class MapData extends ChangeNotifier {
  MapData._();
  static final MapData instance = MapData._();

  /// A ready instance around an in-memory graph, for controller tests that
  /// need a router without files or isolates.
  @visibleForTesting
  MapData.forTest(RouteGraph graph)
      : _graph = graph,
        _router = Router(graph),
        _state = MapDataState.ready;

  static const graphFileName = 'route_graph.bin';
  static const indexFileName = 'search_index.tsv';

  Directory? _dir;
  MapDataState _state = MapDataState.absent;
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
  String _userAgent = 'AaBridgeNavigator/dev';

  MapDataState get state => _state;
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

  /// Downloads a Geofabrik extract and builds the graph from it — the same
  /// route scripts/mapgen takes.
  ///
  /// A whole province is one 190 MB file, which is what makes routing over
  /// more than a neighbourhood practical at all.
  Future<void> buildFromRegion(GeofabrikRegion region) async {
    await init();
    _messageKey = null;
    _messageArgs = null;
    _set(MapDataState.downloading);
    AppLog.i('map', 'region ${region.name}: downloading ${region.pbfUrl}');
    final pbf = File('${_dir!.path}/region.osm.pbf');
    final client = HttpClient()..userAgent = _userAgent;
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
      await _loadFromDisk();
    } on Object catch (e) {
      AppLog.e('map', 'region build failed: $e');
      _fail('mapdata.err.download', <String, String>{'err': '$e'});
    } finally {
      _stageKey = null;
      client.close(force: true);
      // The extract is only an input; keeping it would double the footprint.
      if (await pbf.exists()) await pbf.delete();
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
