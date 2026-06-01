/// Tiny hand-rolled i18n. Two locales, no codegen, no ARB pipeline —
/// the app is small enough that a flat per-language map pays for itself
/// in build simplicity.
///
/// Usage:
///   t(context, 'home.title')
///
/// The active locale is held by [LocaleScope] above MaterialApp and
/// persisted via SharedPreferences. Default is English.
library;

import 'package:flutter/material.dart';
import 'package:shared_preferences/shared_preferences.dart';

const _en = <String, String>{
  'app.title': 'AA Bridge',

  'home.status.connected': 'Connected',
  'home.status.disconnected': 'Not connected',
  'home.state.idle': 'BLE idle',
  'home.state.scanning': 'Scanning…',
  'home.state.connecting': 'Connecting…',
  'home.state.connected': 'Link established',
  'home.state.disconnected':
      'Link lost — waiting for device to come back on air…',
  'home.forget': 'Forget device',
  'home.forget.confirm.title': 'Forget device?',
  'home.forget.confirm.body':
      'Auto-reconnect will stop. To use it again you\'ll have to pair from scratch.',
  'home.forget.cancel': 'Cancel',
  'home.forget.ok': 'Forget',

  'home.notif.title': 'Notification access',
  'home.notif.granted': 'Granted',
  'home.notif.denied': 'Allow it in system settings',
  'home.batt.title': 'No battery restrictions',
  'home.batt.granted': 'OS won\'t throttle the BLE link',
  'home.batt.denied':
      'Without this the OS kills BLE in the background after ~30 min',
  'home.pairing.title': 'Pair with head unit',
  'home.pairing.none': 'No device selected',
  'home.pairing.saved': 'Saved: ',
  'home.filter.title': 'Which apps to forward',
  'home.ios.title': 'iOS mode',
  'home.ios.body':
      'iOS sandbox blocks reading third-party notifications. Only pairing '
      'and the app\'s own media metadata work.',
  'home.lang.title': 'Language',

  'pairing.title': 'Find head unit',
  'pairing.empty':
      'Tap "refresh" to start scanning. The head unit must be nearby and '
      'advertising.',
  'pairing.error': 'Error: ',
  'pairing.unnamed': 'Unnamed',

  'filter.title': 'Apps',
  'filter.search': 'Search…',

  'test.notif.section': 'Notification test',
  'test.media.section': 'Media test',
  'test.notif.whatsapp': 'WhatsApp-style',
  'test.notif.telegram': 'Telegram-style',
  'test.notif.long': 'Long text',
  'test.not_connected': 'Not connected to head unit',
  'test.sent': 'Sent: ',
  'test.seek': '+30s',

  'lang.choose': 'Choose language',
  'lang.en': 'English',
  'lang.ru': 'Русский',

  'home.fw.title': 'Update head unit firmware',
  'home.about.title': 'About',
  'about.title': 'About',
  'about.app': 'App version',
  'about.fw': 'Bundled firmware',
  'about.desc': 'Bridges phone notifications, media and time to an ESP32-P4 head unit over BLE, and flashes its firmware over WiFi.',
  'about.source': 'Source code',
  'fw.title': 'Firmware update',
  'fw.device': 'Head unit version',
  'fw.bundled': 'Bundled in app',
  'fw.unsupported': 'This head unit does not support over-the-air updates from the app.',
  'fw.disconnected': 'Connect to the head unit over Bluetooth first.',
  'fw.reading': 'Reading device info…',
  'fw.uptodate': 'Head unit is already on this version.',
  'fw.flash': 'Flash firmware',
  'fw.flashing': 'Updating…',
  'fw.warn.title': 'Flash firmware?',
  'fw.warn.body': 'The phone will join the head unit\'s WiFi and upload the firmware. Keep the device powered the whole time — it reboots when done.',
  'fw.warn.go': 'Flash',
  'fw.cancel': 'Cancel',
  'fw.done': 'Firmware uploaded — the head unit is rebooting.',
  'fw.manual.hint': 'Couldn\'t read WiFi credentials from the head unit — enter them manually.',
  'fw.manual.ssid': 'Head unit WiFi name (SSID)',
  'fw.manual.password': 'Head unit WiFi password',
  'fw.manual.needfields': 'Enter the WiFi name and password.',
  'fw.manual.current': 'Phone is on WiFi: ',
  'fw.manual.current.none': 'not connected to WiFi',
  'fw.manual.scan': 'Scan',
  'fw.manual.pick': 'Select head unit WiFi',
  'fw.manual.noscan': 'No networks found — is location enabled?',
};

