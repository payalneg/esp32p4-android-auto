/// What the head unit shows: the navigator view, rendered at the size the
/// picture is sent in.
///
/// This is a second, small map rather than a capture of the phone's own
/// screen. The phone is portrait, has floating buttons over the map and is
/// zoomed for a hand-held view; the head unit is a 800x480 panel on the
/// handlebar. Rendering it separately also means the picture stays put while
/// the rider pans and pinches their own map.
///
/// It is drawn at [kNavFrameW] x [kNavFrameH] and scaled down for the on-screen
/// preview: the layer underneath is always the full frame, whatever the
/// preview looks like, so [RepaintBoundary.toImage] captures the real thing.
library;

import 'package:flutter/material.dart';
import 'package:flutter_map/flutter_map.dart';
import 'package:latlong2/latlong.dart';

import '../../i18n/strings.dart';
import '../../nav/frame_codec.dart';
import '../../nav/nav_controller.dart';
import '../../nav/route_guide.dart';
import '../../nav/tile_cache.dart';
import 'cached_tile_provider.dart';
import 'maneuver_banner.dart';

/// The map tile source, kept in step with the navigator screen.
const String kHeadUnitTileUrl =
    'https://tile.openstreetmap.org/{z}/{x}/{y}.png';

class HeadUnitView extends StatelessWidget {
  const HeadUnitView({
    super.key,
    required this.controller,
    required this.mapController,
    required this.tiles,
    required this.routeLine,
    required this.boundaryKey,
    this.onMapReady,
  });

  final NavController controller;
  final MapController mapController;
  final TileCache? tiles;
  final List<LatLng> routeLine;
  final GlobalKey boundaryKey;

  /// Fired once the little map is laid out and safe to drive.
  final VoidCallback? onMapReady;

  @override
  Widget build(BuildContext context) {
    final fix = controller.lastFix;
    final guidance = controller.guidance;
    return RepaintBoundary(
      key: boundaryKey,
      child: SizedBox(
        width: kNavFrameW.toDouble(),
        height: kNavFrameH.toDouble(),
        child: ColoredBox(
          color: const Color(0xFF101418),
          child: Stack(
            children: <Widget>[
              FlutterMap(
                mapController: mapController,
                options: MapOptions(
                  initialCenter: const LatLng(kFallbackLat, kFallbackLon),
                  initialZoom: 13,
                  minZoom: 3,
                  maxZoom: 19,
                  backgroundColor: const Color(0xFF101418),
                  onMapReady: onMapReady,
                  // Nobody touches this map; it follows the rider.
                  interactionOptions:
                      const InteractionOptions(flags: InteractiveFlag.none),
                ),
                children: <Widget>[
                  if (tiles != null)
                    TileLayer(
                      urlTemplate: kHeadUnitTileUrl,
                      tileProvider: CachedTileProvider(tiles!),
                      userAgentPackageName: 'com.aabridge.aa_bridge',
                      maxZoom: 19,
                      maxNativeZoom: 19,
                      // No retina simulation: the head unit's panel is 800x480
                      // over 4.3 inches and the frame is upscaled 2x there, so
                      // a plain tile is already as sharp as the picture can be
                      // — and it costs a quarter of the tiles.
                      retinaMode: false,
                      errorImage: MemoryImage(kTransparentPng),
                      evictErrorTileStrategy:
                          EvictErrorTileStrategy.notVisibleRespectMargin,
                    ),
                  if (routeLine.isNotEmpty)
                    PolylineLayer<Object>(
                      polylines: <Polyline<Object>>[
                        Polyline<Object>(
                          points: routeLine,
                          color: const Color(0xFF1E64DC),
                          // Thinner than the phone's: the head unit doubles it.
                          strokeWidth: 4,
                          borderColor: Colors.white,
                          borderStrokeWidth: 1.5,
                        ),
                      ],
                    ),
                  MarkerLayer(
                    markers: <Marker>[
                      if (controller.finish != null)
                        Marker(
                          point: LatLng(controller.finish!.lat,
                              controller.finish!.lon),
                          width: 22,
                          height: 22,
                          alignment: Alignment.topCenter,
                          child: const Icon(Icons.place,
                              color: Colors.redAccent, size: 20),
                        ),
                      if (fix != null)
                        Marker(
                          point:
                              LatLng(fix.position.lat, fix.position.lon),
                          width: 18,
                          height: 18,
                          child: const _RiderDot(),
                        ),
                    ],
                  ),
                ],
              ),
              if (guidance != null && controller.hasRoute)
                Positioned(
                  top: 6,
                  left: 6,
                  child: _TurnPlate(
                    guidance: guidance,
                    label: guidance.offRoute
                        ? t(context, 'nav.guide.offRoute')
                        : formatDistance(context, guidance.nextDistM),
                  ),
                ),
              if (guidance != null && controller.hasRoute)
                Positioned(
                  right: 6,
                  bottom: 6,
                  child: _RemainingPlate(
                    text: tf(context, 'nav.hu.remaining', <String, Object?>{
                      'km': (guidance.remainingM / 1000).toStringAsFixed(1),
                      'min': (guidance.remainingS / 60).round(),
                    }),
                  ),
                ),
            ],
          ),
        ),
      ),
    );
  }
}

/// Kraków Main Square — the same fallback the navigator screen uses before a
/// position is known.
const double kFallbackLat = 50.0619;
const double kFallbackLon = 19.9368;

class _TurnPlate extends StatelessWidget {
  const _TurnPlate({required this.guidance, required this.label});

  final Guidance guidance;
  final String label;

  @override
  Widget build(BuildContext context) {
    final offRoute = guidance.offRoute;
    return DecoratedBox(
      decoration: BoxDecoration(
        color: offRoute ? const Color(0xE6B3261E) : const Color(0xE61C2530),
        borderRadius: BorderRadius.circular(8),
      ),
      child: Padding(
        padding: const EdgeInsets.symmetric(horizontal: 8, vertical: 6),
        child: Row(
          mainAxisSize: MainAxisSize.min,
          children: <Widget>[
            Icon(
              kManeuverIcons[guidance.next.type] ??
                  Icons.straight,
              color: Colors.white,
              size: 30,
            ),
            const SizedBox(width: 8),
            Text(
              label,
              style: const TextStyle(
                color: Colors.white,
                // Sized for a panel seen at arm's length, after the head unit
                // doubles it.
                fontSize: 26,
                fontWeight: FontWeight.w600,
                height: 1.1,
              ),
            ),
          ],
        ),
      ),
    );
  }
}

class _RemainingPlate extends StatelessWidget {
  const _RemainingPlate({required this.text});

  final String text;

  @override
  Widget build(BuildContext context) => DecoratedBox(
        decoration: BoxDecoration(
          color: const Color(0xE61C2530),
          borderRadius: BorderRadius.circular(8),
        ),
        child: Padding(
          padding: const EdgeInsets.symmetric(horizontal: 8, vertical: 4),
          child: Text(
            text,
            style: const TextStyle(
                color: Colors.white, fontSize: 18, fontWeight: FontWeight.w500),
          ),
        ),
      );
}

class _RiderDot extends StatelessWidget {
  const _RiderDot();

  @override
  Widget build(BuildContext context) => const DecoratedBox(
        decoration: BoxDecoration(
          color: Color(0xFF1E64DC),
          shape: BoxShape.circle,
          border: Border.fromBorderSide(
              BorderSide(color: Colors.white, width: 2.5)),
        ),
      );
}
