import 'dart:io';
import 'dart:typed_data';

import 'package:file_picker/file_picker.dart';
import 'package:flutter/material.dart';
import 'package:path_provider/path_provider.dart';

import '../ble/file_manager.dart';
import '../ble/file_ops.dart';
import '../i18n/strings.dart';

/// Browse + manage the head unit's filesystem (/vescfs + /sdcard) over BLE.
class DeviceFilesScreen extends StatefulWidget {
  const DeviceFilesScreen({super.key});
  @override
  State<DeviceFilesScreen> createState() => _DeviceFilesScreenState();
}

class _DeviceFilesScreenState extends State<DeviceFilesScreen> {
  final _fm = FileManager.instance;
  String _cwd = '/';
  List<RemoteEntry> _entries = const [];
  bool _loading = true;
  bool _truncated = false;
  String? _error; // localized message
  bool _notReady = false;

  @override
  void initState() {
    super.initState();
    _reload();
  }

  String _join(String dir, String name) =>
      dir == '/' ? '/$name' : '$dir/$name';

  Future<void> _reload() async {
    setState(() {
      _loading = true;
      _error = null;
      _notReady = false;
    });
    try {
      final listing = await _fm.listDir(_cwd);
      if (!mounted) return;
      setState(() {
        _entries = listing.entries;
        _truncated = listing.truncated;
        _loading = false;
      });
    } on FileOpException catch (e) {
      if (!mounted) return;
      setState(() {
        _loading = false;
        _notReady = e.notReady;
        _error = t(context, e.key);
      });
    } catch (e, st) {
      debugPrint('[files] listDir("$_cwd") FAILED: $e');
      debugPrint('[files] $st');
      if (!mounted) return;
      setState(() {
        _loading = false;
        _error = t(context, 'files.err.unknown');
      });
    }
  }

  void _enter(String name) {
    setState(() => _cwd = _join(_cwd, name));
    _reload();
  }

  void _up() {
    if (_cwd == '/') return;
    final i = _cwd.lastIndexOf('/');
    setState(() => _cwd = (i <= 0) ? '/' : _cwd.substring(0, i));
    _reload();
  }

  bool get _atRoot => _cwd == '/';

  // ---- actions ----

  Future<void> _doUpload() async {
    final picked = await FilePicker.platform.pickFiles(withData: true);
    if (picked == null || picked.files.isEmpty) return;
    final f = picked.files.first;
    final bytes = f.bytes;
    if (bytes == null) return;
    final dest = _join(_cwd, f.name);
    await _runTransfer(
      title: t(context, 'files.uploading'),
      task: (onProgress) =>
          _fm.uploadFile(dest, Uint8List.fromList(bytes), onProgress: onProgress),
    );
    await _reload();
  }

  Future<void> _doDownload(RemoteEntry e) async {
    final src = _join(_cwd, e.name);
    Uint8List? data;
    final ok = await _runTransfer(
      title: t(context, 'files.downloading'),
      task: (onProgress) async {
        data = await _fm.downloadFile(src, onProgress: onProgress);
      },
    );
    if (!ok || data == null) return;
    try {
      Directory? dir;
      if (Platform.isAndroid) dir = await getDownloadsDirectory();
      dir ??= await getApplicationDocumentsDirectory();
      final out = File('${dir.path}/${e.name}');
      await out.writeAsBytes(data!);
      if (mounted) {
        ScaffoldMessenger.of(context).showSnackBar(SnackBar(
            content: Text('${t(context, 'files.saved')}${out.path}')));
      }
    } catch (_) {
      _snack(t(context, 'files.err.io'));
    }
  }

