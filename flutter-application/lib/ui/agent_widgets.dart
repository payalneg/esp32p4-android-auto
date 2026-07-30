/// Transcript tiles for the Assistant tab.
///
/// No markdown or syntax-highlighting package: the renderer here handles
/// fenced code blocks and `**bold**`, which is all the model produces that's
/// worth styling, and the diff carries its own signal through colour.
library;

import 'package:flutter/material.dart';

import '../agent/agent_events.dart';
import '../i18n/strings.dart';
import 'lisp_syntax.dart';

const _mono = TextStyle(fontFamily: 'monospace', fontSize: 12, height: 1.35);

class UserBubble extends StatelessWidget {
  const UserBubble(this.event, {super.key});
  final UserMsgEvent event;

  @override
  Widget build(BuildContext context) {
    final scheme = Theme.of(context).colorScheme;
    return Align(
      alignment: Alignment.centerRight,
      child: Container(
        margin: const EdgeInsets.fromLTRB(48, 6, 0, 6),
        padding: const EdgeInsets.symmetric(horizontal: 12, vertical: 8),
        decoration: BoxDecoration(
          color: scheme.primaryContainer,
          borderRadius: BorderRadius.circular(12),
        ),
        child: Text(event.text,
            style: TextStyle(color: scheme.onPrimaryContainer)),
      ),
    );
  }
}

class AssistantText extends StatelessWidget {
  const AssistantText(this.event, {super.key});
  final AssistantMsgEvent event;

  @override
  Widget build(BuildContext context) {
    return Padding(
      padding: const EdgeInsets.fromLTRB(0, 6, 24, 6),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: _render(context, event.text),
      ),
    );
  }

  /// Fenced blocks become monospace cards; everything else is a paragraph
  /// with `**bold**` honoured.
  static List<Widget> _render(BuildContext context, String text) {
    final out = <Widget>[];
    final parts = text.split('```');
    for (var i = 0; i < parts.length; i++) {
      final chunk = parts[i];
      if (chunk.isEmpty) continue;
      if (i.isOdd) {
        // Drop the ```lisp info line if there is one.
        final body = chunk.contains('\n')
            ? chunk.substring(chunk.indexOf('\n') + 1)
            : chunk;
        out.add(Container(
          width: double.infinity,
          margin: const EdgeInsets.symmetric(vertical: 4),
          padding: const EdgeInsets.all(8),
          decoration: BoxDecoration(
            color: Theme.of(context).colorScheme.surfaceContainerHighest,
            borderRadius: BorderRadius.circular(8),
          ),
          child: SingleChildScrollView(
            scrollDirection: Axis.horizontal,
            // Same colouring as the Code tab, so a snippet the model shows
            // reads exactly like the buffer it is about to change.
            child: LispCodeView(body.trimRight()),
          ),
        ));
      } else {
        out.add(Text.rich(_bold(chunk.trim())));
      }
    }
    return out;
  }

  static TextSpan _bold(String s) {
    final spans = <TextSpan>[];
    final parts = s.split('**');
    for (var i = 0; i < parts.length; i++) {
      spans.add(TextSpan(
        text: parts[i],
        style: i.isOdd ? const TextStyle(fontWeight: FontWeight.bold) : null,
      ));
    }
    return TextSpan(children: spans);
  }
}

class ThinkingTile extends StatelessWidget {
  const ThinkingTile(this.event, {super.key});
  final ThinkingEvent event;

  @override
  Widget build(BuildContext context) {
    final outline = Theme.of(context).colorScheme.outline;
    return Theme(
      data: Theme.of(context).copyWith(dividerColor: Colors.transparent),
      child: ExpansionTile(
        dense: true,
        tilePadding: EdgeInsets.zero,
        leading: Icon(Icons.psychology_outlined, size: 18, color: outline),
        title: Text(t(context, 'agent.thinking'),
            style: TextStyle(fontSize: 12, color: outline)),
        children: [
          Padding(
            padding: const EdgeInsets.only(bottom: 8),
            child: Text(event.text,
                style: TextStyle(
                    fontSize: 12, fontStyle: FontStyle.italic, color: outline)),
          ),
        ],
      ),
    );
  }
}

