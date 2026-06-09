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
  'fw.detected': 'Detected device',
  'fw.detected.unknown': 'Unknown',
  'fw.select': 'Firmware to flash',
  'fw.warn.undetected':
      'Couldn\'t read the head unit\'s board — pick the right firmware manually.',
  'fw.warn.mismatch':
      'Selected firmware is for a different board than detected — flash only if you\'re sure.',
  'fw.bundled': 'Bundled in app',
  'fw.unsupported': 'This head unit does not support over-the-air updates from the app.',
  'fw.disconnected': 'Connect to the head unit over Bluetooth first.',
  'fw.reading': 'Reading device info…',
  'fw.uptodate': 'Head unit is already on this version.',
  'fw.flash': 'Flash firmware',
  'fw.flashing': 'Updating…',
  'fw.warn.title': 'Flash firmware?',
  'fw.warn.body': 'The firmware bundled in the app will be uploaded to the head unit. Keep it powered the whole time — it reboots when done.',
  'fw.host': 'Device address',
  'fw.host.note': 'Make sure the phone is on the head unit\'s WiFi.',
  'fw.method': 'Update method',
  'fw.method.ble': 'Bluetooth',
  'fw.method.wifi': 'WiFi',
  'fw.method.ble.note':
      'Flashes over the existing Bluetooth link — no WiFi needed. Slower (a few minutes); keep the app open.',
  'fw.method.ble.unsupported':
      'This head unit\'s firmware can\'t flash over Bluetooth — update over WiFi.',
  'fw.warn.go': 'Flash',
  'fw.cancel': 'Cancel',
  'fw.done': 'Firmware uploaded — the head unit is rebooting.',
  // Firmware-update status/error messages (emitted as keys by the updater so
  // they localise; {size}/{host}/{err} are substituted at display time).
  'fw.ota.uploading.wifi': 'Uploading firmware ({size} KB)…',
  'fw.ota.uploading.ble': 'Sending firmware over Bluetooth ({size} KB)…',
  'fw.ota.verifying': 'Verifying image…',
  'fw.ota.verifying.device': 'Verifying image on the device…',
  'fw.ota.rejected': 'The device rejected the firmware',
  'fw.ota.connfail': 'Couldn\'t connect to {host}: {err}',
  'fw.ota.blefail': 'Bluetooth update failed',
  'fw.ota.bleerror': 'Bluetooth update error: {err}',
  'fw.ota.err.noconn': 'No Bluetooth connection to the head unit',
  'fw.ota.err.badsha': 'Invalid image checksum',
  'fw.ota.err.linklost': 'Bluetooth connection lost',
  'fw.ota.err.timeout': 'The device didn\'t respond in time',
  'fw.ota.err.nopart': 'No spare update partition on the device',
  'fw.ota.err.size': 'The image doesn\'t fit the device partition',
  'fw.ota.err.alloc': 'The device ran out of memory',
  'fw.ota.err.sha': 'Checksum error — the transfer was corrupted, retry',
  'fw.ota.err.begin': 'Couldn\'t prepare the device flash',
  'fw.ota.err.write': 'Device flash write error',
  'fw.ota.err.end': 'The image failed verification on the device',
  'fw.ota.err.boot': 'Couldn\'t switch the boot partition',
  'fw.ota.err.proto': 'Transfer failed — retry the update',
  'fw.ota.err.unknown': 'Device error',
  'fw.manual.hint': 'Couldn\'t read WiFi credentials from the head unit — enter them manually.',
  'fw.manual.ssid': 'Head unit WiFi name (SSID)',
  'fw.manual.password': 'Head unit WiFi password',
  'fw.manual.needfields': 'Enter the WiFi name and password.',
  'fw.manual.current': 'Phone is on WiFi: ',
  'fw.manual.current.none': 'not connected to WiFi',
  'fw.manual.scan': 'Scan',
  'fw.manual.pick': 'Select head unit WiFi',
  'fw.manual.noscan': 'No networks found — is location enabled?',

  'home.files.title': 'Device files',
  'files.title': 'Device files',
  'files.unsupported': 'This head unit firmware does not support file browsing.',
  'files.empty': 'Empty folder',
  'files.loading': 'Loading…',
  'files.dir': 'Folder',
  'files.notready': 'Storage is being prepared (first boot formats it, ~1 min).',
  'files.notready.retry': 'Retry',
  'files.upload': 'Upload file',
  'files.uploading': 'Uploading…',
  'files.mkdir': 'New folder',
  'files.mkdir.name': 'Folder name',
  'files.download': 'Download',
  'files.downloading': 'Downloading…',
  'files.saved': 'Saved to: ',
  'files.rename': 'Rename',
  'files.rename.to': 'New name',
  'files.delete': 'Delete',
  'files.delete.confirm': 'Permanently delete?',
  'files.cancel': 'Cancel',
  'files.ok': 'OK',
  'files.truncated': 'Listing truncated — too many entries.',
  'files.err.noconn': 'Not connected to head unit.',
  'files.err.notready': 'Storage is still mounting — try again shortly.',
  'files.err.badpath': 'Invalid path.',
  'files.err.noent': 'No such file or folder.',
  'files.err.notdir': 'Not a folder.',
  'files.err.isdir': 'That is a folder.',
  'files.err.nospc': 'Not enough free space on the device.',
  'files.err.exist': 'Already exists.',
  'files.err.io': 'I/O error.',
  'files.err.sha': 'Checksum mismatch — transfer corrupted.',
  'files.err.proto': 'Protocol error.',
  'files.err.alloc': 'Out of memory on the device.',
  'files.err.busy': 'Device is busy with another operation.',
  'files.err.toobig': 'File is too large for the device storage.',
  'files.err.unknown': 'Unknown error.',
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
  'fw.detected': 'Определённое устройство',
  'fw.detected.unknown': 'Не определено',
  'fw.select': 'Прошивка для заливки',
  'fw.warn.undetected':
      'Не удалось определить плату устройства — выбери нужную прошивку вручную.',
  'fw.warn.mismatch':
      'Выбранная прошивка для другой платы, чем определена — прошивай только если уверен.',
  'fw.bundled': 'В приложении',
  'fw.unsupported': 'Это устройство не поддерживает обновление прошивки из приложения.',
  'fw.disconnected': 'Сначала подключитесь к устройству по Bluetooth.',
  'fw.reading': 'Чтение информации с устройства…',
  'fw.uptodate': 'На устройстве уже эта версия.',
  'fw.flash': 'Прошить',
  'fw.flashing': 'Обновление…',
  'fw.warn.title': 'Прошить устройство?',
  'fw.warn.body': 'Прошивка, зашитая в приложение, будет залита на устройство. Не выключай его до конца — по завершении оно перезагрузится.',
  'fw.host': 'Адрес устройства',
  'fw.host.note': 'Убедись, что телефон в WiFi устройства.',
  'fw.method': 'Способ обновления',
  'fw.method.ble': 'Bluetooth',
  'fw.method.wifi': 'WiFi',
  'fw.method.ble.note':
      'Прошивка по уже установленному Bluetooth — WiFi не нужен. Медленнее (несколько минут); не закрывай приложение.',
  'fw.method.ble.unsupported':
      'Прошивка устройства не умеет обновляться по Bluetooth — обнови по WiFi.',
  'fw.warn.go': 'Прошить',
  'fw.cancel': 'Отмена',
  'fw.done': 'Прошивка залита — устройство перезагружается.',
  'fw.ota.uploading.wifi': 'Загрузка прошивки ({size} КБ)…',
  'fw.ota.uploading.ble': 'Передача прошивки по Bluetooth ({size} КБ)…',
  'fw.ota.verifying': 'Проверка образа…',
  'fw.ota.verifying.device': 'Проверка образа на устройстве…',
  'fw.ota.rejected': 'Устройство отклонило прошивку',
  'fw.ota.connfail': 'Не удалось подключиться к {host}: {err}',
  'fw.ota.blefail': 'Не удалось обновить по Bluetooth',
  'fw.ota.bleerror': 'Ошибка Bluetooth-обновления: {err}',
  'fw.ota.err.noconn': 'Нет Bluetooth-соединения с устройством',
  'fw.ota.err.badsha': 'Некорректная контрольная сумма образа',
  'fw.ota.err.linklost': 'Bluetooth-соединение потеряно',
  'fw.ota.err.timeout': 'Устройство не ответило вовремя',
  'fw.ota.err.nopart': 'На устройстве нет свободного раздела для обновления',
  'fw.ota.err.size': 'Образ не помещается в раздел устройства',
  'fw.ota.err.alloc': 'На устройстве не хватило памяти',
  'fw.ota.err.sha': 'Ошибка контрольной суммы — передача повредилась, повторите',
  'fw.ota.err.begin': 'Не удалось подготовить flash устройства',
  'fw.ota.err.write': 'Ошибка записи во flash устройства',
  'fw.ota.err.end': 'Образ не прошёл проверку на устройстве',
  'fw.ota.err.boot': 'Не удалось переключить загрузочный раздел',
  'fw.ota.err.proto': 'Сбой передачи — повторите обновление',
  'fw.ota.err.unknown': 'Ошибка устройства',
  'fw.manual.hint': 'Не удалось получить данные WiFi с устройства — введите вручную.',
  'fw.manual.ssid': 'Имя WiFi устройства (SSID)',
  'fw.manual.password': 'Пароль WiFi устройства',
  'fw.manual.needfields': 'Введите имя и пароль WiFi.',
  'fw.manual.current': 'Телефон в WiFi: ',
  'fw.manual.current.none': 'не подключён к WiFi',
  'fw.manual.scan': 'Поиск',
  'fw.manual.pick': 'Выберите WiFi устройства',
  'fw.manual.noscan': 'Сети не найдены — включена ли геолокация?',

  'home.files.title': 'Файлы устройства',
  'files.title': 'Файлы устройства',
  'files.unsupported': 'Прошивка этой магнитолы не поддерживает просмотр файлов.',
  'files.empty': 'Папка пуста',
  'files.loading': 'Загрузка…',
  'files.dir': 'Папка',
  'files.notready': 'Хранилище готовится (при первом запуске форматируется, ~1 мин).',
  'files.notready.retry': 'Повторить',
  'files.upload': 'Загрузить файл',
  'files.uploading': 'Загрузка…',
  'files.mkdir': 'Новая папка',
  'files.mkdir.name': 'Имя папки',
  'files.download': 'Скачать',
  'files.downloading': 'Скачивание…',
  'files.saved': 'Сохранено в: ',
  'files.rename': 'Переименовать',
  'files.rename.to': 'Новое имя',
  'files.delete': 'Удалить',
  'files.delete.confirm': 'Удалить безвозвратно?',
  'files.cancel': 'Отмена',
  'files.ok': 'OK',
  'files.truncated': 'Список обрезан — слишком много записей.',
  'files.err.noconn': 'Нет связи с магнитолой.',
  'files.err.notready': 'Хранилище ещё монтируется — повторите чуть позже.',
  'files.err.badpath': 'Недопустимый путь.',
  'files.err.noent': 'Файл или папка не найдены.',
  'files.err.notdir': 'Это не папка.',
  'files.err.isdir': 'Это папка.',
  'files.err.nospc': 'Недостаточно свободного места на устройстве.',
  'files.err.exist': 'Уже существует.',
  'files.err.io': 'Ошибка ввода-вывода.',
  'files.err.sha': 'Несовпадение контрольной суммы — передача повреждена.',
  'files.err.proto': 'Ошибка протокола.',
  'files.err.alloc': 'Недостаточно памяти на устройстве.',
  'files.err.busy': 'Устройство занято другой операцией.',
  'files.err.toobig': 'Файл слишком велик для хранилища устройства.',
  'files.err.unknown': 'Неизвестная ошибка.',
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
