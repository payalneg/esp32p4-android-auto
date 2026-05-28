import 'dart:async';

import 'package:flutter/services.dart';

class MediaSnapshot {
  final String title;
  final String artist;
  final String album;
  final int durationMs;
  final int positionMs;
  final bool isPlaying;
  final String sourceApp;
  final Uint8List? albumArtPng;

  MediaSnapshot({
    required this.title,
    required this.artist,
    required this.album,
    required this.durationMs,
    required this.positionMs,
    required this.isPlaying,
    required this.sourceApp,
    required this.albumArtPng,
  });

  factory MediaSnapshot.fromMap(Map m) => MediaSnapshot(
        title: m['title'] as String? ?? '',
        artist: m['artist'] as String? ?? '',
        album: m['album'] as String? ?? '',
        durationMs: (m['durationMs'] as num?)?.toInt() ?? 0,
        positionMs: (m['positionMs'] as num?)?.toInt() ?? 0,
        isPlaying: m['isPlaying'] as bool? ?? false,
        sourceApp: m['sourceApp'] as String? ?? '',
        albumArtPng: m['albumArt'] as Uint8List?,
      );
}

enum MediaCommand { play, pause, next, prev }

class MediaBridge {
  static const _events = EventChannel('aabridge/media');
  static const _methods = MethodChannel('aabridge/media.cmd');

  Stream<MediaSnapshot> get events =>
      _events.receiveBroadcastStream().map((e) => MediaSnapshot.fromMap(e as Map));

  Future<void> sendControl(MediaCommand cmd) =>
      _methods.invokeMethod('control', {'cmd': cmd.name});
}