  Future<void> _doDelete(RemoteEntry e) async {
    final ok = await showDialog<bool>(
      context: context,
      builder: (ctx) => AlertDialog(
        title: Text(t(context, 'files.delete.confirm')),
        content: Text(e.name),
        actions: [
          TextButton(
              onPressed: () => Navigator.pop(ctx, false),
              child: Text(t(context, 'files.cancel'))),
          FilledButton(
              onPressed: () => Navigator.pop(ctx, true),
              child: Text(t(context, 'files.delete'))),
        ],
      ),
    );
    if (ok != true) return;
    await _guard(() => _fm.deleteEntry(_join(_cwd, e.name)));
    await _reload();
  }

  Future<void> _doRename(RemoteEntry e) async {
    final name = await _promptText(t(context, 'files.rename.to'), e.name);
    if (name == null || name.isEmpty || name == e.name) return;
    await _guard(() => _fm.rename(_join(_cwd, e.name), _join(_cwd, name)));
    await _reload();
  }

  Future<void> _doMkdir() async {
    final name = await _promptText(t(context, 'files.mkdir.name'), '');
    if (name == null || name.isEmpty) return;
    await _guard(() => _fm.mkdir(_join(_cwd, name)));
    await _reload();
  }

  // ---- helpers ----

  Future<bool> _guard(Future<void> Function() op) async {
    try {
      await op();
      return true;
    } on FileOpException catch (e) {
      _snack(t(context, e.key));
    } catch (_) {
      _snack(t(context, 'files.err.unknown'));
    }
    return false;
  }

  void _snack(String msg) {
    if (mounted) {
      ScaffoldMessenger.of(context)
          .showSnackBar(SnackBar(content: Text(msg)));
    }
  }

  Future<String?> _promptText(String label, String initial) {
    final ctrl = TextEditingController(text: initial);
    return showDialog<String>(
      context: context,
      builder: (ctx) => AlertDialog(
        content: TextField(
          controller: ctrl,
          autofocus: true,
          decoration: InputDecoration(labelText: label),
        ),
        actions: [
          TextButton(
              onPressed: () => Navigator.pop(ctx),
              child: Text(t(context, 'files.cancel'))),
          FilledButton(
              onPressed: () => Navigator.pop(ctx, ctrl.text.trim()),
              child: Text(t(context, 'files.ok'))),
        ],
      ),
    );
  }

