import 'package:aa_bridge/nav/geo.dart';
import 'package:flutter_test/flutter_test.dart';

void main() {
  group('haversineM', () {
    test('is zero for the same point', () {
      expect(haversineM(const LatLon(50, 20), const LatLon(50, 20)), 0);
    });

    test('one degree of latitude is about 111 km', () {
      final d = haversineM(const LatLon(50, 20), const LatLon(51, 20));
      expect(d, closeTo(111195, 200));
    });

    test('shrinks with latitude for a degree of longitude', () {
      final equator = haversineM(const LatLon(0, 0), const LatLon(0, 1));
      final krakow = haversineM(const LatLon(50, 19), const LatLon(50, 20));
      expect(krakow, lessThan(equator * 0.7));
    });
  });

  group('bearingDeg', () {
    test('cardinal directions', () {
      expect(bearingDeg(const LatLon(0, 0), const LatLon(1, 0)), closeTo(0, 0.1));
      expect(bearingDeg(const LatLon(0, 0), const LatLon(0, 1)), closeTo(90, 0.1));
      expect(
          bearingDeg(const LatLon(1, 0), const LatLon(0, 0)), closeTo(180, 0.1));
      expect(
          bearingDeg(const LatLon(0, 1), const LatLon(0, 0)), closeTo(270, 0.1));
    });
  });

  group('turnDelta', () {
    test('wraps across north', () {
      expect(turnDelta(350, 10), closeTo(20, 1e-9));
      expect(turnDelta(10, 350), closeTo(-20, 1e-9));
    });

    test('sign is right-positive', () {
      expect(turnDelta(0, 90), closeTo(90, 1e-9));
      expect(turnDelta(0, 270), closeTo(-90, 1e-9));
    });
  });

  group('cumulativeM / pointAtDistance', () {
    final line = <LatLon>[
      const LatLon(50.0, 20.0),
      const LatLon(50.01, 20.0),
      const LatLon(50.02, 20.0),
    ];

    test('cumulative starts at zero and increases', () {
      final cum = cumulativeM(line);
      expect(cum.length, line.length);
      expect(cum[0], 0);
      expect(cum[1], greaterThan(0));
      expect(cum[2], greaterThan(cum[1]));
    });

    test('interpolates inside a segment', () {
      final cum = cumulativeM(line);
      final mid = pointAtDistance(line, cum, cum[1] / 2);
      expect(mid.lat, closeTo(50.005, 1e-4));
      expect(mid.lon, closeTo(20.0, 1e-9));
    });

    test('clamps at both ends', () {
      final cum = cumulativeM(line);
      expect(pointAtDistance(line, cum, -100), line.first);
      expect(pointAtDistance(line, cum, 1e9), line.last);
    });
  });

  group('projectToSegment', () {
    test('foot of the perpendicular in the middle', () {
      final r = projectToSegment(const LatLon(50.005, 20.001),
          const LatLon(50.0, 20.0), const LatLon(50.01, 20.0));
      expect(r.t, closeTo(0.5, 0.02));
      expect(r.offsetM, closeTo(71.6, 5)); // 0.001° of lon at 50°N
    });

    test('clamps past the ends instead of extrapolating', () {
      final before = projectToSegment(const LatLon(49.99, 20.0),
          const LatLon(50.0, 20.0), const LatLon(50.01, 20.0));
      expect(before.t, 0);
      final after = projectToSegment(const LatLon(50.02, 20.0),
          const LatLon(50.0, 20.0), const LatLon(50.01, 20.0));
      expect(after.t, 1);
    });

    test('a degenerate segment still answers', () {
      final r = projectToSegment(const LatLon(50.001, 20.0),
          const LatLon(50.0, 20.0), const LatLon(50.0, 20.0));
      expect(r.t, 0);
      expect(r.offsetM, greaterThan(0));
    });
  });
}
