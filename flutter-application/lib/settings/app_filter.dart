import 'package:shared_preferences/shared_preferences.dart';

/// Per-package allowlist persisted via SharedPreferences.
///
/// Inverted from the v1 default: nothing forwards until the user checks
/// it in the filter screen. Stops the head unit drowning in spam from
/// every app on the phone the moment Notification Access is granted.
///
/// Key bumped to v2 so a stale v1 "disabled" list (opposite semantics)
/// doesn't get misinterpreted as an allowlist on upgrade.
class AppFilter {
  static const _key = 'enabled_packages_v2';

  final SharedPreferences _prefs;
  AppFilter._(this._prefs);

  static Future<AppFilter> load() async {
    final p = await SharedPreferences.getInstance();
    return AppFilter._(p);
  }

  Set<String> get enabled => (_prefs.getStringList(_key) ?? const []).toSet();

  bool allows(String package) => enabled.contains(package);

  Future<void> setEnabled(String package, bool on) async {
    final s = enabled;
    if (on) {
      s.add(package);
    } else {
      s.remove(package);
    }
    await _prefs.setStringList(_key, s.toList()..sort());
  }
}
