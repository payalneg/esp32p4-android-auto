import 'dart:typed_data';

import 'package:file_picker/file_picker.dart';
import 'package:flutter/material.dart';

import '../ble/ble_proxy.dart';
import '../ble/file_ops.dart';
import '../i18n/strings.dart';
import '../splash/splash_builder.dart';

/// Frame folder + fallback GIF path on the head unit — must match firmware
/// main/splash_screen.c (SPLASH_DIR / SPLASH_PATH).
const String kSplashDir = '/vescfs/splash';
const String kSplashGif = '/vescfs/splash.gif';

/// Configure the head unit's boot splash: slice a GIF into JPEG frames (the new
/// hardware-decoded animated splash) or upload a raw GIF (the fallback).
class SplashSetupScreen extends StatefulWidget {
  const SplashSetupScreen({super.key});
  @override
  State<SplashSetupScreen> createState() => _SplashSetupScreenState();
}

class _SplashSetupScreenState extends State<SplashSetupScreen> {
  final _fm = FileManagerProxy.instance;
  bool _busy = false;

  Future<Uint8List?> _pickGif() async {
    final picked = await FilePicker.platform.pickFiles(
      type: FileType.custom,
      allowedExtensions: ['gif'],
      withData: true,
    );
    if (picked == null || picked.files.isEmpty) return null;
    final bytes = picked.files.first.bytes;
    return bytes == null ? null : Uint8List.fromList(bytes);
  }

  // ---- animated splash (GIF → JPEG frames) ----

  Future<void> _setAnimated() async {
    final gif = await _pickGif();
    if (gif == null || !mounted) return;

    final sBuilding = t(context, 'splash.building');
    final sClearing = t(context, 'splash.clearing');
    final sUploading = t(context, 'splash.uploading');

    final phase = ValueNotifier<String>(sBuilding);
    final progress = ValueNotifier<double>(0);
    Object? caught;
    setState(() => _busy = true);

    final fut = () async {
      final result = await buildSplashFrames(gif);
      phase.value = sClearing;
      await _ensureCleanDir();
      phase.value = sUploading;
      final frames = result.frames;
      for (int i = 0; i < frames.length; i++) {
        final f = frames[i];
        await _fm.uploadFile(
          '$kSplashDir/$i-${f.durMs}.jpg',
          f.jpeg,
          onProgress: (p) => progress.value = (i + p) / frames.length,
        );
      }
    }();

    await _showProgress(phase, progress, fut, (e) => caught = e);
    _finish(caught);
  }

  /// mkdir [kSplashDir] (ignoring "already exists") then delete every file in
  /// it, so stale frames from a previous, longer GIF don't linger.
  Future<void> _ensureCleanDir() async {
    try {
      await _fm.mkdir(kSplashDir);
    } on FileOpException catch (e) {
      if (e.key != 'files.err.exist') rethrow;
    }
    final listing = await _fm.listDir(kSplashDir);
    for (final e in listing.entries) {
      if (!e.isDir) await _fm.deleteEntry('$kSplashDir/${e.name}');
    }
  }

  // ---- fallback raw GIF ----

  Future<void> _setSimpleGif() async {
    final gif = await _pickGif();
    if (gif == null || !mounted) return;

    final phase = ValueNotifier<String>(t(context, 'splash.uploading'));
    final progress = ValueNotifier<double>(0);
    Object? caught;
    setState(() => _busy = true);

    final fut = () async {
      await _fm.uploadFile(kSplashGif, gif,
          onProgress: (p) => progress.value = p);
      // Clear any frame folder so the GIF (lower priority) actually plays.
      try {
        final listing = await _fm.listDir(kSplashDir);
        for (final e in listing.entries) {
          if (!e.isDir) await _fm.deleteEntry('$kSplashDir/${e.name}');
        }
      } catch (_) {/* no frame folder — fine */}
    }();

    await _showProgress(phase, progress, fut, (e) => caught = e);
    _finish(caught);
  }

  // ---- shared helpers ----

  Future<void> _showProgress(
    ValueNotifier<String> phase,
    ValueNotifier<double> progress,
    Future<void> fut,
    void Function(Object) onError,
  ) async {
    await showDialog<void>(
      context: context,
      barrierDismissible: false,
      builder: (ctx) {
        final nav = Navigator.of(ctx);
        fut.then((_) {
          if (nav.canPop()) nav.pop();
        }, onError: (e) {
          onError(e);
          if (nav.canPop()) nav.pop();
        });
        return AlertDialog(
          title: ValueListenableBuilder<String>(
            valueListenable: phase,
            builder: (_, p, __) => Text(p),
          ),
          content: ValueListenableBuilder<double>(
            valueListenable: progress,
            builder: (_, v, __) => Column(
              mainAxisSize: MainAxisSize.min,
              children: [
                LinearProgressIndicator(value: v == 0 ? null : v),
                const SizedBox(height: 8),
                Text('${(v * 100).toStringAsFixed(0)}%'),
              ],
            ),
          ),
        );
      },
    );
  }

  void _finish(Object? caught) {
    if (!mounted) return;
    setState(() => _busy = false);
    final String msg;
    if (caught is FileOpException) {
      msg = t(context, caught.key);
    } else if (caught is SplashBuildException) {
      msg = t(context, caught.key);
    } else if (caught != null) {
      msg = t(context, 'files.err.unknown');
    } else {
      msg = t(context, 'splash.done');
    }
    ScaffoldMessenger.of(context).showSnackBar(SnackBar(content: Text(msg)));
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(title: Text(t(context, 'splash.title'))),
      body: ListView(
        padding: const EdgeInsets.all(16),
        children: [
          Text(t(context, 'splash.intro'),
              style: Theme.of(context).textTheme.bodyMedium),
          const SizedBox(height: 16),
          Card(
            child: ListTile(
              leading: const Icon(Icons.gif_box_outlined),
              title: Text(t(context, 'splash.set_animated')),
              subtitle: Text(t(context, 'splash.set_animated.sub')),
              trailing: const Icon(Icons.chevron_right),
              onTap: _busy ? null : _setAnimated,
            ),
          ),
          Card(
            child: ListTile(
              leading: const Icon(Icons.image_outlined),
              title: Text(t(context, 'splash.set_gif')),
              subtitle: Text(t(context, 'splash.set_gif.sub')),
              trailing: const Icon(Icons.chevron_right),
              onTap: _busy ? null : _setSimpleGif,
            ),
          ),
          const SizedBox(height: 16),
          Row(
            children: [
              const Icon(Icons.info_outline, size: 18),
              const SizedBox(width: 8),
              Expanded(child: Text(t(context, 'splash.reboot_hint'))),
            ],
          ),
        ],
      ),
    );
  }
}