  /// Run an upload/download with a modal progress dialog. Returns success.
  Future<bool> _runTransfer({
    required String title,
    required Future<void> Function(void Function(double)) task,
  }) async {
    final progress = ValueNotifier<double>(0);
    bool ok = false;
    String? err;
    final fut = task((p) => progress.value = p).then((_) {
      ok = true;
    }, onError: (e) {
      err = (e is FileOpException)
          ? t(context, e.key)
          : t(context, 'files.err.unknown');
    });
    await showDialog<void>(
      context: context,
      barrierDismissible: false,
      builder: (ctx) {
        fut.whenComplete(() {
          if (Navigator.of(ctx).canPop()) Navigator.of(ctx).pop();
        });
        return AlertDialog(
          title: Text(title),
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
    if (err != null) _snack(err!);
    return ok;
  }

  String _fmtSize(int b) {
    if (b < 1024) return '$b B';
    if (b < 1024 * 1024) return '${(b / 1024).toStringAsFixed(1)} KiB';
    if (b < 1024 * 1024 * 1024) return '${(b / 1048576).toStringAsFixed(1)} MiB';
    return '${(b / 1073741824).toStringAsFixed(2)} GiB';
  }

  void _onLongPress(RemoteEntry e) {
    showModalBottomSheet<void>(
      context: context,
      builder: (ctx) => SafeArea(
        child: Column(
          mainAxisSize: MainAxisSize.min,
          children: [
            if (!e.isDir)
              ListTile(
                leading: const Icon(Icons.download),
                title: Text(t(context, 'files.download')),
                onTap: () {
                  Navigator.pop(ctx);
                  _doDownload(e);
                },
              ),
            ListTile(
              leading: const Icon(Icons.drive_file_rename_outline),
              title: Text(t(context, 'files.rename')),
              onTap: () {
                Navigator.pop(ctx);
                _doRename(e);
              },
            ),
            ListTile(
              leading: const Icon(Icons.delete_outline),
              title: Text(t(context, 'files.delete')),
              onTap: () {
                Navigator.pop(ctx);
                _doDelete(e);
              },
            ),
          ],
        ),
      ),
    );
  }

  @override
  Widget build(BuildContext context) {
    return PopScope(
      canPop: _atRoot,
      onPopInvokedWithResult: (didPop, _) {
        if (!didPop) _up();
      },
      child: Scaffold(
        appBar: AppBar(
          title: Text(t(context, 'files.title')),
          leading: _atRoot
              ? null
              : IconButton(icon: const Icon(Icons.arrow_back), onPressed: _up),
          actions: [
            IconButton(
              icon: const Icon(Icons.refresh),
              onPressed: _loading ? null : _reload,
            ),
            if (!_atRoot)
              IconButton(
                icon: const Icon(Icons.create_new_folder_outlined),
                tooltip: t(context, 'files.mkdir'),
                onPressed: _doMkdir,
              ),
          ],
        ),
        body: Column(
          children: [
            Container(
              width: double.infinity,
              padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 8),
              color: Theme.of(context).colorScheme.surfaceContainerHigh,
              child: Text(_cwd,
                  style: Theme.of(context).textTheme.bodySmall,
                  overflow: TextOverflow.ellipsis),
            ),
            Expanded(child: _buildBody()),
          ],
        ),
        floatingActionButton: _atRoot
            ? null
            : FloatingActionButton(
                onPressed: _doUpload,
                tooltip: t(context, 'files.upload'),
                child: const Icon(Icons.upload_file),
              ),
      ),
    );
  }

  Widget _buildBody() {
    if (_loading) return const Center(child: CircularProgressIndicator());
    if (_notReady) {
      return Center(
        child: Padding(
          padding: const EdgeInsets.all(24),
          child: Column(
            mainAxisSize: MainAxisSize.min,
            children: [
              const Icon(Icons.hourglass_top, size: 40),
              const SizedBox(height: 12),
              Text(t(context, 'files.notready'), textAlign: TextAlign.center),
              const SizedBox(height: 12),
              FilledButton(
                  onPressed: _reload,
                  child: Text(t(context, 'files.notready.retry'))),
            ],
          ),
        ),
      );
    }
    if (_error != null) {
      return Center(
        child: Padding(
          padding: const EdgeInsets.all(24),
          child: Column(
            mainAxisSize: MainAxisSize.min,
            children: [
              const Icon(Icons.error_outline, size: 40),
              const SizedBox(height: 12),
              Text(_error!, textAlign: TextAlign.center),
              const SizedBox(height: 12),
              FilledButton(
                  onPressed: _reload,
                  child: Text(t(context, 'files.notready.retry'))),
            ],
          ),
        ),
      );
    }
    if (_entries.isEmpty) {
      return Center(child: Text(t(context, 'files.empty')));
    }
    return ListView.builder(
      itemCount: _entries.length + (_truncated ? 1 : 0),
      itemBuilder: (ctx, i) {
        if (i >= _entries.length) {
          return ListTile(
            leading: const Icon(Icons.more_horiz),
            title: Text(t(context, 'files.truncated')),
          );
        }
        final e = _entries[i];
        return ListTile(
          leading: Icon(e.isDir ? Icons.folder : Icons.insert_drive_file),
          title: Text(e.name),
          subtitle:
              Text(e.isDir ? t(context, 'files.dir') : _fmtSize(e.size)),
          trailing: e.isDir ? const Icon(Icons.chevron_right) : null,
          onTap: e.isDir ? () => _enter(e.name) : () => _doDownload(e),
          // No rename/delete on the synthetic drive roots.
          onLongPress: _atRoot ? null : () => _onLongPress(e),
        );
      },
    );
  }
}
