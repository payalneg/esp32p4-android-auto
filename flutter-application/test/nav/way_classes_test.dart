/// Parity guard between the Dart profiles and scripts/mapgen/profiles.py.
///
/// The index of a class in WAY_CLASSES is written into every edge of the graph
/// file, so a reorder on either side silently reroutes traffic onto stairs.
/// This test reads the Python source directly, the way lisp_lint_test reads
/// lisp/main.lisp, so drift fails the suite instead of the ride.
library;

import 'dart:io';

import 'package:aa_bridge/nav/way_classes.dart';
import 'package:flutter_test/flutter_test.dart';

File _profilesPy() {
  for (final p in <String>[
    '../scripts/mapgen/profiles.py',
    'scripts/mapgen/profiles.py',
    '../../scripts/mapgen/profiles.py',
  ]) {
    final f = File(p);
    if (f.existsSync()) return f;
  }
  fail('scripts/mapgen/profiles.py not found from ${Directory.current.path}');
}

/// Pulls `WAY_CLASSES = (...)` out of the Python source.
List<String> _pyWayClasses(String src) {
  // Stop at a ')' that starts a line: the entries carry comments with their
  // own parentheses, which a lazy match would end on.
  final block = RegExp(r'WAY_CLASSES\s*=\s*\(\n(.*?)\n\)', dotAll: true)
      .firstMatch(src)!
      .group(1)!;
  return RegExp('"([a-z_]+)"')
      .allMatches(block)
      .map((m) => m.group(1)!)
      .toList();
}

/// Pulls one profile's `"class": value,` pairs out of PROFILES.
Map<String, double?> _pyProfile(String src, String name) {
  final body = RegExp('"${RegExp.escape(name)}"\\s*:\\s*\\{(.*?)\\n    \\}',
          dotAll: true)
      .firstMatch(src)!
      .group(1)!;
  final out = <String, double?>{};
  for (final m
      in RegExp('"([a-z_]+)"\\s*:\\s*(None|[0-9.]+)').allMatches(body)) {
    final v = m.group(2)!;
    out[m.group(1)!] = v == 'None' ? null : double.parse(v);
  }
  return out;
}

void main() {
  final src = _profilesPy().readAsStringSync();

  test('class list matches profiles.py exactly, in order', () {
    expect(kWayClasses, _pyWayClasses(src));
    // Hard-coded too: if both sides are edited together, this still catches a
    // change to the on-disk byte width assumption.
    expect(kWayClasses.length, 14);
    expect(kWayClassCount, kWayClasses.length);
  });

  test('every profile has a speed for every class', () {
    for (final p in RideProfile.values) {
      expect(kProfileSpeedsKmh[p]!.length, kWayClassCount,
          reason: '${p.name} speed table is the wrong length');
    }
  });

  test('speeds match profiles.py, including the forbidden classes', () {
    for (final p in RideProfile.values) {
      final py = _pyProfile(src, p.pythonName);
      expect(py.length, kWayClassCount, reason: 'python ${p.pythonName}');
      for (var i = 0; i < kWayClassCount; i++) {
        expect(kProfileSpeedsKmh[p]![i], py[kWayClasses[i]],
            reason: '${p.pythonName} / ${kWayClasses[i]}');
      }
    }
  });

  test('default profile matches DEFAULT_PROFILE', () {
    final name = RegExp('DEFAULT_PROFILE\\s*=\\s*"([^"]+)"')
        .firstMatch(src)!
        .group(1)!;
    expect(kDefaultProfile.pythonName, name);
  });

  test('speedsMs marks forbidden classes NaN and converts the rest', () {
    final scooter = speedsMs(RideProfile.scooter);
    expect(scooter[kWayClasses.indexOf('steps')].isNaN, isTrue);
    expect(scooter[kWayClasses.indexOf('sidewalk')], closeTo(15 / 3.6, 1e-9));
    expect(maxSpeedMs(RideProfile.scooter), closeTo(20 / 3.6, 1e-9));
  });

  test('sidewalks-only forbids the carriageway but keeps pavements', () {
    final s = speedsMs(RideProfile.sidewalksOnly);
    for (final banned in <String>['tertiary', 'residential', 'secondary', 'primary']) {
      expect(s[kWayClasses.indexOf(banned)].isNaN, isTrue, reason: banned);
    }
    expect(s[kWayClasses.indexOf('sidewalk')].isNaN, isFalse);
  });

  test('foot mask covers exactly the pedestrian classes', () {
    for (var i = 0; i < kWayClassCount; i++) {
      expect(kFootClassMask & (1 << i) != 0, kFootClasses.contains(kWayClasses[i]),
          reason: kWayClasses[i]);
    }
  });
}
