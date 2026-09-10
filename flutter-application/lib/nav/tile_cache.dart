/// On-device store for raster map tiles.
///
/// Cache-first: a tile already on disk is served without touching the network,
/// which is what makes the map work in a tunnel or on a dead phone plan. New
/// tiles are fetched with `dart:io HttpClient` (the project avoids
/// package:http — see agent/ai_client.dart) and written through a `.part` file
/// so a crash mid-download cannot leave a half PNG behind.
///
/// The OSM tile usage policy is what shapes the rest: a User-Agent that names
/// the app, a hard cap on concurrency, a minimum gap between requests, and a
/// corridor download that stops rather than hammers when the server pushes
/// back with 403/429.
library;

import 'dart:async';
import 'dart:io';
import 'dart:typed_data';

import 'tile_math.dart';

/// Thrown when the tile server refuses us — bulk download must stop, not retry.
class TileBlockedException implements Exception {
  TileBlockedException(this.statusCode);
  final int statusCode;
  @override
  String toString() => 'tile server refused the request ($statusCode)';
}

class CorridorReport {
  const CorridorReport({
    required this.downloaded,
    required this.skipped,
    required this.failed,
    required this.cancelled,
    required this.blocked,
  });

  final int downloaded;
  final int skipped;
  final int failed;
  final bool cancelled;
  final bool blocked;
}

class TileCache {
  TileCache(
    this.root, {
    required this.userAgent,
    this.minGap = const Duration(milliseconds: 100),
    this.maxConcurrent = 2,
    HttpClient Function()? clientFactory,
  }) : _clientFactory = clientFactory ?? (() => HttpClient());

  /// Directory holding `{z}/{x}/{y}.png`.
  final Directory root;
  final String userAgent;
  final Duration minGap;
  final int maxConcurrent;
  final HttpClient Function() _clientFactory;

  final Map<TileId, Future<Uint8List?>> _inFlight = <TileId, Future<Uint8List?>>{};
  DateTime _lastRequest = DateTime.fromMillisecondsSinceEpoch(0);
  int _active = 0;

  /// Bytes pulled over the network since this cache was created — the figure
  /// the download status shows, so it counts what was actually transferred
  /// rather than what was already on disk.
  int bytesFetched = 0;

  File fileFor(TileId t) =>
      File('${root.path}/${t.z}/${t.x}/${t.y}.png');

  /// Cached bytes, or null when the tile is not stored yet.
  Future<Uint8List?> read(TileId t) async {
    final f = fileFor(t);
    if (!await f.exists()) return null;
    try {
      return await f.readAsBytes();
    } on FileSystemException {
      return null; // torn write from an older run; treat as a miss
    }
  }

  /// Fetches a tile and stores it, trying each address in turn.
  ///
  /// Returns null on any network failure — the caller draws a blank rather
  /// than an error, because a missing tile is normal offline. Throws
  /// [TileBlockedException] only when every mirror refuses, so a bulk download
  /// stops instead of digging deeper into servers that have said no.
  Future<Uint8List?> fetchAndStore(TileId t, List<Uri> urls) {
    final pending = _inFlight[t];
    if (pending != null) return pending; // several viewport tiles, one request
    final future = _fetchAny(t, urls).whenComplete(() => _inFlight.remove(t));
    _inFlight[t] = future;
    return future;
  }

  Future<Uint8List?> _fetchAny(TileId t, List<Uri> urls) async {
    var blocked = 0;
    for (final url in urls) {
      try {
        final bytes = await _fetch(t, url);
        if (bytes != null) return bytes;
      } on TileBlockedException {
        blocked++;
      }
    }
    // Everyone refused: that is a policy answer, not a flaky network.
    if (blocked == urls.length && urls.isNotEmpty) {
      throw TileBlockedException(429);
    }
    return null;
  }

