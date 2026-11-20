import 'package:flutter/material.dart';
import '../ble/ble_proxy.dart';
import '../firmware/recovery.dart';

class RecoveryScreen extends StatefulWidget {
  const RecoveryScreen({super.key});
  @override
  State<RecoveryScreen> createState() => _RecoveryScreenState();
}

class _RecoveryScreenState extends State<RecoveryScreen> {
  RecoveryStatus? _status;
  bool _busy = true;
  String? _message;

  @override
  void initState() {
    super.initState();
    _load();
  }

  Future<void> _load() async {
    setState(() => _busy = true);
    try {
      final raw = await BleProxy.instance.readRecovery();
      if (!mounted) return;
      setState(() {
        _status = raw == null ? null : RecoveryStatus(raw);
        _message =
            raw == null
                ? 'Подключитесь к устройству с поддержкой восстановления.'
                : null;
      });
    } catch (e) {
      if (mounted) {
        setState(() => _message = 'Не удалось прочитать состояние: $e');
      }
    } finally {
      if (mounted) setState(() => _busy = false);
    }
  }

  Future<void> _command(int op, String title, String explanation) async {
    final status = _status;
    if (status == null) return;
    final go = await showDialog<bool>(
      context: context,
      builder:
          (ctx) => AlertDialog(
            title: Text(title),
            content: Text('$explanation\n\nУстройство перезапустится.'),
            actions: [
              TextButton(
                onPressed: () => Navigator.pop(ctx, false),
                child: const Text('Отмена'),
              ),
              FilledButton(
                onPressed: () => Navigator.pop(ctx, true),
                child: const Text('Продолжить'),
              ),
            ],
          ),
    );
    if (go != true || !mounted) return;
    setState(() => _busy = true);
    try {
      await BleProxy.instance.recoveryCommand(op, status.nonce);
      if (mounted) {
        setState(() {
          _status = null;
          _message =
              'Команда принята. Дождитесь переподключения и нажмите обновить.';
        });
      }
    } catch (e) {
      if (mounted) setState(() => _message = 'Ошибка восстановления: $e');
    } finally {
      if (mounted) setState(() => _busy = false);
    }
  }

  @override
  Widget build(BuildContext context) {
    final s = _status;
    return Scaffold(
      appBar: AppBar(
        title: const Text('Восстановление'),
        actions: [
          IconButton(
            onPressed: _busy ? null : _load,
            icon: const Icon(Icons.refresh),
          ),
        ],
      ),
      body: SafeArea(
        child: ListView(
          padding: const EdgeInsets.all(16),
          children: [
            if (_busy) const LinearProgressIndicator(),
            if (_message != null)
              Padding(padding: const EdgeInsets.all(8), child: Text(_message!)),
            if (s != null) ...[
          if (s.error != 0)
            Text('Команда не выполнена: 0x${s.error.toRadixString(16)}. Перезапуска не было.'),
              Text(
                s.recovery
                    ? 'Аварийный режим: экран отключён'
                    : 'Обычный режим',
              ),
              Text(
                'Аварийных запусков подряд: ${s.failures}\nСлот: ${s.slot}\nПричина перезапуска: ${s.reasonName}\nБезопасные настройки экрана: ${s.safe ? "да" : "нет"}',
              ),
              const SizedBox(height: 16),
              if (!s.recovery)
                FilledButton(
                  onPressed:
                      _busy
                          ? null
                          : () => _command(
                            1,
                            'Войти в восстановление?',
                            'Bluetooth запустится без интерфейса и управления двигателем.',
                          ),
                  child: const Text('Войти в восстановление'),
                ),
              if (s.recovery) ...[
                FilledButton(
                  onPressed:
                      _busy
                          ? null
                          : () => _command(
                            2,
                            'Сбросить настройки экрана?',
                            'Яркость, тема, заставка и виджеты получат безопасные значения. Пробег, поездки и настройки двигателя сохранятся. Старые настройки экрана будут сохранены для возврата.',
                          ),
                  child: const Text('Безопасные настройки экрана'),
                ),
                if (s.other.isNotEmpty)
                  OutlinedButton(
                    onPressed:
                        _busy
                            ? null
                            : () => _command(
                              3,
                              'Переключить прошивку?',
                              'Другой слот: ${s.other}. Будут применены безопасные настройки экрана. Наличие образа ещё не гарантирует его исправность.',
                            ),
                    child: Text('Другой слот: ${s.other}'),
                  ),
                OutlinedButton(
                  onPressed:
                      _busy
                          ? null
                          : () => _command(
                            5,
                            'Повторить запуск?',
                            'Запустить текущую прошивку с текущими настройками.',
                          ),
                  child: const Text('Повторить обычный запуск'),
                ),
                OutlinedButton(
                  onPressed:
                      _busy
                          ? null
                          : () => _command(
                            4,
                            'Вернуть настройки экрана?',
                            'Восстановить сохранённые настройки. Если зависание вызвано ими, оно может повториться.',
                          ),
                  child: const Text('Вернуть сохранённые настройки'),
                ),
                const Text(
                  'Здесь также доступна обычная прошивка по Bluetooth: вернитесь на экран обновления.',
                ),
              ],
            ],
          ],
        ),
      ),
    );
  }
}
