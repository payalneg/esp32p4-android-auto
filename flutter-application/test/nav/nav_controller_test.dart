/// The tap flow is where a route is actually made, and its states are easy to
/// get subtly wrong — asking for a point that is already known, or forgetting
/// one that is.
library;

import 'package:aa_bridge/nav/geo.dart';
import 'package:aa_bridge/nav/nav_controller.dart';
import 'package:flutter_test/flutter_test.dart';

void main() {
  const a = LatLon(50.06, 19.94);
  const b = LatLon(49.98, 20.06);

  test('two taps set the ends and ask for nothing more', () async {
    final c = NavController()..toggleTapMode();
    expect(c.tapMode, TapMode.start);

    c.onMapTap(a);
    expect(c.start, a);
    expect(c.tapMode, TapMode.finish);
    expect(c.messageKey, 'nav.route.tapFinish');

    c.onMapTap(b);
    expect(c.finish, b);
    expect(c.tapMode, TapMode.none);
  });

  test('a destination chosen from search only needs a start', () async {
    // The bug this pins: after setFinish the controller asked for a
    // destination again once the start arrived, though it already had one.
    final c = NavController();
    await c.setFinish(b);
    expect(c.finish, b);
    expect(c.tapMode, TapMode.start, reason: 'the start is what is missing');

    c.onMapTap(a);
    expect(c.start, a);
    expect(c.finish, b, reason: 'the destination survives');
    expect(c.tapMode, TapMode.none, reason: 'and is not asked for twice');
  });

  test('a start chosen from search only needs a destination', () async {
    final c = NavController();
    await c.setStart(a);
    expect(c.tapMode, TapMode.finish);
    c.onMapTap(b);
    expect(c.start, a);
    expect(c.finish, b);
  });

  test('taps do nothing until the mode is on', () {
    final c = NavController();
    c.onMapTap(a);
    expect(c.start, isNull);
    expect(c.finish, isNull);
  });

  test('reset clears the pair and the prompt', () {
    final c = NavController()..toggleTapMode();
    c.onMapTap(a);
    c.reset();
    expect(c.start, isNull);
    expect(c.tapMode, TapMode.none);
    expect(c.messageKey, isNull);
  });
}
