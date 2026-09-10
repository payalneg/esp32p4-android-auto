/// Serves map tiles from our own on-disk cache, falling back to the network.
///
/// flutter_map's stock NetworkTileProvider has its own revalidating cache, but
/// it is a browser-style cache, not an offline store: it will not hand back a
/// tile once the phone has no connection. This provider reads the file first
/// and only then reaches out, which is what lets a downloaded corridor keep
/// working in a tunnel.
///
/// A tile that is neither cached nor reachable throws, and TileLayer paints
/// its `errorImage` — a transparent pixel over the map's dark background, so
/// gaps read as "not downloaded" rather than as a broken app.
library;

import 'dart:async';
import 'dart:ui' as ui;

import 'package:flutter/foundation.dart';
import 'package:flutter/painting.dart';
import 'package:flutter_map/flutter_map.dart';

import '../../nav/tile_cache.dart';
import '../../nav/tile_math.dart';

/// 1×1 transparent PNG, used where a tile is missing.
final Uint8List kTransparentPng = Uint8List.fromList(<int>[
  0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A, 0x00, 0x00, 0x00, 0x0D,
  0x49, 0x48, 0x44, 0x52, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01,
  0x08, 0x06, 0x00, 0x00, 0x00, 0x1F, 0x15, 0xC4, 0x89, 0x00, 0x00, 0x00,
  0x0A, 0x49, 0x44, 0x41, 0x54, 0x78, 0x9C, 0x63, 0x00, 0x01, 0x00, 0x00,
  0x05, 0x00, 0x01, 0x0D, 0x0A, 0x2D, 0xB4, 0x00, 0x00, 0x00, 0x00, 0x49,
  0x45, 0x4E, 0x44, 0xAE, 0x42, 0x60, 0x82,
]);

class CachedTileProvider extends TileProvider {
  CachedTileProvider(this.cache);

  final TileCache cache;

  @override
  ImageProvider<Object> getImage(
      TileCoordinates coordinates, TileLayer options) {
    final tile = TileId(coordinates.z, coordinates.x, coordinates.y);
    return _CachedTileImage(cache, tile, Uri.parse(getTileUrl(coordinates, options)));
  }
}

class _CachedTileImage extends ImageProvider<_CachedTileImage> {
  const _CachedTileImage(this.cache, this.tile, this.url);

  final TileCache cache;
  final TileId tile;
  final Uri url;

  @override
  Future<_CachedTileImage> obtainKey(ImageConfiguration configuration) =>
      SynchronousFuture<_CachedTileImage>(this);

  @override
  ImageStreamCompleter loadImage(
      _CachedTileImage key, ImageDecoderCallback decode) {
    return MultiFrameImageStreamCompleter(
      codec: _load(decode),
      scale: 1,
      debugLabel: 'tile ${tile.z}/${tile.x}/${tile.y}',
    );
  }

  Future<ui.Codec> _load(ImageDecoderCallback decode) async {
    final bytes = await cache.read(tile) ?? await cache.fetchAndStore(tile, url);
    if (bytes == null) {
      throw StateError('tile $tile unavailable'); // TileLayer draws errorImage
    }
    return decode(await ui.ImmutableBuffer.fromUint8List(bytes));
  }

  @override
  bool operator ==(Object other) =>
      other is _CachedTileImage && other.tile == tile;

  @override
  int get hashCode => tile.hashCode;
}