class ToolStepTile extends StatelessWidget {
  const ToolStepTile(this.event, {super.key});
  final ToolStepEvent event;

  @override
  Widget build(BuildContext context) {
    final scheme = Theme.of(context).colorScheme;
    // Device tools are the ones that can change the bike; colour says so.
    final accent = event.touchedDevice
        ? Colors.amber
        : (event.name.startsWith('apply_') || event.name.startsWith('write_')
            ? scheme.primary
            : scheme.outline);

    final Widget leading = switch (event.status) {
      StepStatus.running => const SizedBox(
          width: 16,
          height: 16,
          child: CircularProgressIndicator(strokeWidth: 2)),
      StepStatus.ok => Icon(Icons.check, size: 18, color: accent),
      StepStatus.failed => Icon(Icons.close, size: 18, color: scheme.error),
      StepStatus.cancelled =>
        Icon(Icons.block, size: 18, color: scheme.outline),
      _ => Icon(Icons.hourglass_empty, size: 18, color: accent),
    };

    return Theme(
      data: Theme.of(context).copyWith(dividerColor: Colors.transparent),
      child: ExpansionTile(
        dense: true,
        tilePadding: const EdgeInsets.symmetric(horizontal: 4),
        leading: leading,
        title: Row(
          children: [
            Flexible(
              child: Text(event.name,
                  style: _mono.copyWith(color: accent),
                  overflow: TextOverflow.ellipsis),
            ),
            if ((event.summary ?? '').isNotEmpty) ...[
              const SizedBox(width: 8),
              Flexible(
                child: Text(event.summary!,
                    style: TextStyle(fontSize: 11, color: scheme.outline),
                    overflow: TextOverflow.ellipsis),
              ),
            ],
          ],
        ),
        children: [
          if (event.diff != null) DiffView(event.diff!),
          if (event.verify != null) VerifyTile(event.verify!),
          Padding(
            padding: const EdgeInsets.fromLTRB(8, 0, 8, 8),
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                if (event.argsJson.trim().isNotEmpty &&
                    event.argsJson.trim() != '{}')
                  _scrollBox(context, event.argsJson),
                if (event.resultJson != null) _scrollBox(context, event.resultJson!),
              ],
            ),
          ),
        ],
      ),
    );
  }

  Widget _scrollBox(BuildContext context, String text) {
    final clipped = text.length > 4000 ? '${text.substring(0, 4000)}\n…' : text;
    return Container(
      width: double.infinity,
      margin: const EdgeInsets.only(top: 4),
      padding: const EdgeInsets.all(8),
      decoration: BoxDecoration(
        color: Theme.of(context).colorScheme.surfaceContainerHighest,
        borderRadius: BorderRadius.circular(6),
      ),
      child: SingleChildScrollView(
        scrollDirection: Axis.horizontal,
        child: Text(clipped, style: _mono.copyWith(fontSize: 11)),
      ),
    );
  }
}

/// Unified diff, coloured. Collapsed past 40 lines so a big edit doesn't push
/// the rest of the transcript off screen.
class DiffView extends StatefulWidget {
  const DiffView(this.diff, {super.key});
  final String diff;

  @override
  State<DiffView> createState() => _DiffViewState();
}

class _DiffViewState extends State<DiffView> {
  bool _expanded = false;

