import 'package:shared_preferences/shared_preferences.dart';

/// Per-package whitelist persisted via SharedPreferences.
///
/// Default policy: forward everything unless explicitly disabled. The UI
/// shows a checkbox per installed app; unchecked packages land in the
/// disabled set.
class AppFilter {
  static const _key = 'disabled_packages_v1';

  final SharedPreferences _prefs;
  AppFilter._(this._prefs);

  static Future<AppFilter> load() async {
    final p = await SharedPreferences.getInstance();
    return AppFilter._(p);
  }

  Set<String> get disabled => (_prefs.getStringList(_key) ?? const []).toSet();

  bool allows(String package) => !disabled.contains(package);

  Future<void> setEnabled(String package, bool enabled) async {
    final s = disabled;
    if (enabled) {
      s.remove(package);
    } else {
      s.add(package);
    }
    await _prefs.setStringList(_key, s.toList()..sort());
  }
}
