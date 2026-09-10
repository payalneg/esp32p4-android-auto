/// Storage behaviour of the tile cache. The network paths are exercised on a
/// device (see the manual steps in the branch description); what matters here
/// is that a cache full of tiles is read, measured and pruned correctly, and
/// that a corridor already on disk costs nothing.
library;

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
  });
}
