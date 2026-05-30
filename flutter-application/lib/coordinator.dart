/// Wires native event streams + filter + icon cache → BleService.
///
/// Singleton: one instance for the app lifetime. Maintains the set of icon
/// hashes already pushed to the head unit so we never re-send identical
/// PNG bytes. Honors P4-side icon requests by re-fetching from PackageManager.
library;

import 'dart:async';

import 'ble/ble_service.dart';
import 'ble/messages.dart';
import 'bridge/media_bridge.dart';
import 'bridge/notification_bridge.dart';
import 'cache/icon_hash.dart';
import 'settings/app_filter.dart';

class Coordinator {
  Coordinator._();
  static final Coordinator instance = Coordinator._();

  final _ble = BleService.instance;
  final _notifBridge = NotificationBridge();
  final _mediaBridge = MediaBridge();

  AppFilter? _filter;
  StreamSubscription? _notifSub;
  StreamSubscription? _mediaSub;
  StreamSubscription? _cmdSub;

  final Map<String, int> _packageToIconHash = {};
  final Set<int> _pushedHashes = {};
  int _lastAlbumArtHash = 0;
  MediaSnapshot? _lastMedia;

  StreamSubscription? _stateSub;

  Future<void> start() async {
    _filter = await AppFilter.load();
    _notifSub = _notifBridge.events.listen(_onNotification);
    _mediaSub = _mediaBridge.events.listen(_onMedia);
    _cmdSub = _ble.commands.listen(_onCommand);
    // Re-arm the resend caches on every reconnect — the head unit boots
    // with an empty icon LRU after a power-cycle / reflash, so the phone
    // mustn't think anything is still "already pushed".
    _stateSub = _ble.state.listen((s) {
      if (s == BleConnState.connected) {
        _pushedHashes.clear();
        _lastAlbumArtHash = 0;
      }
    });
  }

  Future<void> stop() async {
    await _notifSub?.cancel();
    await _mediaSub?.cancel();
    await _cmdSub?.cancel();
    await _stateSub?.cancel();
    _notifSub = null;
    _mediaSub = null;
    _cmdSub = null;
    _stateSub = null;
  }

  AppFilter? get filter => _filter;

  Future<void> _onNotification(IncomingNotification n) async {
    if (_filter?.allows(n.package) == false) return;
    final iconHash = await _ensureIcon(n.package);
    await _ble.sendNotification(NotificationMsg(
      id: n.id,
      package: n.package,
      appName: n.appName,
      title: n.title,
      text: n.text,
      postedAtMs: n.postedAtMs,
      iconHash: iconHash,
      removed: n.removed,
    ));
  }

  Future<int> _ensureIcon(String package) async {
    // Fast path: we have a recent hash AND we already pushed the PNG
    // bytes to the head unit (since the last reconnect cleared the
    // pushed-set). Skip the PackageManager round-trip.
    final cached = _packageToIconHash[package];
    if (cached != null && _pushedHashes.contains(cached)) return cached;

    // Either first time, or the head unit was reset and lost the LRU.
    // Re-fetch the PNG and re-send so the toast resolves the hash on
    // the firmware side; otherwise the head unit would render the
    // first-letter fallback ("T" instead of the Telegram glyph).
    final png = await _notifBridge.getAppIconPng(package);
    if (png == null || png.isEmpty) return 0;
    final hash = fnv1a32(png);
    _packageToIconHash[package] = hash;
    await _ble.sendIcon(IconMsg(hash: hash, png: png));
    _pushedHashes.add(hash);
    return hash;
  }

  // Set while a long PDU (album art) is in flight, so the 1 Hz media
  // ticker doesn't pile a second sendAlbumArt on top of the first.
  bool _mediaSending = false;

  Future<void> _onMedia(MediaSnapshot m) async {
    _lastMedia = m;
    if (_mediaSending) return;
    _mediaSending = true;
    try {
      int artHash = 0;
      if (m.albumArtPng != null && m.albumArtPng!.isNotEmpty) {
        artHash = fnv1a32(m.albumArtPng!);
        if (artHash != _lastAlbumArtHash && !_pushedHashes.contains(artHash)) {
          await _ble.sendAlbumArt(IconMsg(hash: artHash, png: m.albumArtPng!));
          _pushedHashes.add(artHash);
        }
        _lastAlbumArtHash = artHash;
      }
      await _ble.sendMedia(MediaMsg(
        title: m.title,
        artist: m.artist,
        album: m.album,
        durationMs: m.durationMs,
        positionMs: m.positionMs,
        isPlaying: m.isPlaying,
        albumArtHash: artHash,
        sourceApp: m.sourceApp,
      ));
    } catch (_) {
      // BleService._send already swallows transient errors; this catch
      // is a backstop so anything escaping doesn't crash the isolate.
    } finally {
      _mediaSending = false;
    }
  }

  void _onCommand(InboundCommand cmd) {
    switch (cmd.op) {
      case OutboundOp.play:
        _mediaBridge.sendControl(MediaCommand.play);
        break;
      case OutboundOp.pause:
        _mediaBridge.sendControl(MediaCommand.pause);
        break;
      case OutboundOp.next:
        _mediaBridge.sendControl(MediaCommand.next);
        break;
      case OutboundOp.prev:
        _mediaBridge.sendControl(MediaCommand.prev);
        break;
      case OutboundOp.requestIcon:
        _resendIconByHash(cmd.argU32 ?? 0);
        break;
      case OutboundOp.requestAlbumArt:
        _resendAlbumArt(cmd.argU32 ?? 0);
        break;
    }
  }

  Future<void> _resendIconByHash(int hash) async {
    String? matchedPkg;
    _packageToIconHash.forEach((pkg, h) {
      if (h == hash) matchedPkg = pkg;
    });
    if (matchedPkg == null) return;
    final png = await _notifBridge.getAppIconPng(matchedPkg!);
    if (png == null) return;
    await _ble.sendIcon(IconMsg(hash: hash, png: png));
    _pushedHashes.add(hash);
  }

  Future<void> _resendAlbumArt(int hash) async {
    // Drop the cache so the next media tick re-uploads the PNG
    // unconditionally. We don't try to validate the hash against the
    // last snapshot because the head unit's request races with the next
    // track change — by the time it lands here, the player may already
    // be on a new song with different bytes. Force the resend and let
    // the tick that follows reconcile.
    _pushedHashes.remove(hash);
    _lastAlbumArtHash = 0;
    final art = _lastMedia?.albumArtPng;
    if (art == null || art.isEmpty) return;
    final h = fnv1a32(art);
    await _ble.sendAlbumArt(IconMsg(hash: h, png: art));
    _pushedHashes.add(h);
  }
}

