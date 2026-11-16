import 'dart:io';

import 'package:aa_bridge/nav/krakow_tiles.dart';
import 'package:aa_bridge/nav/tile_cache.dart';
import 'package:aa_bridge/nav/tile_math.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:image/image.dart' as img;

void main() {
  TestWidgetsFlutterBinding.ensureInitialized();
  test('Krakow tiles are rendered offline and cached separately', () async {
    final dir = await Directory.systemTemp.createTemp('krakow-tiles-test-');
    addTearDown(() => dir.delete(recursive: true));
    final tile = deg2tile(50.0619, 19.9368, 17);
    final nodes = {
      1: tile2deg(tile.x + 0.2, tile.y + 0.5, tile.z),
      2: tile2deg(tile.x + 0.8, tile.y + 0.5, tile.z),
    };
    final kind = KrakowFeature.fromTags({
      'highway': 'residential',
      'cycleway:right': 'lane',
    }, false);
    expect(kind, KrakowFeature.bikeLane);
    final geometry = '${dir.path}/geometry';
    final builder = KrakowTileBuilder()..add(kind!, [1, 2]);
    await builder.write(geometry, (id) => nodes[id]);
    final source = KrakowTileSource(geometry, 'test');
    final cacheRoot = Directory('${dir.path}/cache');
    final stock = TileCache(cacheRoot, userAgent: 'test');
    final oldTile = stock.fileFor(tile);
    await oldTile.parent.create(recursive: true);
    await oldTile.writeAsBytes([1, 2, 3]);
    final cache = TileCache(
      cacheRoot,
      userAgent: 'test',
      krakow: source,
      clientFactory: () => throw StateError('network must not be used'),
    );

    expect(await cache.read(tile), isNull);
    final png = await cache.fetchAndStore(tile, tileUrls(tile));
    final image = img.decodePng(png!)!;
    expect((image.width, image.height), (256, 256));
    final pixel = image.getPixel(128, 128);
    expect(pixel.r, lessThan(210));
    expect(pixel.g, greaterThan(pixel.r));
    expect(png.length, lessThan(96 * 1024));
    expect(await cache.read(tile), png);
    expect(await oldTile.readAsBytes(), [1, 2, 3]);
    final outside = deg2tile(47.5, 19.04, 17);
    expect(source.covers(outside), isFalse);
    expect(cache.fileFor(outside).path, stock.fileFor(outside).path);
  });
}