const _ru = <String, String>{
  'app.title': 'AA Bridge',

  'home.status.connected': 'Подключено',
  'home.status.disconnected': 'Не подключено',
  'home.state.idle': 'BLE простой',
  'home.state.scanning': 'Сканирую…',
  'home.state.connecting': 'Подключаюсь…',
  'home.state.connected': 'Соединение установлено',
  'home.state.disconnected':
      'Соединение потеряно, жду пока устройство появится в эфире…',
  'home.forget': 'Забыть устройство',
  'home.forget.confirm.title': 'Забыть устройство?',
  'home.forget.confirm.body':
      'Авто-подключение отключится. Чтобы снова подключиться, придётся пройти '
      'pairing заново.',
  'home.forget.cancel': 'Отмена',
  'home.forget.ok': 'Забыть',

  'home.notif.title': 'Доступ к уведомлениям',
  'home.notif.granted': 'Разрешено',
  'home.notif.denied': 'Нужно разрешить в системных настройках',
  'home.batt.title': 'Без энергосбережения',
  'home.batt.granted': 'ОС не будет душить BLE-соединение',
  'home.batt.denied':
      'Без этого ОС отключит BLE через ~30 мин в фоне',
  'home.pairing.title': 'Pairing с head unit',
  'home.pairing.none': 'Устройство не выбрано',
  'home.pairing.saved': 'Сохранено: ',
  'home.filter.title': 'Какие приложения транслировать',
  'home.ios.title': 'iOS режим',
  'home.ios.body':
      'На iOS уведомления приложений не пересылаются (sandbox). '
      'Доступно только pairing и media-метаданные собственного приложения.',
  'home.lang.title': 'Язык',

  'pairing.title': 'Поиск head unit',
  'pairing.empty':
      'Нажмите «обновить» чтобы начать сканирование. Head unit должен быть '
      'рядом и в режиме advertising.',
  'pairing.error': 'Ошибка: ',
  'pairing.unnamed': 'Безымянное',

  'filter.title': 'Приложения',
  'filter.search': 'Поиск…',

  'test.notif.section': 'Тест уведомлений',
  'test.media.section': 'Тест медиа',
  'test.notif.whatsapp': 'WhatsApp-стиль',
  'test.notif.telegram': 'Telegram-стиль',
  'test.notif.long': 'Длинный текст',
  'test.not_connected': 'Не подключено к head unit',
  'test.sent': 'Отправлено: ',
  'test.seek': '+30 сек',

  'lang.choose': 'Выбор языка',
  'lang.en': 'English',
  'lang.ru': 'Русский',

  'home.fw.title': 'Обновить прошивку устройства',
  'home.about.title': 'О приложении',
  'about.title': 'О приложении',
  'about.app': 'Версия приложения',
  'about.fw': 'Прошивка в комплекте',
  'about.desc': 'Мост уведомлений, медиа и времени с телефона на головное устройство ESP32-P4 по BLE; прошивка устройства по WiFi.',
  'about.source': 'Исходный код',
  'fw.title': 'Обновление прошивки',
  'fw.device': 'Версия на устройстве',
  'fw.bundled': 'В приложении',
  'fw.unsupported': 'Это устройство не поддерживает обновление прошивки из приложения.',
  'fw.disconnected': 'Сначала подключитесь к устройству по Bluetooth.',
  'fw.reading': 'Чтение информации с устройства…',
  'fw.uptodate': 'На устройстве уже эта версия.',
  'fw.flash': 'Прошить',
  'fw.flashing': 'Обновление…',
  'fw.warn.title': 'Прошить устройство?',
  'fw.warn.body': 'Телефон подключится к WiFi устройства и зальёт прошивку. Не выключайте устройство до конца — по завершении оно перезагрузится.',
  'fw.warn.go': 'Прошить',
  'fw.cancel': 'Отмена',
  'fw.done': 'Прошивка залита — устройство перезагружается.',
  'fw.manual.hint': 'Не удалось получить данные WiFi с устройства — введите вручную.',
  'fw.manual.ssid': 'Имя WiFi устройства (SSID)',
  'fw.manual.password': 'Пароль WiFi устройства',
  'fw.manual.needfields': 'Введите имя и пароль WiFi.',
  'fw.manual.current': 'Телефон в WiFi: ',
  'fw.manual.current.none': 'не подключён к WiFi',
  'fw.manual.scan': 'Поиск',
  'fw.manual.pick': 'Выберите WiFi устройства',
  'fw.manual.noscan': 'Сети не найдены — включена ли геолокация?',
};

const _tables = <String, Map<String, String>>{
  'en': _en,
  'ru': _ru,
};

const supportedLocales = [Locale('en'), Locale('ru')];

class LocaleNotifier extends ChangeNotifier {
  static const _prefKey = 'app_locale_v1';

  Locale _locale = const Locale('en');
  Locale get locale => _locale;

  Future<void> load() async {
    final p = await SharedPreferences.getInstance();
    final code = p.getString(_prefKey);
    if (code != null && _tables.containsKey(code)) {
      _locale = Locale(code);
    }
  }

  Future<void> set(Locale l) async {
    if (l == _locale) return;
    _locale = l;
    final p = await SharedPreferences.getInstance();
    await p.setString(_prefKey, l.languageCode);
    notifyListeners();
  }
}

class LocaleScope extends InheritedNotifier<LocaleNotifier> {
  const LocaleScope({
    super.key,
    required LocaleNotifier super.notifier,
    required super.child,
  });

  static LocaleNotifier of(BuildContext context) {
    final scope = context.dependOnInheritedWidgetOfExactType<LocaleScope>();
    assert(scope?.notifier != null, 'LocaleScope missing above MaterialApp');
    return scope!.notifier!;
  }
}

/// Returns the translation for [key] in the currently active locale.
/// Falls back to English if the key is missing in the active table,
/// then to the key itself so missing entries surface visibly.
String t(BuildContext context, String key) {
  final l = LocaleScope.of(context).locale.languageCode;
  return _tables[l]?[key] ?? _en[key] ?? key;
}
