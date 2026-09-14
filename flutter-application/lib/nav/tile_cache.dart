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

/// Who is asking for a tile.
///
/// The screen always wins. A tile the rider is looking at is fetched before
/// any tile a background job merely wants on disk, however long that job has
/// been queued — otherwise "save the area" turns the live map into a black
/// grid for the length of the download.
enum TilePriority {
  /// Wanted on screen now. Newer requests beat older ones — see the
  /// `generation` argument of [TileCache.fetchAndStore].
  view,

  /// Wanted on disk eventually: corridors, area pyramids, the ride-along
  /// top-up. First come, first served among themselves.
  bulk,
}

/// Consecutive failures that end a bulk download: enough to ride out a bad
/// patch of signal, few enough that a dead network is noticed in seconds.
const int kStallAfterFailures = 12;

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
    this.stalled = false,
    this.lastError,
  });

  final int downloaded;
  final int skipped;
  final int failed;
  final bool cancelled;
  final bool blocked;

  /// Every recent tile failed, so the job gave up rather than grind on.
  final bool stalled;

  /// Cause of the last failure, for the message the user actually sees.
  final String? lastError;
}

/// One request waiting for a download slot.
class _Waiter {
  _Waiter(this.tile, this.priority, this.generation);

  final TileId tile;
  TilePriority priority;
  int generation;
  final Completer<void> done = Completer<void>();

  /// Strict ordering: view over bulk, newer generation over older, and
  /// otherwise whoever was queued first (the caller walks the list in order,
  /// so an equal candidate never replaces the earlier one).
  bool beats(_Waiter other) {
    if (priority != other.priority) return priority.index < other.priority.index;
    if (priority == TilePriority.view) return generation > other.generation;
    return false;
  }
}

class TileCache {
  TileCache(
    this.root, {
    required this.userAgent,
    this.minGap = const Duration(milliseconds: 100),
    this.maxConcurrent = 2,
    this.requestTimeout = const Duration(seconds: 20),
    HttpClient Function()? clientFactory,
  }) : _clientFactory = clientFactory ?? (() => HttpClient());

  /// Directory holding `{z}/{x}/{y}.png`.
  final Directory root;
  final String userAgent;
  final Duration minGap;
  final int maxConcurrent;

  /// How long one attempt at one tile may take, from connect to last byte.
  ///
  /// The whole request lives under this one deadline. Guarding only the header
  /// exchange, as an earlier version did, left a body that stopped arriving
  /// holding a concurrency slot for good: the map went black and an area
  /// download sat at "0 of 3000" until the app was killed.
  final Duration requestTimeout;

  final HttpClient Function() _clientFactory;

  final Map<TileId, Future<Uint8List?>> _inFlight = <TileId, Future<Uint8List?>>{};
  DateTime _lastRequest = DateTime.fromMillisecondsSinceEpoch(0);

  /// Requests waiting for a slot, in arrival order; [_pickNext] chooses among
  /// them by priority, not position. A queue rather than a counter and a sleep
  /// loop, so a slot passes straight to the next request instead of to
  /// whoever polls first.
  final List<_Waiter> _waiting = <_Waiter>[];
  int _active = 0;
  int _activeBulk = 0;

  /// How many slots background work may hold at once. One is always kept
  /// back for the screen, so a pan never queues behind two bulk fetches.
  int get _bulkCap => maxConcurrent > 1 ? maxConcurrent - 1 : 1;

  /// Bytes pulled over the network since this cache was created — the figure
  /// the download status shows, so it counts what was actually transferred
  /// rather than what was already on disk.
  int bytesFetched = 0;

