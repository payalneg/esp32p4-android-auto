/// The camera pose the navigator drives the map with.
library;

import 'package:aa_bridge/nav/geo.dart';
import 'package:aa_bridge/nav/location_service.dart';
import 'package:aa_bridge/nav/maneuvers.dart';
import 'package:aa_bridge/nav/nav_camera.dart';
import 'package:aa_bridge/nav/route_guide.dart';
import 'package:flutter_test/flutter_test.dart';

const _p = LatLon(50.06, 19.94);

GeoFix _fix({double speed = 5, double? heading}) =>
    GeoFix(position: _p, speedMs: speed, headingDeg: heading);

Guidance _turnIn(double m) => Guidance(
      alongM: 0,
      remainingM: 1000,
      remainingS: 200,
      offsetM: 0,
      offRoute: false,
      arrived: false,
      next: Maneuver(distM: m, type: ManeuverType.turnLeft, angleDeg: 90, point: _p),
      nextDistM: m,
    );

void main() {
  test('zoom follows speed, and a turn ahead zooms in regardless', () {
    expect(NavCamera.zoomForSpeed(1), 18);
    expect(NavCamera.zoomForSpeed(5), 17.5);
    expect(NavCamera.zoomForSpeed(10), 17);
    expect(NavCamera.zoomForSpeed(15), 16.5);
    final cam = NavCamera();
    final far = cam.pose(_fix(speed: 15), _turnIn(500),
        trackUp: false, viewportHeightPx: 800);
    expect(far.zoom, 16.5);
    final near = cam.pose(_fix(speed: 15), _turnIn(100),
        trackUp: false, viewportHeightPx: 800);
    expect(near.zoom, 18);
  });

  test('north-up keeps the rider centred and the map unrotated', () {
    final cam = NavCamera();
    final pose = cam.pose(_fix(heading: 90), null,
        trackUp: false, viewportHeightPx: 800);
    expect(pose.rotationDeg, 0);
    expect(pose.center, _p);
  });

  test('heading-up rotates against the heading and looks ahead', () {
    final cam = NavCamera();
    final pose = cam.pose(_fix(heading: 90), null,
        trackUp: true, viewportHeightPx: 800);
    expect(pose.rotationDeg, -90);
    // Riding east: the centre is east of the rider, same latitude.
    expect(pose.center.lon, greaterThan(_p.lon));
    expect(pose.center.lat, closeTo(_p.lat, 1e-6));
    // 800 px tall, rider 30 % up → centre 160 px ahead.
    final expectedM = 160 * NavCamera.metresPerPixel(pose.zoom, _p.lat);
    expect(haversineM(_p, pose.center), closeTo(expectedM, expectedM * 0.01));
  });

  test('a standing rider does not spin the map', () {
    final cam = NavCamera();
    cam.pose(_fix(heading: 90), null, trackUp: true, viewportHeightPx: 800);
    final pose = cam.pose(_fix(speed: 0.3, heading: 270), null,
        trackUp: true, viewportHeightPx: 800);
    expect(pose.rotationDeg, -90, reason: 'last good heading kept');
  });

  test('heading is smoothed the short way round', () {
    final cam = NavCamera();
    cam.pose(_fix(heading: 350), null, trackUp: true, viewportHeightPx: 800);
    cam.pose(_fix(heading: 10), null, trackUp: true, viewportHeightPx: 800);
    final h = cam.headingDeg!;
    // Between 350 and 10 through north — never through 180.
    expect(h > 350 || h < 10, isTrue, reason: 'got $h');
  });

  test('no heading yet means north up even in heading-up mode', () {
    final cam = NavCamera();
    final pose = cam.pose(_fix(speed: 5), null,
        trackUp: true, viewportHeightPx: 800);
    expect(pose.rotationDeg, 0);
    expect(pose.center, _p);
  });
}