  @override
  Widget build(BuildContext context) {
    final lines = widget.diff.split('\n');
    final show = _expanded ? lines : lines.take(40).toList();
    final scheme = Theme.of(context).colorScheme;
    return Container(
      width: double.infinity,
      margin: const EdgeInsets.fromLTRB(8, 4, 8, 4),
      padding: const EdgeInsets.all(8),
      decoration: BoxDecoration(
        color: scheme.surfaceContainerHighest,
        borderRadius: BorderRadius.circular(6),
      ),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          SingleChildScrollView(
            scrollDirection: Axis.horizontal,
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                for (final l in show)
                  Text(
                    l,
                    style: _mono.copyWith(
                      fontSize: 11,
                      color: l.startsWith('+')
                          ? Colors.green.shade400
                          : l.startsWith('-')
                              ? Colors.red.shade300
                              : l.startsWith('@@')
                                  ? scheme.primary
                                  : null,
                    ),
                  ),
              ],
            ),
          ),
          if (lines.length > 40)
            TextButton(
              onPressed: () => setState(() => _expanded = !_expanded),
              child: Text(_expanded ? '−' : '+ ${lines.length - 40}'),
            ),
        ],
      ),
    );
  }
}

/// The post-flash report — the whole point of the feature, so it gets read
/// like a result, not like log spam.
class VerifyTile extends StatelessWidget {
  const VerifyTile(this.report, {super.key});
  final Map<String, dynamic> report;

  @override
  Widget build(BuildContext context) {
    final scheme = Theme.of(context).colorScheme;
    final pass = report['pass'] == true;
    final verdict = '${report['verdict']}';
    final samples = (report['samples'] as List?) ?? const [];
    final console = (report['console'] as List?) ?? const [];
    final reasons = (report['reasons'] as List?) ?? const [];

    return Container(
      width: double.infinity,
      margin: const EdgeInsets.fromLTRB(8, 4, 8, 4),
      padding: const EdgeInsets.all(10),
      decoration: BoxDecoration(
        color: (pass ? Colors.green : scheme.error).withValues(alpha: 0.10),
        borderRadius: BorderRadius.circular(8),
      ),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Row(
            children: [
              Icon(pass ? Icons.verified : Icons.error_outline,
                  size: 18, color: pass ? Colors.green : scheme.error),
              const SizedBox(width: 6),
              Expanded(
                child: Text(
                  t(context, 'agent.verify.$verdict'),
                  style: const TextStyle(fontWeight: FontWeight.bold),
                ),
              ),
            ],
          ),
          for (final r in reasons)
            Padding(
              padding: const EdgeInsets.only(top: 2),
              child: Text('$r', style: const TextStyle(fontSize: 12)),
            ),
          if ((report['done_ctx'] as String?)?.isNotEmpty ?? false)
            Padding(
              padding: const EdgeInsets.only(top: 4),
              child: Text('done_ctx: ${report['done_ctx']}', style: _mono),
            ),
          if (samples.isNotEmpty) ...[
            const SizedBox(height: 6),
            Text('cpu / heap / mem / stack',
                style: TextStyle(fontSize: 11, color: scheme.outline)),
            for (final s in samples)
              Text(
                '${s['cpu']}%  ${s['heap']}%  ${s['mem']}%  ${s['stack']}%',
                style: _mono.copyWith(fontSize: 11),
              ),
          ],
          if (report['moving_globals'] is Map &&
              (report['moving_globals'] as Map).isNotEmpty) ...[
            const SizedBox(height: 6),
            for (final e in (report['moving_globals'] as Map).entries)
              Text('${e.key}: ${(e.value as List).join(' → ')}',
                  style: _mono.copyWith(fontSize: 11)),
          ],
          if (console.isNotEmpty) ...[
            const SizedBox(height: 6),
            for (final l in console.take(8))
              Text('$l', style: _mono.copyWith(fontSize: 11)),
          ],
          if (report['hint'] != null)
            Padding(
              padding: const EdgeInsets.only(top: 6),
              child: Text('${report['hint']}',
                  style: TextStyle(fontSize: 11, color: scheme.outline)),
            ),
        ],
      ),
    );
  }
}

