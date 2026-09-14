/// Decides what to say, and when, while riding a route.
///
/// Pure Dart on purpose: it takes the [Guidance] the guide already computes
/// per fix and returns [Announcement]s — small, string-free records the UI
/// turns into a localised phrase and a buzz. Nothing here knows about TTS or
/// the screen, so the timing rules are unit-tested with a list of guidances,
/// not a ride.
///
/// Two calls per turn, like every navigator: "in 200 metres, turn left" far
/// enough out to slow down, then "turn left" at the junction. How far out
/// scales with speed — at 30 km/h a hundred metres is twelve seconds — and is
/// clamped so a walker still hears it in time and a fast rider is not warned
/// half a kilometre early.
library;

import 'maneuvers.dart';
import 'route_guide.dart';

/// Seconds of riding the "prepare" call is given before the junction.
const double kPrepareSeconds = 15;
const double kPrepareMinM = 100;
const double kPrepareMaxM = 400;

/// Seconds before the junction the "now" call lands.
const double kNowSeconds = 3;
const double kNowMinM = 20;
const double kNowMaxM = 50;

/// Spoken distances are rounded to this — "180 metres" is noise.
const int kSpokenStepM = 50;

enum AnnouncementKind {
  /// "In N metres, <action>."
  prepare,

  /// "<action>" — at the junction.
  now,

  /// "In N metres you reach your destination."
  arriveSoon,

  /// "You have arrived."
  arrived,

  /// "Off route, rerouting." Emitted by NavController when it starts a
  /// reroute, not by [TurnAnnouncer] — the announcer only knows the route it
  /// is on, not whether a new one is coming.
  rerouting,
}

class Announcement {
  const Announcement(this.kind, {this.maneuver, this.distM});

  final AnnouncementKind kind;

  /// The turn being announced; null for arrival.
  final ManeuverType? maneuver;

  /// Rounded distance to say, for the kinds that say one.
  final int? distM;

  @override
  String toString() => 'Announcement($kind, $maneuver, $distM)';
}

class TurnAnnouncer {
  Maneuver? _preparedFor;
  Maneuver? _calledFor;
  bool _saidArrived = false;

  /// Forget what was said — the route changed.
  void reset() {
    _preparedFor = null;
    _calledFor = null;
    _saidArrived = false;
  }

  /// Announcements due for this fix, oldest first. Usually none, sometimes
  /// one; two only when a turn is so close that its prepare and now calls
  /// fall in the same fix.
  List<Announcement> update(Guidance g, double? speedMs) {
    final out = <Announcement>[];
    final s = (speedMs ?? 0).clamp(0.0, double.infinity);
    final prepareAt = (s * kPrepareSeconds).clamp(kPrepareMinM, kPrepareMaxM);
    final nowAt = (s * kNowSeconds).clamp(kNowMinM, kNowMaxM);

    if (g.arrived) {
      if (!_saidArrived) {
        _saidArrived = true;
        out.add(const Announcement(AnnouncementKind.arrived));
      }
      return out;
    }

    final next = g.next;
    // A "straight" entry marks a fork where the route goes on; nobody needs
    // to be told to keep going.
    if (next.type == ManeuverType.straight) return out;
    final isArrive = next.type == ManeuverType.arrive;
    final d = g.nextDistM;

    if (d <= prepareAt && !identical(_preparedFor, next)) {
      _preparedFor = next;
      // Too close for a warning to be worth anything on its own: fall
      // through to the immediate call instead of saying both in one breath.
      if (d > nowAt) {
        out.add(Announcement(
          isArrive ? AnnouncementKind.arriveSoon : AnnouncementKind.prepare,
          maneuver: isArrive ? null : next.type,
          distM: _spoken(d),
        ));
      }
    }
    if (!isArrive && d <= nowAt && !identical(_calledFor, next)) {
      _calledFor = next;
      out.add(Announcement(AnnouncementKind.now, maneuver: next.type));
    }
    return out;
  }

  static int _spoken(double d) {
    final r = (d / kSpokenStepM).round() * kSpokenStepM;
    return r < kSpokenStepM ? kSpokenStepM : r;
  }
}
