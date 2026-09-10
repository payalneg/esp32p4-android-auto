/// Storage behaviour of the tile cache. The network paths are exercised on a
/// device (see the manual steps in the branch description); what matters here
/// is that a cache full of tiles is read, measured and pruned correctly, and
/// that a corridor already on disk costs nothing.
library;

import 'dart:async';
import 'dart:io';
import 'dart:typed_data';

import 'package:aa_bridge/nav/tile_cache.dart';
import 'package:aa_bridge/nav/tile_math.dart';
import 'package:flutter_test/flutter_test.dart';

void main() {
  late Directory root;
  late TileCache cache;

  setUp(() {
    root = Directory.systemTemp.createTempSync('tilecache');
    cache = TileCache(root, userAgent: 'test/1.0');
  });

  tearDown(() {
    if (root.existsSync()) root.deleteSync(recursive: true);
  });

  Future<void> put(TileId t, {int size = 64, DateTime? at}) async {
    final f = cache.fileFor(t);
    await f.parent.create(recursive: true);
    await f.writeAsBytes(Uint8List(size));
    if (at != null) await f.setLastModified(at);
  }

  test('lays tiles out as {z}/{x}/{y}.png', () {
    expect(cache.fileFor(const TileId(16, 36397, 22208)).path,
        '${root.path}/16/36397/22208.png');
  });

  test('reads back what was stored, and nothing else', () async {
    const t = TileId(16, 1, 2);
    expect(await cache.read(t), isNull);
    await put(t, size: 12);
    expect((await cache.read(t))!.length, 12);
    expect(await cache.read(const TileId(16, 1, 3)), isNull);
  });

  test('stats counts png files only', () async {
    await put(const TileId(16, 1, 1), size: 100);
    await put(const TileId(16, 1, 2), size: 200);
    File('${root.path}/16/1/2.png.part').writeAsBytesSync(Uint8List(999));
    final s = await cache.stats();
    expect(s.files, 2);
    expect(s.bytes, 300);
  });

  test('stats on an empty cache is zero, not an error', () async {
    final missing =
        TileCache(Directory('${root.path}/nope'), userAgent: 'test/1.0');
    expect(await missing.stats(), (bytes: 0, files: 0));
  });

  test('clear removes everything', () async {
    await put(const TileId(16, 1, 1));
    await cache.clear();
    expect(root.existsSync(), isFalse);
    expect(await cache.stats(), (bytes: 0, files: 0));
  });

  test('evictToCap drops the oldest tiles first', () async {
    final now = DateTime.now();
    await put(const TileId(16, 1, 1),
        size: 100, at: now.subtract(const Duration(days: 3)));
    await put(const TileId(16, 1, 2),
        size: 100, at: now.subtract(const Duration(days: 2)));
    await put(const TileId(16, 1, 3), size: 100, at: now);

    await cache.evictToCap(250);

    expect(await cache.read(const TileId(16, 1, 1)), isNull, reason: 'oldest');
    expect(await cache.read(const TileId(16, 1, 3)), isNotNull, reason: 'newest');
    expect((await cache.stats()).bytes, lessThanOrEqualTo(250));
  });

  test('evictToCap leaves a cache that already fits alone', () async {
    await put(const TileId(16, 1, 1), size: 100);
    await cache.evictToCap(1000);
    expect((await cache.stats()).files, 1);
  });

  group('downloadCorridor', () {
    final tiles = <TileId>[
      const TileId(16, 1, 1),
      const TileId(16, 1, 2),
      const TileId(16, 1, 3),
    ];
    // Any request here would be a bug: every tile is already on disk.
    List<Uri> boom(TileId t) => throw StateError('should not fetch $t');

    test('skips tiles that are already cached', () async {
      for (final t in tiles) {
        await put(t);
      }
      final progress = <int>[];
      final report = await cache.downloadCorridor(tiles, boom,
          onProgress: (done, total) => progress.add(done));
      expect(report.skipped, 3);
      expect(report.downloaded, 0);
      expect(report.failed, 0);
      expect(report.blocked, isFalse);
      expect(progress, <int>[1, 2, 3]);
    });

    test('stops when told to', () async {
      for (final t in tiles) {
        await put(t);
      }
      var seen = 0;
      final report = await cache.downloadCorridor(tiles, boom,
          onProgress: (done, _) => seen = done, cancelled: () => seen >= 2);
      expect(report.cancelled, isTrue);
      expect(report.skipped, 2);
    });

    test('an empty corridor is a no-op', () async {
      final report = await cache.downloadCorridor(const <TileId>[], boom);
      expect(report.downloaded, 0);
      expect(report.cancelled, isFalse);
    });

    test('gives up once every request in a row fails', () async {
      // Nothing is listening on this port, so each attempt fails at once —
      // what matters is that the job ends by itself instead of walking all
      // 200 tiles twice.
      final dead = Uri.parse('http://127.0.0.1:1/{z}.png');
      final tiles = <TileId>[
        for (var i = 0; i < 200; i++) TileId(16, 7, i),
      ];
      final report = await cache
          .downloadCorridor(tiles, (TileId t) => <Uri>[dead])
          .timeout(const Duration(seconds: 30));
      expect(report.stalled, isTrue);
      expect(report.failed, kStallAfterFailures);
      expect(report.lastError, isNotNull);
    });
  });

  /// The failure that hung the app: headers arrived, the body never did, and
  /// the fetch — guarded only around the header exchange — kept its
  /// concurrency slot for good. Every tile after it queued behind a request
  /// that would never end, so the map went black and an area download sat at
  /// "0 of N" until the app was killed.
  group('a body that stops arriving', () {
    late HttpServer server;
    late TileCache net;

    setUp(() async {
      server = await HttpServer.bind(InternetAddress.loopbackIPv4, 0);
      server.listen((HttpRequest req) async {
        final res = req.response;
        if (req.uri.path.startsWith('/stall')) {
          res.contentLength = 4096; // promised, never delivered
          res.add(<int>[1, 2, 3]);
          await res.flush();
          return; // and now silence, with the response left open
        }
        res.add(Uint8List(64));
        await res.close();
      });
      net = TileCache(root,
          userAgent: 'test/1.0',
          minGap: Duration.zero,
          maxConcurrent: 1,
          requestTimeout: const Duration(milliseconds: 300));
    });

    tearDown(() async => server.close(force: true));

    Uri url(String path) =>
        Uri.parse('http://127.0.0.1:${server.port}$path');

    test('times out instead of waiting for ever', () async {
      final bytes = await net
          .fetchAndStore(const TileId(16, 1, 1), <Uri>[url('/stall/a.png')])
          .timeout(const Duration(seconds: 10));
      expect(bytes, isNull);
      expect(net.failures, greaterThan(0));
      expect(net.lastError, contains('timeout'));
    });

    test('two callers for one tile share the request and both get bytes',
        () async {
      // The in-flight map used to be cleared with `whenComplete(() =>
      // map.remove(t))`, which handed whenComplete the future it was about to
      // complete and left it waiting on itself. Both of these would hang.
      final a = net.fetchAndStore(const TileId(16, 2, 1), <Uri>[url('/ok/a')]);
      final b = net.fetchAndStore(const TileId(16, 2, 1), <Uri>[url('/ok/a')]);
      final both = await Future.wait<Uint8List?>(<Future<Uint8List?>>[a, b])
          .timeout(const Duration(seconds: 10));
      expect(both[0], isNotNull);
      expect(both[1], isNotNull);
    });

    test('hands its slot on, so the next tile still loads', () async {
      // Fire and forget: this one will hang until its deadline.
      unawaited(net.fetchAndStore(
          const TileId(16, 1, 1), <Uri>[url('/stall/a.png')]));
      await Future<void>.delayed(const Duration(milliseconds: 20));

      final bytes = await net
          .fetchAndStore(const TileId(16, 1, 2), <Uri>[url('/ok/b.png')])
          .timeout(const Duration(seconds: 10));
      expect(bytes, isNotNull, reason: 'the single slot was released');
      expect(await net.read(const TileId(16, 1, 2)), isNotNull);
    });
  });
}
