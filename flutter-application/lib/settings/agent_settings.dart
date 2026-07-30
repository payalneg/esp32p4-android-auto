/// Settings for the AI assistant.
///
/// The API key goes to [FlutterSecureStorage] (Android Keystore / iOS
/// Keychain) — SharedPreferences is a plain XML file in app-private storage
/// and, with `allowBackup` unset, would ride along into adb and cloud backups.
/// Everything else is an ordinary preference and follows the project's
/// `_v1`-suffixed key convention (see settings/app_filter.dart).
///
/// A [ChangeNotifier] rather than a bare singleton because the editor's
/// Assistant tab appears and disappears with the key being set.
library;

import 'package:flutter/foundation.dart';
import 'package:flutter_secure_storage/flutter_secure_storage.dart';
import 'package:shared_preferences/shared_preferences.dart';

import '../agent/ai_models.dart';
import '../agent/embedded_key.dart';

class AgentSettings extends ChangeNotifier {
  AgentSettings._();
  static final AgentSettings instance = AgentSettings._();

  static const _kApiKey = 'agent_api_key_v1'; // secure storage
  static const _kRemember = 'agent_remember_key_v1';
  static const _kProvider = 'agent_provider_v1';
  static const _kBaseUrl = 'agent_base_url_v1';
  static const _kStrict = 'agent_strict_v1';
  static const _kModel = 'agent_model_v1';
  static const _kTemperature = 'agent_temperature_v1';
  static const _kThinking = 'agent_thinking_v1';
  static const _kMaxSteps = 'agent_max_steps_v1';
  static const _kMaxFlashes = 'agent_max_flashes_v1';
  static const _kSpendCap = 'agent_spend_cap_v1';
  static const _kSpendTotal = 'agent_spend_total_v1';
  static const _kDisclaimer = 'agent_disclaimer_ack_v1';

  static const _secure = FlutterSecureStorage(
    aOptions: AndroidOptions(encryptedSharedPreferences: true),
  );

  String? _apiKey;
  bool _remember = true;
  AiProvider _provider = kDefaultProvider;
  String _baseUrl = '';
  bool _strict = true;
  String _model = '';
  double _temperature = 0;
  bool _thinking = false;
  int _maxSteps = 24;
  int _maxFlashes = 6;
  double _spendCap = 0.50;
  double _spendTotal = 0;
  bool _disclaimerAck = false;
  bool _loaded = false;

  /// Whether the user has acknowledged that the assistant writes code for a
  /// motor controller and can be wrong. Gates the whole chat tab once.
  bool get disclaimerAcknowledged => _disclaimerAck;

  Future<void> acknowledgeDisclaimer() async {
    _disclaimerAck = true;
    await _putBool(_kDisclaimer, true);
  }

  bool get loaded => _loaded;

  /// The key to use: what the user typed, else the one baked into the build
  /// (see embedded_key.dart). A user-entered key always wins, so a build-time
  /// key can be overridden without rebuilding.
  ///
  /// In-memory only when [rememberKey] is false — the user re-enters it each
  /// session and it never touches storage.
  String? get apiKey {
    final own = (_apiKey ?? '').trim();
    return own.isNotEmpty ? own : embeddedApiKey;
  }

  bool get configured => (apiKey ?? '').trim().isNotEmpty;

  /// True when the app is running on a key compiled into the build rather than
  /// one the user entered. The settings screen says so out loud — an embedded
  /// key is recoverable from the APK.
  bool get usingEmbeddedKey =>
      (_apiKey ?? '').trim().isEmpty && embeddedApiKey != null;

  bool get rememberKey => _remember;

  AiProvider get provider => _provider;

  /// Empty means "the provider's default" — kept that way so switching
  /// provider moves the endpoint with it instead of stranding a stale URL.
  String get baseUrl =>
      _baseUrl.isEmpty ? _provider.defaultBaseUrl : _baseUrl;
  bool get baseUrlIsDefault => _baseUrl.isEmpty;
  bool get strict => _strict;
  String get model => _model.isEmpty ? _provider.defaultModel : _model;
  String get strongModel => _provider.strongModel;
  double get temperature => _temperature;
  bool get thinking => _thinking;
  int get maxSteps => _maxSteps;
  int get maxFlashes => _maxFlashes;

  /// Per-session spend ceiling in USD; 0 disables the cap.
  double get spendCap => _spendCap;

  /// Lifetime spend, informational.
  double get spendTotal => _spendTotal;

  /// Local fallback price table entry; null when the provider reports cost
  /// itself (OpenRouter) or the model is unknown.
  ModelPricing? get pricing => kPricing[model];