/// Inline approval for anything that writes to the VESC.
///
/// Inline rather than a dialog: a modal over a streaming transcript is easy to
/// dismiss by reflex, and this way the card stays in the history as a record
/// of what was approved and when.
class ConfirmTile extends StatelessWidget {
  const ConfirmTile(this.event, {super.key, required this.onResolve});
  final ConfirmEvent event;
  final void Function(bool approved) onResolve;

  @override
  Widget build(BuildContext context) {
    final scheme = Theme.of(context).colorScheme;
    final pending = event.status == StepStatus.awaitingUser;
    return Card(
      color: scheme.surfaceContainerHigh,
      margin: const EdgeInsets.symmetric(vertical: 8),
      child: Padding(
        padding: const EdgeInsets.all(12),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Row(
              children: [
                Icon(Icons.memory, size: 18, color: Colors.amber),
                const SizedBox(width: 6),
                Text(t(context, event.titleKey),
                    style: const TextStyle(fontWeight: FontWeight.bold)),
                const Spacer(),
                if (event.diffSummary.isNotEmpty)
                  Text(event.diffSummary,
                      style: TextStyle(fontSize: 11, color: scheme.outline)),
              ],
            ),
            if (event.rationale.isNotEmpty) ...[
              const SizedBox(height: 6),
              Text(event.rationale, style: const TextStyle(fontSize: 13)),
            ],
            if (event.warnings.isNotEmpty) ...[
              const SizedBox(height: 6),
              Text(t(context, 'agent.confirm.warnings'),
                  style: TextStyle(fontSize: 11, color: scheme.outline)),
              for (final w in event.warnings.take(4))
                Text('• $w', style: TextStyle(fontSize: 11, color: scheme.outline)),
            ],
            if (event.run) ...[
              const SizedBox(height: 6),
              Text(t(context, 'agent.confirm.runWarning'),
                  style: TextStyle(fontSize: 12, color: Colors.orange)),
            ],
            const SizedBox(height: 8),
            if (pending)
              Row(
                mainAxisAlignment: MainAxisAlignment.end,
                children: [
                  TextButton(
                    onPressed: () => onResolve(false),
                    child: Text(t(context, 'agent.confirm.cancel')),
                  ),
                  const SizedBox(width: 8),
                  FilledButton(
                    style: event.run
                        ? FilledButton.styleFrom(
                            backgroundColor: scheme.error,
                            foregroundColor: scheme.onError)
                        : null,
                    onPressed: () => onResolve(true),
                    child: Text(t(context, 'agent.confirm.approve')),
                  ),
                ],
              )
            else
              Text(
                t(
                    context,
                    event.status == StepStatus.ok
                        ? 'agent.confirm.approved'
                        : 'agent.confirm.declined'),
                style: TextStyle(fontSize: 12, color: scheme.outline),
              ),
          ],
        ),
      ),
    );
  }
}

class NoticeTile extends StatelessWidget {
  const NoticeTile(this.event, {super.key});
  final NoticeEvent event;

  @override
  Widget build(BuildContext context) {
    final scheme = Theme.of(context).colorScheme;
    final color = switch (event.kind) {
      NoticeKind.error => scheme.error,
      NoticeKind.budget => Colors.orange,
      NoticeKind.info => scheme.outline,
    };
    var text = t(context, event.messageKey);
    event.args?.forEach((k, v) => text = text.replaceAll('{$k}', v));
    return Padding(
      padding: const EdgeInsets.symmetric(vertical: 6),
      child: Row(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Icon(
              event.kind == NoticeKind.error
                  ? Icons.error_outline
                  : Icons.info_outline,
              size: 16,
              color: color),
          const SizedBox(width: 6),
          Expanded(
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Text(text, style: TextStyle(fontSize: 12, color: color)),
                if (event.detail != null)
                  Text(event.detail!,
                      style: TextStyle(fontSize: 11, color: scheme.outline)),
              ],
            ),
          ),
        ],
      ),
    );
  }
}
