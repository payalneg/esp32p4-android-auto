/// The app's journal: what the UI and the background service have been doing.
///
/// Newest at the top, because a problem is always the last thing that
/// happened. The filter is by level rather than by tag — on a phone, at the
/// side of a road, "show me only what went wrong" is the question worth
/// answering in one tap.
library;

import 'dart:convert';
import 'dart:io';

import 'package:file_picker/file_picker.dart';
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:path_provider/path_provider.dart';
import 'package:share_plus/share_plus.dart';

import '../diagnostics/problem_report.dart';
import '../i18n/strings.dart';
import '../log/app_log.dart';

class LogScreen extends StatefulWidget {
  const LogScreen({super.key});

  @override
  State<LogScreen> createState() => _LogScreenState();
}

class _LogScreenState extends State<LogScreen> {
  bool _problemsOnly = false;
  bool _reporting = false;

  @override
  void initState() {
    super.initState();
    AppLog.instance.addListener(_onLog);
  }

  @override
  void dispose() {
    AppLog.instance.removeListener(_onLog);
    super.dispose();
  }

  void _onLog() {
    if (mounted) setState(() {});
  }

  /// 'aa-bridge-log-0913-2311.txt' — the day and the minute, which is how
  /// anyone looking at two of these tells them apart.
  String _fileName() {
    final n = DateTime.now();
    String two(int v) => v.toString().padLeft(2, '0');
    return 'aa-bridge-log-${two(n.month)}${two(n.day)}'
        '-${two(n.hour)}${two(n.minute)}.txt';
  }

  Future<void> _save() async {
    final bytes = Uint8List.fromList(utf8.encode(AppLog.instance.asText()));
    try {
      final path = await FilePicker.platform.saveFile(
        dialogTitle: t(context, 'log.save'),
        fileName: _fileName(),
        bytes: bytes,
      );
      if (path == null) return;
      // Android writes the bytes itself; a desktop picker only hands back a
      // path, so write again and ignore the harmless second write.
      try {
        await File(path).writeAsBytes(bytes);
      } on Object {
        // already written by the picker
      }
      if (mounted) _snack(t(context, 'log.saved'));
    } on Object catch (e) {
      if (mounted) _snack('$e');
    }
  }

  /// Gather a problem report: this screen, the panel, both logs. Asking the
  /// head unit for its picture takes a moment over BLE, so the button is
  /// disabled while it runs — a second tap would photograph a spinner.
  Future<void> _report() async {
    setState(() => _reporting = true);
    try {
      final n = await ProblemReport.gatherAndShare();
      if (mounted) {
        _snack(tf(context, 'log.reportSent', <String, Object?>{'n': n}));
      }
    } on Object catch (e) {
      if (mounted) _snack('$e');
    } finally {
      if (mounted) setState(() => _reporting = false);
    }
  }

  /// The share sheet takes a file rather than a wall of text: a log pasted
  /// into a chat message is unreadable, an attachment is not.
  Future<void> _share() async {
    // Read the title before the first await: the sheet's subject is the only
    // thing here that needs a context, and it must not outlive one.
    final subject = t(context, 'log.title');
    try {
      final dir = await getTemporaryDirectory();
      final file = File('${dir.path}/${_fileName()}');
      await file.writeAsString(AppLog.instance.asText(), flush: true);
      await Share.shareXFiles(<XFile>[XFile(file.path)], subject: subject);
    } on Object catch (e) {
      if (mounted) _snack('$e');
    }
  }

  void _snack(String message) => ScaffoldMessenger.of(context)
      .showSnackBar(SnackBar(content: Text(message)));

  Color _colour(LogLevel level, ColorScheme scheme) => switch (level) {
        LogLevel.error => scheme.error,
        LogLevel.warn => Colors.orange,
        LogLevel.info => scheme.onSurface,
        LogLevel.debug => scheme.onSurfaceVariant,
      };

  @override
  Widget build(BuildContext context) {
    final theme = Theme.of(context);
    final all = AppLog.instance.entries;
    final shown = (_problemsOnly
            ? all.where((e) =>
                e.level == LogLevel.warn || e.level == LogLevel.error)
            : all)
        .toList()
        .reversed
        .toList();

    return Scaffold(
      appBar: AppBar(
        title: Text(t(context, 'log.title')),
        actions: <Widget>[
          IconButton(
            tooltip: t(context, 'log.copy'),
            icon: const Icon(Icons.copy_all),
            onPressed: all.isEmpty
                ? null
                : () async {
                    await Clipboard.setData(
                        ClipboardData(text: AppLog.instance.asText()));
                    if (context.mounted) {
                      _snack(tf(context, 'log.copied',
                          <String, Object?>{'n': all.length}));
                    }
                  },
          ),
          IconButton(
            tooltip: t(context, 'log.save'),
            icon: const Icon(Icons.save_alt),
            onPressed: all.isEmpty ? null : _save,
          ),
          IconButton(
            tooltip: t(context, 'log.share'),
            icon: const Icon(Icons.share),
            onPressed: all.isEmpty ? null : _share,
          ),
          // A full report is the journal plus the two things it cannot
          // describe: a picture of this screen and a picture of the panel,
          // with the head unit's own log beside them.
          IconButton(
            tooltip: t(context, 'log.report'),
            icon: const Icon(Icons.bug_report_outlined),
            onPressed: _reporting ? null : _report,
          ),
          IconButton(
            tooltip: t(context, 'log.clear'),
            icon: const Icon(Icons.delete_outline),
            onPressed: all.isEmpty ? null : AppLog.instance.clear,
          ),
        ],
      ),
      body: Column(
        children: <Widget>[
          SwitchListTile(
            title: Text(t(context, 'log.problemsOnly')),
            value: _problemsOnly,
            onChanged: (v) => setState(() => _problemsOnly = v),
          ),
          const Divider(height: 1),
          Expanded(
            child: shown.isEmpty
                ? Center(child: Text(t(context, 'log.empty')))
                : ListView.builder(
                    itemCount: shown.length,
                    itemBuilder: (context, i) {
                      final e = shown[i];
                      return Padding(
                        padding: const EdgeInsets.symmetric(
                            horizontal: 12, vertical: 4),
                        child: Text(
                          e.toString(),
                          style: theme.textTheme.bodySmall?.copyWith(
                            fontFamily: 'monospace',
                            color: _colour(e.level, theme.colorScheme),
                          ),
                        ),
                      );
                    },
                  ),
          ),
        ],
      ),
    );
  }
}
