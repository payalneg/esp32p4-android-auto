/// High-level message builders + decoders on top of [Tlv] / [ChunkedEncoder].
library;

import 'dart:typed_data';

import 'protocol.dart';

/// TLV tags inside NOTIFICATION body.
class NotifTag {
  static const int id = 1;
  static const int package = 2;
  static const int appName = 3;
  static const int title = 4;
  static const int text = 5;
  static const int postedAtMs = 6;
  static const int iconHash = 7;
  static const int removed = 8;
  static const int category = 9;
}

/// TLV tags inside MEDIA body.
class MediaTag {
  static const int title = 1;
  static const int artist = 2;
  static const int album = 3;
  static const int durationMs = 4;
  static const int positionMs = 5;
  static const int isPlaying = 6;
  static const int albumArtHash = 7;
  static const int sourceApp = 8;
}

/// TLV tags inside ICON / ALBUM_ART body.
class IconTag {
  static const int hash = 1;
  static const int png = 2;
}

/// Outbound (P4 → phone) command codes — single-byte body for simple ones,
/// or a 4-byte hash payload for icon requests.
class OutboundOp {
  static const int play = 0x10;
  static const int pause = 0x11;
  static const int next = 0x12;
  static const int prev = 0x13;
  static const int dismissNotif = 0x14; // followed by u32 id
  static const int requestIcon = 0x20; // followed by u32 hash
  static const int requestAlbumArt = 0x21; // followed by u32 hash
}

class NotificationMsg {
  final int id;
  final String package;
  final String appName;
  final String title;
  final String text;
  final int postedAtMs;
  final int iconHash;
  final bool removed;
  final String category;

  NotificationMsg({
    required this.id,
    required this.package,
    required this.appName,
    required this.title,
    required this.text,
    required this.postedAtMs,
    required this.iconHash,
    this.removed = false,
    this.category = '',
  });

  Uint8List encode() => Tlv.encode({
        NotifTag.id: id,
        NotifTag.package: package,
        NotifTag.appName: appName,
        NotifTag.title: title,
        NotifTag.text: text,
        NotifTag.postedAtMs: _u64(postedAtMs),
        NotifTag.iconHash: iconHash,
        NotifTag.removed: removed,
        // Only ship a non-empty category — the head unit uses it to keep
        // high-frequency navigation notifications out of the INFO log.
        if (category.isNotEmpty) NotifTag.category: category,
      });

  static Uint8List _u64(int v) {
    final b = ByteData(8)..setUint64(0, v, Endian.little);
    return b.buffer.asUint8List();
  }
}

class MediaMsg {
  final String title;
  final String artist;
  final String album;
  final int durationMs;
  final int positionMs;
  final bool isPlaying;
  final int albumArtHash;
  final String sourceApp;

  MediaMsg({
    required this.title,
    required this.artist,
    required this.album,
    required this.durationMs,
    required this.positionMs,
    required this.isPlaying,
    required this.albumArtHash,
    required this.sourceApp,
  });

  Uint8List encode() => Tlv.encode({
        MediaTag.title: title,
        MediaTag.artist: artist,
        MediaTag.album: album,
        MediaTag.durationMs: durationMs,
        MediaTag.positionMs: positionMs,
        MediaTag.isPlaying: isPlaying,
        MediaTag.albumArtHash: albumArtHash,
        MediaTag.sourceApp: sourceApp,
      });
}

class IconMsg {
  final int hash;
  final Uint8List png;
  IconMsg({required this.hash, required this.png});

  Uint8List encode() => Tlv.encode({
        IconTag.hash: hash,
        IconTag.png: png,
      });
}

/// Decoded inbound command from P4.
class InboundCommand {
  final int op;
  final int? argU32;
  InboundCommand(this.op, [this.argU32]);

  static InboundCommand? parse(Uint8List body) {
    if (body.isEmpty) return null;
    final op = body[0];
    if (body.length >= 5) {
      final arg = ByteData.sublistView(body, 1, 5).getUint32(0, Endian.little);
      return InboundCommand(op, arg);
    }
    return InboundCommand(op);
  }
}
