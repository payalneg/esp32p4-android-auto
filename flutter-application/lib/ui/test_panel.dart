import 'package:flutter/material.dart';

import '../ble/ble_service.dart';
import '../ble/messages.dart';
import '../cache/icon_hash.dart';
import '../i18n/strings.dart';
import 'test_assets.dart';

/// Sends fake notifications / media frames over BLE without involving
/// the system listener. Lets us verify the head unit's parser and LVGL
/// toast without waiting for a real WhatsApp ping.
class TestPanel extends StatefulWidget {
  const TestPanel({super.key});
  @override
  State<TestPanel> createState() => _TestPanelState();
}

class _TestPanelState extends State<TestPanel> {
  int _idSeed = 1;

  // Local media-mock state — pretends to be a player so the head unit
  // sees plausible title/artist/position transitions.
  static const _tracks = [
    (title: 'Sunset Drive', artist: 'Synthwave Bot', album: 'Bench Sessions',
        duration: 213000),
    (title: 'Neon Heart', artist: 'CRT King', album: 'Phosphor', duration: 187000),
    (title: 'Cold Boot', artist: 'JTAG / Yann', album: 'EPROM', duration: 245000),
  ];
  int _trackIdx = 0;
  bool _playing = false;
  int _positionMs = 0;

  bool get _connected =>
      BleService.instance.currentState == BleConnState.connected;

  Future<void> _sendNotif(NotificationMsg n) async {
    if (!_connected) return _toast(t(context, 'test.not_connected'));
    await BleService.instance.sendNotification(n);
    _toast('${t(context, 'test.sent')}${n.title}');
  }

  /// Renders a coloured-letter PNG on the fly and ships it with the
  /// notification. After this call the head unit has [color/letter]
  /// in its icon cache keyed by the returned hash — pass the hash to
  /// the next sendNotification so the toast picks it up instead of
  /// falling back to the first-letter placeholder.
  Future<int> _pushTestIcon(Color color, String letter) async {
    final png = await makeIconPng(color, letter);
    final hash = fnv1a32(png);
    await BleService.instance.sendIcon(IconMsg(hash: hash, png: png));
    return hash;
  }

  Future<void> _sendMedia() async {
    if (!_connected) return _toast(t(context, 'test.not_connected'));
    final tr = _tracks[_trackIdx];
    await BleService.instance.sendMedia(MediaMsg(
      title: tr.title,
      artist: tr.artist,
      album: tr.album,
      durationMs: tr.duration,
      positionMs: _positionMs,
      isPlaying: _playing,
      albumArtHash: 0,
      sourceApp: 'com.test.player',
    ));
  }

  void _toast(String s) {
    ScaffoldMessenger.of(context).showSnackBar(
      SnackBar(content: Text(s), duration: const Duration(seconds: 2)),
    );
  }

  Future<void> _testWhatsapp() async {
    if (!_connected) return _toast(t(context, 'test.not_connected'));
    final iconHash = await _pushTestIcon(const Color(0xFF25D366), 'W');
    return _sendNotif(NotificationMsg(
      id: _idSeed++,
      package: 'com.whatsapp',
      appName: 'WhatsApp',
      title: 'Аня',
      text: 'Где ты? Я приехала',
      postedAtMs: DateTime.now().millisecondsSinceEpoch,
      iconHash: iconHash,
    ));
  }

  Future<void> _testTelegram() async {
    if (!_connected) return _toast(t(context, 'test.not_connected'));
    final iconHash = await _pushTestIcon(const Color(0xFF229ED9), 'T');
    return _sendNotif(NotificationMsg(
      id: _idSeed++,
      package: 'org.telegram.messenger',
      appName: 'Telegram',
      title: 'Чат проекта',
      text: 'Серёжа: смотри, новая прошивка собралась',
      postedAtMs: DateTime.now().millisecondsSinceEpoch,
      iconHash: iconHash,
    ));
  }