  Future<void> load() async {
    final p = await SharedPreferences.getInstance();
    _remember = p.getBool(_kRemember) ?? true;
    _provider = AiProvider.values.firstWhere(
        (e) => e.name == p.getString(_kProvider),
        orElse: () => kDefaultProvider);
    _baseUrl = p.getString(_kBaseUrl) ?? '';
    _strict = p.getBool(_kStrict) ?? true;
    _model = p.getString(_kModel) ?? '';
    _temperature = p.getDouble(_kTemperature) ?? 0;
    _thinking = p.getBool(_kThinking) ?? false;
    _maxSteps = p.getInt(_kMaxSteps) ?? 24;
    _maxFlashes = p.getInt(_kMaxFlashes) ?? 6;
    _spendCap = p.getDouble(_kSpendCap) ?? 0.50;
    _spendTotal = p.getDouble(_kSpendTotal) ?? 0;
    _disclaimerAck = p.getBool(_kDisclaimer) ?? false;
    if (_remember) {
      try {
        _apiKey = await _secure.read(key: _kApiKey);
      } catch (e) {
        // A Keystore that can't be read (restored backup, changed lock screen)
        // must not take the app down — the user just re-enters the key.
        debugPrint('[agent] secure storage read failed: $e');
        _apiKey = null;
      }
    }
    _loaded = true;
    notifyListeners();
  }

  Future<void> setApiKey(String? key) async {
    final v = (key ?? '').trim();
    _apiKey = v.isEmpty ? null : v;
    if (_remember) {
      try {
        if (_apiKey == null) {
          await _secure.delete(key: _kApiKey);
        } else {
          await _secure.write(key: _kApiKey, value: _apiKey);
        }
      } catch (e) {
        debugPrint('[agent] secure storage write failed: $e');
      }
    }
    notifyListeners();
  }

  Future<void> setRememberKey(bool on) async {
    _remember = on;
    final p = await SharedPreferences.getInstance();
    await p.setBool(_kRemember, on);
    try {
      if (on) {
        if (_apiKey != null) await _secure.write(key: _kApiKey, value: _apiKey);
      } else {
        await _secure.delete(key: _kApiKey);
      }
    } catch (e) {
      debugPrint('[agent] secure storage update failed: $e');
    }
    notifyListeners();
  }

  /// Switching provider resets the endpoint and model to that provider's
  /// defaults — an OpenRouter model id is meaningless to api.deepseek.com and
  /// vice versa. The key is NOT cleared: it is the user's to manage.
  Future<void> setProvider(AiProvider v) async {
    if (_provider == v) return;
    _provider = v;
    _baseUrl = '';
    _model = '';
    final p = await SharedPreferences.getInstance();
    await p.setString(_kProvider, v.name);
    await p.remove(_kBaseUrl);
    await p.remove(_kModel);
    notifyListeners();
  }

  /// Empty resets to the provider default.
  Future<void> setBaseUrl(String v) async {
    _baseUrl = v.trim();
    await _putString(_kBaseUrl, _baseUrl);
  }

  Future<void> setStrict(bool v) async {
    _strict = v;
    await _putBool(_kStrict, v);
  }

  Future<void> setModel(String v) async {
    _model = v.trim();
    await _putString(_kModel, _model);
  }

  Future<void> setTemperature(double v) async {
    _temperature = v.clamp(0, 2);
    await _putDouble(_kTemperature, _temperature);
  }

  Future<void> setThinking(bool v) async {
    _thinking = v;
    await _putBool(_kThinking, v);
  }

  Future<void> setMaxSteps(int v) async {
    _maxSteps = v.clamp(1, 100);
    await _putInt(_kMaxSteps, _maxSteps);
  }

  Future<void> setMaxFlashes(int v) async {
    _maxFlashes = v.clamp(0, 50);
    await _putInt(_kMaxFlashes, _maxFlashes);
  }

  Future<void> setSpendCap(double v) async {
    _spendCap = v < 0 ? 0 : v;
    await _putDouble(_kSpendCap, _spendCap);
  }

  /// Accumulate what a finished session cost. Best-effort: a lost update here
  /// only skews an informational number.
  Future<void> addSpend(double usd) async {
    if (usd <= 0) return;
    _spendTotal += usd;
    await _putDouble(_kSpendTotal, _spendTotal);
  }

  Future<void> _putString(String k, String v) async {
    final p = await SharedPreferences.getInstance();
    await p.setString(k, v);
    notifyListeners();
  }

  Future<void> _putBool(String k, bool v) async {
    final p = await SharedPreferences.getInstance();
    await p.setBool(k, v);
    notifyListeners();
  }

  Future<void> _putInt(String k, int v) async {
    final p = await SharedPreferences.getInstance();
    await p.setInt(k, v);
    notifyListeners();
  }

  Future<void> _putDouble(String k, double v) async {
    final p = await SharedPreferences.getInstance();
    await p.setDouble(k, v);
    notifyListeners();
  }
}
