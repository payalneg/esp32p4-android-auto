/// Preferences for the offline navigator.
///
/// Ordinary preferences, so SharedPreferences with the project's `_v1`-suffixed
/// key convention (see settings/agent_settings.dart). A [ChangeNotifier]
/// because the map screen and the map-data screen both read the profile and
/// have to redraw when the other one changes it.
library;

import 'package:flutter/foundation.dart';
import 'package:shared_preferences/shared_preferences.dart';

import '../nav/way_classes.dart';

class NavSettings extends ChangeNotifier {
  NavSettings._();
  static final NavSettings instance = NavSettings._();

  /// A settings object with defaults and no singleton behind it, for tests
  /// that need to flip a preference without touching the rider's real one.
  @visibleForTesting
  NavSettings.forTest();

  static const _kProfile = 'nav_profile_v1';
  static const _kTileCapMb = 'nav_tile_cap_mb_v1';
  static const _kCorridorMaxTiles = 'nav_corridor_max_tiles_v1';
  static const _kAreaRadiusKm = 'nav_area_radius_km_v1';
  static const _kTileBudget = 'nav_tile_budget_v1';
  static const _kLastView = 'nav_last_view_v1';
  static const _kVoice = 'nav_voice_v1';
  static const _kHaptics = 'nav_haptics_v1';
  static const _kTrackUp = 'nav_track_up_v1';
  static const _kSimulator = 'nav_simulator_v1';
  static const _kStreamToDisplay = 'nav_stream_v1';
  static const _kAutoRegion = 'nav_auto_region_v1';
  static const _kRegionSnooze = 'nav_region_snooze_v1';

  /// Whether the phone draws its own map from the offline OSM geometry
  /// instead of downloading OpenStreetMap raster tiles.
  ///
  /// Public because the foreground-service isolate builds its own tile cache
  /// and has to read the same switch without a second settings object.
  static const kUseGraphTiles = 'nav_graph_tiles_v1';

  /// The raster map is the familiar one and needs no downloaded geometry to
  /// look right, so it is what a fresh install (and an upgraded one that never
  /// expressed a preference) gets.
  static const defaultUseGraphTiles = false;

  /// How long "Not now" keeps a region quiet. Long enough not to nag a rider
  /// who just rode through, short enough to offer again on a later trip.
  static const Duration regionSnoozeFor = Duration(days: 3);

  RideProfile _profile = kDefaultProfile;
  int _tileCapMb = 300;

  /// 250 is the bulk-download line in the OSM tile usage policy; the screen
  /// offers more for long routes, as the user's own call.
  int _corridorMaxTiles = 250;
  /// How far around the rider an offline area reaches. Two kilometres is a
  /// town centre and a few thousand tiles across all the zoom levels;
  /// five is a whole city and takes a while.
  double _areaRadiusKm = 2;

  /// Ceiling on the tiles one saved area may cost. The pyramid is ordered
  /// so the cut lands on fine detail at the edges, never on the overview.
  int _tileBudget = 3000;
  String _lastView = '';

  /// Spoken turn instructions while navigating. Off unless asked for: the
  /// phone is not what the rider looks at or listens to — the head unit is.
  bool _voice = false;

  /// A buzz before each turn. Off by default, same reasoning.
  bool _haptics = false;

  /// Rotate the map so the direction of travel is up while navigating.
  bool _trackUp = true;

  /// Show the ▶ button that rides the route without a GPS. A developer's
  /// tool; hidden unless switched on.
  bool _simulator = false;

  /// Send the navigator picture to the head unit while it is showing its
  /// navigator screen. On by default — unlike voice and vibration, this is
  /// the point of the feature, and nothing is sent unless the head unit says
  /// it is showing that screen.
  bool _streamToDisplay = true;

  /// Offer the regional map for where the rider is, when no downloaded map
  /// covers it. On by default: this is the one download a navigator cannot do
  /// without, and a rider who has just arrived should not have to know that
  /// Geofabrik exists to get a map.
  bool _autoRegion = true;

  /// Draw the map from the offline graph geometry rather than OSM raster
  /// tiles. Off by default: raster tiles are what the map looked like before
  /// the graph renderer existed, and a toggle makes the choice reversible.
  bool _useGraphTiles = defaultUseGraphTiles;

  /// The region a "Not now" was given to, as `id|millisecondsSinceEpoch`.
  String _regionSnooze = '';
  bool _loaded = false;

  RideProfile get profile => _profile;
  int get tileCapMb => _tileCapMb;
  int get corridorMaxTiles => _corridorMaxTiles;
  double get areaRadiusKm => _areaRadiusKm;
  int get tileBudget => _tileBudget;
  bool get voice => _voice;
  bool get haptics => _haptics;
  bool get trackUp => _trackUp;
  bool get simulator => _simulator;
  bool get streamToDisplay => _streamToDisplay;
  bool get autoRegion => _autoRegion;
  bool get useGraphTiles => _useGraphTiles;
  bool get loaded => _loaded;

  /// Whether [regionId] was waved away recently enough to stay quiet.
  bool isRegionSnoozed(String regionId) {
    final at = _regionSnooze.lastIndexOf('|');
    if (at <= 0) return false;
    final id = _regionSnooze.substring(0, at);
    final ms = int.tryParse(_regionSnooze.substring(at + 1));
    if (id != regionId || ms == null) return false;
    final age = DateTime.now().millisecondsSinceEpoch - ms;
    return age >= 0 && age < regionSnoozeFor.inMilliseconds;
  }