  Future<Uint8List?> _fetch(TileId t, Uri url) async {
    while (_active >= maxConcurrent) {
      await Future<void>.delayed(const Duration(milliseconds: 20));
    }
    _active++;
    HttpClient? client;
    try {
      final since = DateTime.now().difference(_lastRequest);
      if (since < minGap) await Future<void>.delayed(minGap - since);
      _lastRequest = DateTime.now();

      client = _clientFactory();
      client.userAgent = userAgent;
      final req = await client.getUrl(url).timeout(const Duration(seconds: 30));
      req.headers.set(HttpHeaders.acceptHeader, 'image/png,image/*');
      final resp = await req.close().timeout(const Duration(seconds: 30));
      if (resp.statusCode == 403 || resp.statusCode == 429) {
        await resp.drain<void>();
        throw TileBlockedException(resp.statusCode);
      }
      if (resp.statusCode != 200) {
        await resp.drain<void>();
        return null;
      }
      final builder = BytesBuilder(copy: false);
      await for (final chunk in resp) {
        builder.add(chunk);
      }
      final bytes = builder.takeBytes();
      if (bytes.isEmpty) return null;
      bytesFetched += bytes.length;
      await _store(t, bytes);
      return bytes;
    } on TileBlockedException {
      rethrow;
    } on Object {
      return null; // offline, DNS, timeout, TLS — all just "no tile"
    } finally {
      _active--;
      client?.close(force: true);
    }
  }

  Future<void> _store(TileId t, Uint8List bytes) async {
    final f = fileFor(t);
    await f.parent.create(recursive: true);
    final part = File('${f.path}.part');
    await part.writeAsBytes(bytes, flush: true);
    await part.rename(f.path); // atomic: readers never see a partial tile
  }

  /// Total size and file count of the cache.
  Future<({int bytes, int files})> stats() async {
    if (!await root.exists()) return (bytes: 0, files: 0);
    var bytes = 0;
    var files = 0;
    await for (final e in root.list(recursive: true, followLinks: false)) {
      if (e is! File || !e.path.endsWith('.png')) continue;
      bytes += await e.length();
      files++;
    }
    return (bytes: bytes, files: files);
  }

  Future<void> clear() async {
    if (await root.exists()) await root.delete(recursive: true);
  }

  /// Deletes the least recently written tiles until the cache fits [capBytes].
  Future<void> evictToCap(int capBytes) async {
    if (!await root.exists()) return;
    final entries = <({File file, int size, DateTime at})>[];
    var total = 0;
    await for (final e in root.list(recursive: true, followLinks: false)) {
      if (e is! File || !e.path.endsWith('.png')) continue;
      final stat = await e.stat();
      total += stat.size;
      entries.add((file: e, size: stat.size, at: stat.modified));
    }
    if (total <= capBytes) return;
    entries.sort((a, b) => a.at.compareTo(b.at));
    for (final e in entries) {
      if (total <= capBytes) break;
      try {
        await e.file.delete();
        total -= e.size;
      } on FileSystemException {
        // raced with a write; skip it
      }
    }
  }

  /// Downloads a whole corridor of tiles, one at a time.
  ///
  /// Sequential on purpose: this is the one place the app asks for many tiles
  /// at once, and pacing it is the difference between a courteous client and
  /// a scraper.
  Future<CorridorReport> downloadCorridor(
    List<TileId> tiles,
    List<Uri> Function(TileId) urlsFor, {
    void Function(int done, int total)? onProgress,
    bool Function()? cancelled,
  }) async {
    var downloaded = 0;
    var skipped = 0;
    var failed = 0;
    for (var i = 0; i < tiles.length; i++) {
      if (cancelled?.call() ?? false) {
        return CorridorReport(
            downloaded: downloaded,
            skipped: skipped,
            failed: failed,
            cancelled: true,
            blocked: false);
      }
      final t = tiles[i];
      if (await fileFor(t).exists()) {
        skipped++;
      } else {
        try {
          final bytes = await fetchAndStore(t, urlsFor(t));
          if (bytes == null) {
            // One retry: a 5xx or a dropped connection is often transient.
            final again = await fetchAndStore(t, urlsFor(t));
            again == null ? failed++ : downloaded++;
          } else {
            downloaded++;
          }
        } on TileBlockedException {
          return CorridorReport(
              downloaded: downloaded,
              skipped: skipped,
              failed: failed,
              cancelled: false,
              blocked: true);
        }
      }
      onProgress?.call(i + 1, tiles.length);
    }
    return CorridorReport(
        downloaded: downloaded,
        skipped: skipped,
        failed: failed,
        cancelled: false,
        blocked: false);
  }
}
