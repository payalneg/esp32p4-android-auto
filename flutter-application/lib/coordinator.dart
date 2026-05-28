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

  Future<void> start() async {
    _filter = await AppFilter.load();
    _notifSub = _notifBridge.events.listen(_onNotification);
    _mediaSub = _mediaBridge.events.listen(_onMedia);
    _cmdSub = _ble.commands.listen(_onCommand);
  }

  Future<void> stop() async {
    await _notifSub?.cancel();
    await _mediaSub?.cancel();
    await _cmdSub?.cancel();
    _notifSub = null;
    _mediaSub = null;
    _cmdSub = null;
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
    final cached = _packageToIconHash[package];
    if (cached != null) return cached;
    final png = await _notifBridge.getAppIconPng(package);
    if (png == null || png.isEmpty) return 0;
    final hash = fnv1a32(png);
    _packageToIconHash[package] = hash;
    if (!_pushedHashes.contains(hash)) {
      await _ble.sendIcon(IconMsg(hash: hash, png: png));
      _pushedHashes.add(hash);
    }
    return hash;
  }

  Future<void> _onMedia(MediaSnapshot m) async {
    _lastMedia = m;
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
    final art = _lastMedia?.albumArtPng;
    if (art == null) return;
    if (fnv1a32(art) != hash) return;
    await _ble.sendAlbumArt(IconMsg(hash: hash, png: art));
    _pushedHashes.add(hash);
  }
}

