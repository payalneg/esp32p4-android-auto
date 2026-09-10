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

  static const _kProfile = 'nav_profile_v1';
  static const _kTileCapMb = 'nav_tile_cap_mb_v1';
  static const _kCorridorMaxTiles = 'nav_corridor_max_tiles_v1';
  static const _kLastView = 'nav_last_view_v1';

  RideProfile _profile = kDefaultProfile;
  int _tileCapMb = 300;

  /// 250 is the bulk-download line in the OSM tile usage policy; the screen
  /// offers more for long routes, as the user's own call.
  int _corridorMaxTiles = 250;
  String _lastView = '';
  bool _loaded = false;

  RideProfile get profile => _profile;
  int get tileCapMb => _tileCapMb;
  int get corridorMaxTiles => _corridorMaxTiles;
  bool get loaded => _loaded;

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
    _lastView = p.getString(_kLastView) ?? '';
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

  /// Saved on every meaningful camera move; not a notifying change, since
  /// nothing rebuilds on it.
  Future<void> saveLastView(double lat, double lon, double zoom) async {
    _lastView = '$lat,$lon,$zoom';
    final p = await SharedPreferences.getInstance();
    await p.setString(_kLastView, _lastView);
  }
}