  /// Last map position as `lat,lon,zoom`, or null if never saved.
  (double lat, double lon, double zoom)? get lastView {
    final parts = _lastView.split(',');
    if (parts.length != 3) return null;
    final lat = double.tryParse(parts[0]);
    final lon = double.tryParse(parts[1]);
    final zoom = double.tryParse(parts[2]);
    if (lat == null || lon == null || zoom == null) return null;
    return (lat, lon, zoom);
  }

  Future<void> load() async {
    final p = await SharedPreferences.getInstance();
    _profile = RideProfile.byName(p.getString(_kProfile) ?? '') ?? kDefaultProfile;
    _tileCapMb = p.getInt(_kTileCapMb) ?? 300;
    _corridorMaxTiles = p.getInt(_kCorridorMaxTiles) ?? 250;
    _areaRadiusKm = p.getDouble(_kAreaRadiusKm) ?? 2;
    _tileBudget = p.getInt(_kTileBudget) ?? 3000;
    _lastView = p.getString(_kLastView) ?? '';
    _voice = p.getBool(_kVoice) ?? false;
    _haptics = p.getBool(_kHaptics) ?? false;
    _trackUp = p.getBool(_kTrackUp) ?? true;
    _simulator = p.getBool(_kSimulator) ?? false;
    _streamToDisplay = p.getBool(_kStreamToDisplay) ?? true;
    _autoRegion = p.getBool(_kAutoRegion) ?? true;
    _useGraphTiles = p.getBool(kUseGraphTiles) ?? defaultUseGraphTiles;
    _regionSnooze = p.getString(_kRegionSnooze) ?? '';
    _loaded = true;
    notifyListeners();
  }

  Future<void> setProfile(RideProfile value) async {
    if (value == _profile) return;
    _profile = value;
    notifyListeners();
    final p = await SharedPreferences.getInstance();
    await p.setString(_kProfile, value.name);
  }

  Future<void> setTileCapMb(int value) async {
    if (value == _tileCapMb) return;
    _tileCapMb = value;
    notifyListeners();
    final p = await SharedPreferences.getInstance();
    await p.setInt(_kTileCapMb, value);
  }

  Future<void> setCorridorMaxTiles(int value) async {
    if (value == _corridorMaxTiles) return;
    _corridorMaxTiles = value;
    notifyListeners();
    final p = await SharedPreferences.getInstance();
    await p.setInt(_kCorridorMaxTiles, value);
  }

  Future<void> setAreaRadiusKm(double value) async {
    if (value == _areaRadiusKm) return;
    _areaRadiusKm = value;
    notifyListeners();
    final p = await SharedPreferences.getInstance();
    await p.setDouble(_kAreaRadiusKm, value);
  }

  Future<void> setTileBudget(int value) async {
    if (value == _tileBudget) return;
    _tileBudget = value;
    notifyListeners();
    final p = await SharedPreferences.getInstance();
    await p.setInt(_kTileBudget, value);
  }

  Future<void> setVoice(bool value) async {
    if (value == _voice) return;
    _voice = value;
    notifyListeners();
    final p = await SharedPreferences.getInstance();
    await p.setBool(_kVoice, value);
  }

  Future<void> setHaptics(bool value) async {
    if (value == _haptics) return;
    _haptics = value;
    notifyListeners();
    final p = await SharedPreferences.getInstance();
    await p.setBool(_kHaptics, value);
  }

  Future<void> setTrackUp(bool value) async {
    if (value == _trackUp) return;
    _trackUp = value;
    notifyListeners();
    final p = await SharedPreferences.getInstance();
    await p.setBool(_kTrackUp, value);
  }

  Future<void> setSimulator(bool value) async {
    if (value == _simulator) return;
    _simulator = value;
    notifyListeners();
    final p = await SharedPreferences.getInstance();
    await p.setBool(_kSimulator, value);
  }

  Future<void> setStreamToDisplay(bool value) async {
    if (value == _streamToDisplay) return;
    _streamToDisplay = value;
    notifyListeners();
    final p = await SharedPreferences.getInstance();
    await p.setBool(_kStreamToDisplay, value);
  }

  /// Saved on every meaningful camera move; not a notifying change, since
  /// nothing rebuilds on it.
  Future<void> saveLastView(double lat, double lon, double zoom) async {
    _lastView = '$lat,$lon,$zoom';
    final p = await SharedPreferences.getInstance();
    await p.setString(_kLastView, _lastView);
  }

  Future<void> setAutoRegion(bool value) async {
    if (value == _autoRegion) return;
    _autoRegion = value;
    notifyListeners();
    final p = await SharedPreferences.getInstance();
    await p.setBool(_kAutoRegion, value);
  }

  Future<void> setUseGraphTiles(bool value) async {
    if (value == _useGraphTiles) return;
    _useGraphTiles = value;
    notifyListeners();
    final p = await SharedPreferences.getInstance();
    await p.setBool(kUseGraphTiles, value);
  }

  /// "Not now" on a region offer. Persisted, because the point is to stop the
  /// offer coming back on the next launch from the same place.
  Future<void> snoozeRegion(String regionId) async {
    _regionSnooze =
        '$regionId|${DateTime.now().millisecondsSinceEpoch}';
    final p = await SharedPreferences.getInstance();
    await p.setString(_kRegionSnooze, _regionSnooze);
  }

  Future<void> clearRegionSnooze() async {
    if (_regionSnooze.isEmpty) return;
    _regionSnooze = '';
    final p = await SharedPreferences.getInstance();
    await p.remove(_kRegionSnooze);
  }
}
