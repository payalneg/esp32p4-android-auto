/// Deciding when a position deserves an unsolicited "download this region".
///
/// The navigator is offline-first but the phone has to be told which piece of
/// the planet it is standing on. That is what this does: when a fix lands
/// outside everything already downloaded, it looks the position up in the
/// Geofabrik catalogue and hands back the regional extract that covers it.
///
/// The rules live here, away from the widget, because "when not to interrupt"
/// is the whole feature: no offer when a build is running, when the region is
/// the one already on the phone, when it has just been waved away, when the
/// rider turned the offers off, and not more than once per few kilometres
/// while a lookup is failing.
library;

import 'package:flutter/foundation.dart';

import '../log/app_log.dart';
import '../settings/nav_settings.dart';
import 'geo.dart';
import 'geofabrik.dart';
import 'map_data.dart';

/// Why a region is being offered: where the rider is, or where they are going.
enum RegionSuggestionReason { here, destination }

/// A region plus the label worth showing for it.
@immutable
class RegionSuggestion {
  const RegionSuggestion(this.region, this.label,
      {this.saved = false, this.reason = RegionSuggestionReason.here});

  final GeofabrikRegion region;

  /// `Poland / Lesser Poland Voivodeship` — the catalogue's own names, minus
  /// the continent on top, which fits on one line of a banner.
  final String label;

  /// Whether this map is already kept on disk, which turns the offer from a
  /// download into a switch.
  final bool saved;

  /// Where the position that prompted the offer came from.
  final RegionSuggestionReason reason;

  @override
  bool operator ==(Object other) =>
      other is RegionSuggestion &&
      other.region.id == region.id &&
      other.label == label &&
      other.saved == saved &&
      other.reason == reason;

  @override
  int get hashCode => Object.hash(region.id, label, saved, reason);

  @override
  String toString() =>
      'RegionSuggestion(${region.id}${saved ? ', saved' : ''}, $reason)';
}

class RegionSuggester {
  RegionSuggester({
    required MapData map,
    required NavSettings settings,
    Future<GeofabrikIndex> Function()? loadCatalogue,
  })  : _map = map,
        _settings = settings,
        _loadCatalogue =
            loadCatalogue ?? (() => map.regionCatalogue(geometry: true));

  final MapData _map;
  final NavSettings _settings;
  final Future<GeofabrikIndex> Function() _loadCatalogue;

  /// Check again only after this long…
  static const Duration recheckAfter = Duration(minutes: 5);

  /// …or this far from where the last check was made. Both have to hold: a
  /// rider standing still must not re-download a catalogue, and one riding
  /// through a city must not ask at every junction.
  static const double recheckDistanceM = 5000;

  GeofabrikIndex? _catalogue;
  bool _loading = false;
  DateTime? _checkedAt;
  LatLon? _checkedFrom;

  /// Forgets the throttle, so a freshly downloaded map is judged by what it
  /// covers rather than by a decision made before it existed.
  void reset() {
    _checkedAt = null;
    _checkedFrom = null;
  }

  bool get _busy =>
      !_map.settled ||
      _map.state == MapDataState.downloading ||
      _map.state == MapDataState.loading ||
      _map.state == MapDataState.error;

  /// The region to offer for [at], or null when the answer is "leave the rider
  /// alone".
  ///
  /// [oncePerPlace] is the normal case for a position fix. A destination may
  /// be anywhere, so it is asked without moving the throttle that belongs to
  /// where the rider actually is.
  Future<RegionSuggestion?> check(LatLon at,
      {bool oncePerPlace = true,
      RegionSuggestionReason reason = RegionSuggestionReason.here}) async {
    if (!_settings.autoRegion || _busy || _loading) return null;
    if (_map.covers(at)) return null;

    if (oncePerPlace) {
      final checkedAt = _checkedAt;
      final checkedFrom = _checkedFrom;
      if (checkedAt != null && checkedFrom != null) {
        final fresh = DateTime.now().difference(checkedAt) < recheckAfter;
        if (fresh && haversineM(checkedFrom, at) < recheckDistanceM) {
          return null;
        }
      }
      _checkedAt = DateTime.now();
      _checkedFrom = at;
    }

    _loading = true;
    try {
      final catalogue = _catalogue ??= await _loadCatalogue();
      final region = catalogue.regionAt(at);
      if (region == null) {
        AppLog.d('nav', 'no region found for $at');
        return null;
      }
      if (region.id == _map.regionId) return null;
      if (_settings.isRegionSnoozed(region.id)) return null;
      final saved = await _map.hasArchivedRegion(region.id);
      return RegionSuggestion(region, region.localLabel(catalogue.byId),
          saved: saved, reason: reason);
    } on Object catch (e) {
      // Offline, a truncated catalogue, a server having a bad day: none of
      // these are worth a dialog. The generic "no map data" path still works.
      AppLog.w('nav', 'region lookup failed: $e');
      return null;
    } finally {
      _loading = false;
    }
  }
}