  /// Network failures since this cache was created, and what the last one was.
  /// The download status shows them, so "nothing is happening yet" can be told
  /// apart from "every request is failing".
  int failures = 0;
  String? lastError;

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
  ///
  /// [priority] says who is asking; for [TilePriority.view] a higher
  /// [generation] means a newer viewport, and newer goes first — the tiles of
  /// a pan that has already ended should not hold up the ones on screen now.
  /// A tile already queued for a background job is promoted in place when the
  /// screen asks for it, so nothing is fetched twice and nothing the rider can
  /// see waits its turn behind the pyramid.
  Future<Uint8List?> fetchAndStore(
    TileId t,
    List<Uri> urls, {
    TilePriority priority = TilePriority.view,
    int generation = 0,
  }) {
    final pending = _inFlight[t];
    if (pending != null) {
      _promote(t, priority, generation);
      return pending; // several viewport tiles, one request
    }
    // Block body, not an arrow: Map.remove returns the value it removed —
    // here the very future being completed — and whenComplete waits for any
    // future its callback returns. Written as `=> _inFlight.remove(t)` this
    // line made every network fetch wait for itself, for ever: the map stayed
    // black and a tile download sat at "0 of N" while the bytes were quietly
    // landing on disk.
    final future = _fetchAny(t, urls, priority, generation).whenComplete(() {
      _inFlight.remove(t);
    });
    _inFlight[t] = future;
    return future;
  }