  Future<void> _testLong() async {
    if (!_connected) return _toast(t(context, 'test.not_connected'));
    final iconHash = await _pushTestIcon(const Color(0xFF7C7C7C), '?');
    return _sendNotif(NotificationMsg(
      id: _idSeed++,
      package: 'com.test.long',
      appName: 'Тест длинный',
      title: 'Длинное название уведомления для проверки переноса',
      text:
          'А это сам текст: проверяем что LVGL label с LONG_DOT режимом '
          'обрезает строку, не ломая UI. Тут специально много букв.',
      postedAtMs: DateTime.now().millisecondsSinceEpoch,
      iconHash: iconHash,
    ));
  }

  Future<void> _mediaPlay() async {
    setState(() => _playing = true);
    await _sendMedia();
    _toast('▶ ${_tracks[_trackIdx].title}');
  }

  Future<void> _mediaPause() async {
    setState(() => _playing = false);
    await _sendMedia();
    _toast('⏸ ${_tracks[_trackIdx].title}');
  }

  Future<void> _mediaNext() async {
    setState(() {
      _trackIdx = (_trackIdx + 1) % _tracks.length;
      _positionMs = 0;
    });
    await _sendMedia();
    _toast('⏭ ${_tracks[_trackIdx].title}');
  }

  Future<void> _mediaPrev() async {
    setState(() {
      _trackIdx = (_trackIdx - 1 + _tracks.length) % _tracks.length;
      _positionMs = 0;
    });
    await _sendMedia();
    _toast('⏮ ${_tracks[_trackIdx].title}');
  }

  Future<void> _mediaSeek() async {
    setState(() {
      final dur = _tracks[_trackIdx].duration;
      _positionMs = (_positionMs + 30000) % dur;
    });
    await _sendMedia();
    _toast('Seek +30s');
  }

  @override
  Widget build(BuildContext context) {
    final track = _tracks[_trackIdx];
    return Card(
      child: Padding(
        padding: const EdgeInsets.all(12),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.stretch,
          children: [
            Padding(
              padding: const EdgeInsets.only(bottom: 4, left: 4),
              child: Text(t(context, 'test.notif.section'),
                  style: const TextStyle(fontWeight: FontWeight.bold)),
            ),
            Wrap(
              spacing: 8,
              runSpacing: 8,
              children: [
                FilledButton.tonalIcon(
                  onPressed: _testWhatsapp,
                  icon: const Icon(Icons.chat),
                  label: Text(t(context, 'test.notif.whatsapp')),
                ),
                FilledButton.tonalIcon(
                  onPressed: _testTelegram,
                  icon: const Icon(Icons.send),
                  label: Text(t(context, 'test.notif.telegram')),
                ),
                FilledButton.tonalIcon(
                  onPressed: _testLong,
                  icon: const Icon(Icons.subject),
                  label: Text(t(context, 'test.notif.long')),
                ),
              ],
            ),
            const Divider(height: 24),
            Padding(
              padding: const EdgeInsets.only(left: 4, bottom: 4),
              child: Text(
                  '${t(context, 'test.media.section')} · '
                  '${track.artist} — ${track.title}',
                  style: const TextStyle(fontWeight: FontWeight.bold)),
            ),
            Row(
              children: [
                IconButton.filledTonal(
                    onPressed: _mediaPrev,
                    icon: const Icon(Icons.skip_previous)),
                const SizedBox(width: 4),
                IconButton.filledTonal(
                    onPressed: _playing ? _mediaPause : _mediaPlay,
                    icon: Icon(_playing ? Icons.pause : Icons.play_arrow)),
                const SizedBox(width: 4),
                IconButton.filledTonal(
                    onPressed: _mediaNext, icon: const Icon(Icons.skip_next)),
                const SizedBox(width: 12),
                FilledButton.tonalIcon(
                  onPressed: _mediaSeek,
                  icon: const Icon(Icons.fast_forward),
                  label: Text(t(context, 'test.seek')),
                ),
              ],
            ),
          ],
        ),
      ),
    );
  }
}
