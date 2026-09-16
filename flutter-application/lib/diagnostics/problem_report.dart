/// One tap that produces something worth sending.
///
/// A problem described in words loses the two things that identify it: what the
/// panel was showing, and what the firmware was saying at the time. This
/// gathers both ends of the link — the head unit's screen and log, the phone's
/// screen and journal, plus the versions and the last reset reason — and hands
/// them to the share sheet as files. Attachments, not a wall of pasted text:
/// the log screen learned that already.
///
/// Everything here is best-effort. A head unit that is not connected, a panel
/// too busy to snapshot, a filesystem that is not mounted — each just means one
/// fewer attachment, never a failed report.
library;

import 'dart:io';
import 'dart:typed_data';
import 'dart:ui' as ui;

import 'package:flutter/rendering.dart';
import 'package:flutter/widgets.dart';
import 'package:path_provider/path_provider.dart';
import 'package:share_plus/share_plus.dart';

import '../ble/ble_proxy.dart';
import '../log/app_log.dart';

/// Wrapped around the app's root so the report can photograph whatever the UI
/// is showing. A boundary that never repaints costs nothing.
final GlobalKey appCaptureKey = GlobalKey(debugLabel: 'appCapture');

class ProblemReport {
  ProblemReport._();

  /// Collect and offer to share. [note] is the rider's own description, if the
  /// caller asked for one. Returns the number of attachments that made it, so
  /// the UI can say "sent the log only" rather than pretend all was well.
  static Future<int> gatherAndShare({String? note}) async {
    final dir = await getTemporaryDirectory();
    final stamp = DateTime.now()
        .toIso8601String()
        .replaceAll(':', '-')
        .split('.')
        .first;
    final files = <XFile>[];

    // 1. The phone's own screen, straight from the render tree.
    final shot = await _captureApp();
    if (shot != null) {
      final f = File('${dir.path}/report-$stamp-phone.png');
      await f.writeAsBytes(shot, flush: true);
      files.add(XFile(f.path));
    }

    // 2. The head unit's screen and log. Asked for together, attached
    //    separately, because either can be missing on its own.
    try {
      final head = await FileManagerProxy.instance.captureReport();
      if (head.screen != null && head.screen!.isNotEmpty) {
        final f = File('${dir.path}/report-$stamp-panel.jpg');
        await f.writeAsBytes(head.screen!, flush: true);
        files.add(XFile(f.path));
      }
      if (head.log != null && head.log!.isNotEmpty) {
        final f = File('${dir.path}/report-$stamp-panel.log');
        await f.writeAsBytes(head.log!, flush: true);
        files.add(XFile(f.path));
      }
    } on Object catch (e) {
      AppLog.w('report', 'head unit gave nothing — $e');
    }

    // 3. The app's own journal, and the header that says what all of this is.
    final summary = File('${dir.path}/report-$stamp-app.log');
    await summary.writeAsString(_header(note) + AppLog.instance.asText(),
        flush: true);
    files.add(XFile(summary.path));

    await Share.shareXFiles(files, subject: 'Problem report $stamp');
    return files.length;
  }

  /// PNG of the live UI. Null when the boundary is not mounted (the report was
  /// triggered with no UI on screen) or the raster fails.
  static Future<Uint8List?> _captureApp() async {
    try {
      final obj = appCaptureKey.currentContext?.findRenderObject();
      if (obj is! RenderRepaintBoundary) return null;
      // 2x so text in the screenshot is legible when someone opens it on a
      // desktop; a phone screen at 1x is thumbnail-sized there.
      final image = await obj.toImage(pixelRatio: 2);
      final data = await image.toByteData(format: ui.ImageByteFormat.png);
      image.dispose();
      return data?.buffer.asUint8List();
    } on Object catch (e) {
      AppLog.w('report', 'app screenshot failed — $e');
      return null;
    }
  }

  static String _header(String? note) {
    final b = StringBuffer()
      ..writeln('# Problem report')
      ..writeln('when: ${DateTime.now()}');
    if (note != null && note.trim().isNotEmpty) {
      b..writeln('note: ${note.trim()}')..writeln();
    }
    b.writeln();
    b.writeln('## app journal');
    return b.toString();
  }
}