  Future<Uint8List?> _fetchAny(
      TileId t, List<Uri> urls, TilePriority priority, int generation) async {
    var blocked = 0;
    for (final url in urls) {
      try {
        final bytes = await _fetch(t, url, priority, generation);
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

  Future<Uint8List?> _fetch(
      TileId t, Uri url, TilePriority priority, int generation) async {
    await _acquireSlot(t, priority, generation);
    HttpClient? client;
    try {
      final since = DateTime.now().difference(_lastRequest);
      if (since < minGap) await Future<void>.delayed(minGap - since);
      _lastRequest = DateTime.now();

      client = _clientFactory()
        ..userAgent = userAgent
        ..connectionTimeout = const Duration(seconds: 10);
      return await _request(t, url, client).timeout(requestTimeout);
    } on TileBlockedException {
      rethrow;
    } on TimeoutException {
      _noteFailure('${url.host}: timeout');
      return null;
    } on Object catch (e) {
      _noteFailure('${url.host}: ${_shortCause(e)}');
      return null; // offline, DNS, TLS — all just "no tile"
    } finally {
      _releaseSlot(priority);
      // Force, not graceful: a socket that stalled mid-body has to be torn
      // down, and after a timeout there is nothing worth keeping alive.
      client?.close(force: true);
    }
  }

  /// One request, no timeout of its own — [_fetch] holds the deadline.
  Future<Uint8List?> _request(TileId t, Uri url, HttpClient client) async {
    final req = await client.getUrl(url);
    req.headers.set(HttpHeaders.acceptHeader, 'image/png,image/*');
    final resp = await req.close();
    // No drain on the paths below: the client is force-closed in _fetch, so
    // there is no connection to hand back to a pool, and draining a body we
    // do not want is one more thing that can stall.
    if (resp.statusCode == 403 || resp.statusCode == 429) {
      throw TileBlockedException(resp.statusCode);
    }
    if (resp.statusCode != 200) {
      _noteFailure('${url.host}: HTTP ${resp.statusCode}');
      return null;
    }
    final builder = BytesBuilder(copy: false);
    await for (final chunk in resp) {
      builder.add(chunk);
    }
    final bytes = builder.takeBytes();
    if (bytes.isEmpty) {
      _noteFailure('${url.host}: empty response');
      return null;
    }
    bytesFetched += bytes.length;
    await _store(t, bytes);
    return bytes;
  }

  Future<void> _acquireSlot(TileId t, TilePriority priority, int generation) {
    if (_canStart(priority)) {
      _take(priority);
      return Future<void>.value();
    }
    final w = _Waiter(t, priority, generation);
    _waiting.add(w);
    return w.done.future;
  }

  bool _canStart(TilePriority p) {
    if (_active >= maxConcurrent) return false;
    // Background work only steps in when nobody on screen is waiting, and
    // never into the slot kept for them.
    if (p == TilePriority.bulk) {
      return _activeBulk < _bulkCap &&
          !_waiting.any((w) => w.priority == TilePriority.view);
    }
    return true;
  }

  void _take(TilePriority p) {
    _active++;
    if (p == TilePriority.bulk) _activeBulk++;
  }

  void _releaseSlot(TilePriority p) {
    _active--;
    if (p == TilePriority.bulk) _activeBulk--;
    // Hand the slot straight on; a bulk waiter may still have to sit this one
    // out if the screen's cap says so, in which case the slot stays free for
    // the next view request.
    final next = _pickNext();
    if (next != null) {
      _waiting.remove(next);
      _take(next.priority);
      next.done.complete();
    }
  }

  /// The most deserving waiter that may start now: on-screen before
  /// background; among on-screen, the newest viewport, then arrival order;
  /// among background, arrival order.
  _Waiter? _pickNext() {
    _Waiter? best;
    for (final w in _waiting) {
      if (!_canStart(w.priority)) continue;
      if (best == null || w.beats(best)) best = w;
    }
    return best;
  }

  /// Raises a queued request to [priority]/[generation] if that is higher.
  void _promote(TileId t, TilePriority priority, int generation) {
    for (final w in _waiting) {
      if (w.tile != t) continue;
      if (priority.index < w.priority.index) {
        w.priority = priority;
        w.generation = generation;
      } else if (priority == w.priority && generation > w.generation) {
        w.generation = generation;
      }
      return;
    }
  }

  void _noteFailure(String what) {
    failures++;
    lastError = what;
  }

  /// A few words a person can act on, not a stack trace.
  static String _shortCause(Object e) {
    if (e is SocketException) {
      final os = e.osError?.message;
      return os != null && os.isNotEmpty ? os : 'no connection';
    }
    if (e is HandshakeException) return 'TLS';
    if (e is HttpException) return 'bad response';
    if (e is FileSystemException) return 'disk';
    return e.runtimeType.toString();
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
    /// The tile about to be fetched, before the request goes out. onProgress
    /// reports what is finished; this reports what is being waited on, which
    /// is the difference between a count that has stopped and a download that
    /// is simply on a slow tile.
    void Function(TileId tile)? onTile,
    bool Function()? cancelled,
  }) async {
    var downloaded = 0;
    var skipped = 0;
    var failed = 0;
    var inARow = 0;
    for (var i = 0; i < tiles.length; i++) {
      if (cancelled?.call() ?? false) {
        return CorridorReport(
            downloaded: downloaded,
            skipped: skipped,
            failed: failed,
            cancelled: true,
            blocked: false,
            lastError: lastError);
      }
      final t = tiles[i];
      if (await fileFor(t).exists()) {
        skipped++;
        inARow = 0;
      } else {
        onTile?.call(t);
        try {
          final bytes = await fetchAndStore(t, urlsFor(t),
              priority: TilePriority.bulk);
          if (bytes == null) {
            // One retry: a 5xx or a dropped connection is often transient.
            final again = await fetchAndStore(t, urlsFor(t),
                priority: TilePriority.bulk);
            if (again == null) {
              failed++;
              inARow++;
            } else {
              downloaded++;
              inARow = 0;
            }
          } else {
            downloaded++;
            inARow = 0;
          }
        } on TileBlockedException {
          return CorridorReport(
              downloaded: downloaded,
              skipped: skipped,
              failed: failed,
              cancelled: false,
              blocked: true,
              lastError: lastError);
        }
      }
      onProgress?.call(i + 1, tiles.length);
      // Thousands of tiles times two attempts times a timeout is an hour of
      // pointless waiting once the network is genuinely gone. Stop and say so.
      if (inARow >= kStallAfterFailures) {
        return CorridorReport(
            downloaded: downloaded,
            skipped: skipped,
            failed: failed,
            cancelled: false,
            blocked: false,
            stalled: true,
            lastError: lastError);
      }
    }
    return CorridorReport(
        downloaded: downloaded,
        skipped: skipped,
        failed: failed,
        cancelled: false,
        blocked: false,
        lastError: lastError);
  }
}
