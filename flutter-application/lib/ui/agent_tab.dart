/// The Assistant tab: chat on the left of the same buffer the Code tab edits.
///
/// The session owns the editor's [TextEditingController], so an `apply_patch`
/// lands in the Code tab live and the user can hand-edit between steps.
library;

import 'dart:async';

import 'package:flutter/material.dart';

import '../agent/agent_events.dart';
import '../agent/agent_session.dart';
import '../i18n/strings.dart';
import '../settings/agent_settings.dart';
import 'agent_settings_screen.dart';
import 'agent_widgets.dart';

class AgentTab extends StatefulWidget {
  const AgentTab({super.key, required this.session});
  final AgentSession session;

  @override
  State<AgentTab> createState() => _AgentTabState();
}

class _AgentTabState extends State<AgentTab> {
  final _input = TextEditingController();
  final _scroll = ScrollController();
  StreamSubscription<void>? _sub;
  bool _pinned = true;

  AgentSession get _s => widget.session;

  @override
  void initState() {
    super.initState();
    _scroll.addListener(() {
      if (!_scroll.hasClients) return;
      _pinned = _scroll.offset >= _scroll.position.maxScrollExtent - 40;
    });
    // A bare change signal; the list rebuilds its tail. Throttled to one frame
    // so a fast token stream doesn't schedule a setState per token.
    _sub = _s.changes.listen((_) {
      if (!mounted) return;
      setState(() {});
      _autoScroll();
    });
    AgentSettings.instance.addListener(_onSettings);
  }

  void _onSettings() {
    if (mounted) setState(() {});
  }

  @override
  void dispose() {
    _sub?.cancel();
    AgentSettings.instance.removeListener(_onSettings);
    _input.dispose();
    _scroll.dispose();
    super.dispose();
  }

  void _autoScroll() {
    if (!_pinned) return;
    WidgetsBinding.instance.addPostFrameCallback((_) {
      if (_scroll.hasClients) {
        _scroll.jumpTo(_scroll.position.maxScrollExtent);
      }
    });
  }

  void _send() {
    final text = _input.text.trim();
    if (text.isEmpty || _s.busy) return;
    _input.clear();
    unawaited(_s.send(text));
  }

  @override
  Widget build(BuildContext context) {
    if (!AgentSettings.instance.configured) return _setupPrompt(context);
    // Shown once, before anything can be typed: this assistant writes code
    // that runs on the motor controller of a vehicle someone rides.
    if (!AgentSettings.instance.disclaimerAcknowledged) {
      return _disclaimer(context);
    }

    return Column(
      children: [
        _header(context),
        const Divider(height: 1),
        Expanded(
          child: _s.events.isEmpty
              ? _emptyState(context)
              : ListView.builder(
                  controller: _scroll,
                  padding: const EdgeInsets.fromLTRB(12, 8, 12, 8),
                  itemCount: _s.events.length,
                  itemBuilder: (_, i) => _tile(_s.events[i]),
                ),
        ),
        if (_s.hasFlashed) _stopScriptBar(context),
        _composer(context),
      ],
    );
  }

  Widget _tile(AgentEvent e) => switch (e) {
        UserMsgEvent() => UserBubble(e, key: ValueKey(e.id)),
        AssistantMsgEvent() => AssistantText(e, key: ValueKey(e.id)),
        ThinkingEvent() => ThinkingTile(e, key: ValueKey(e.id)),
        ToolStepEvent() => ToolStepTile(e, key: ValueKey(e.id)),
        ConfirmEvent() => ConfirmTile(e,
            key: ValueKey(e.id),
            onResolve: (ok) => _s.resolveConfirmation(e.callId, ok)),
        NoticeEvent() => NoticeTile(e, key: ValueKey(e.id)),
      };

  Widget _setupPrompt(BuildContext context) {
    return Center(
      child: Padding(
        padding: const EdgeInsets.all(24),
        child: Column(
          mainAxisSize: MainAxisSize.min,
          children: [
            const Icon(Icons.auto_awesome, size: 40),
            const SizedBox(height: 12),
            Text(t(context, 'agent.setup.desc'), textAlign: TextAlign.center),
            const SizedBox(height: 16),
            FilledButton(
              onPressed: () => Navigator.push(
                context,
                MaterialPageRoute(
                    builder: (_) => const AgentSettingsScreen()),
              ),
              child: Text(t(context, 'agent.setup')),
            ),
          ],
        ),
      ),
    );
  }

  /// Full-screen, must be accepted once. Deliberately not a snackbar or a
  /// footnote: the failure mode being warned about is a vehicle behaving
  /// unexpectedly, not a typo.
  Widget _disclaimer(BuildContext context) {
    final scheme = Theme.of(context).colorScheme;
    return SingleChildScrollView(
      padding: const EdgeInsets.all(20),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.stretch,
        children: [
          const SizedBox(height: 8),
          Icon(Icons.warning_amber_rounded, size: 56, color: Colors.orange),
          const SizedBox(height: 12),
          Text(
            t(context, 'agent.disclaimer.title'),
            textAlign: TextAlign.center,
            style: Theme.of(context)
                .textTheme
                .titleLarge
                ?.copyWith(fontWeight: FontWeight.bold),
          ),
          const SizedBox(height: 16),
          Container(
            padding: const EdgeInsets.all(16),
            decoration: BoxDecoration(
              color: Colors.orange.withValues(alpha: 0.12),
              borderRadius: BorderRadius.circular(12),
              border: Border.all(color: Colors.orange.withValues(alpha: 0.5)),
            ),
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                for (final k in const [
                  'agent.disclaimer.p1',
                  'agent.disclaimer.p2',
                  'agent.disclaimer.p3',
                  'agent.disclaimer.p4',
                ])
                  Padding(
                    padding: const EdgeInsets.only(bottom: 10),
                    child: Row(
                      crossAxisAlignment: CrossAxisAlignment.start,
                      children: [
                        const Text('•  '),
                        Expanded(
                          child: Text(t(context, k),
                              style: const TextStyle(fontSize: 14, height: 1.4)),
                        ),
                      ],
                    ),
                  ),
              ],
            ),
          ),
          const SizedBox(height: 16),
          Text(
            t(context, 'agent.disclaimer.risk'),
            textAlign: TextAlign.center,
            style: TextStyle(
                fontSize: 13,
                fontWeight: FontWeight.w600,
                color: scheme.error),
          ),
          const SizedBox(height: 20),
          FilledButton(
            style: FilledButton.styleFrom(
                padding: const EdgeInsets.symmetric(vertical: 14)),
            onPressed: () async {
              await AgentSettings.instance.acknowledgeDisclaimer();
              if (mounted) setState(() {});
            },
            child: Text(t(context, 'agent.disclaimer.accept')),
          ),
          const SizedBox(height: 24),
        ],
      ),
    );
  }

  Widget _emptyState(BuildContext context) => ListView(
        padding: const EdgeInsets.all(20),
        children: [
          Container(
            padding: const EdgeInsets.all(12),
            decoration: BoxDecoration(
              color: Colors.orange.withValues(alpha: 0.10),
              borderRadius: BorderRadius.circular(10),
              border: Border.all(color: Colors.orange.withValues(alpha: 0.4)),
            ),
            child: Row(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                const Icon(Icons.warning_amber_rounded,
                    size: 20, color: Colors.orange),
                const SizedBox(width: 10),
                Expanded(
                  child: Text(t(context, 'agent.disclaimer.short'),
                      style: const TextStyle(fontSize: 12, height: 1.4)),
                ),
              ],
            ),
          ),
          const SizedBox(height: 20),
          Text(
            t(context, 'agent.empty'),
            textAlign: TextAlign.center,
            style: TextStyle(color: Theme.of(context).colorScheme.outline),
          ),
        ],
      );

  Widget _header(BuildContext context) {
    final scheme = Theme.of(context).colorScheme;
    final p = _s.progress;
    final small = TextStyle(fontSize: 11, color: scheme.outline);
    final cost = p.costUsd;

    return Padding(
      padding: const EdgeInsets.fromLTRB(12, 6, 8, 6),
      child: Row(
        children: [
          Flexible(
            child: Text(AgentSettings.instance.model,
                style: small, overflow: TextOverflow.ellipsis),
          ),
          const SizedBox(width: 10),
          if (p.step > 0)
            Text(
                t(context, 'agent.step')
                    .replaceAll('{n}', '${p.step}')
                    .replaceAll('{max}', '${p.maxSteps}'),
                style: small),
          const SizedBox(width: 10),
          // A dash, never a made-up number, when the price is unknown.
          Text(cost == null ? '—' : '\$${cost.toStringAsFixed(4)}', style: small),
          if (p.usage.promptTokens > 0) ...[
            const SizedBox(width: 6),
            Text(
                t(context, 'agent.cached').replaceAll(
                    '{pct}', '${(p.usage.cacheRatio * 100).round()}'),
                style: small),
          ],
          const Spacer(),
          if (_s.busy)
            TextButton.icon(
              icon: const Icon(Icons.stop_circle_outlined, size: 18),
              label: Text(t(context, 'agent.stop')),
              style: TextButton.styleFrom(foregroundColor: scheme.error),
              onPressed: _s.cancel,
            )
          else ...[
            // Starting a fresh conversation and reaching the API key both have
            // to be here: this tab is where the assistant lives, and with a
            // key already configured the setup screen never appears.
            IconButton(
              icon: const Icon(Icons.add_comment_outlined, size: 20),
              tooltip: t(context, 'agent.newSession'),
              visualDensity: VisualDensity.compact,
              onPressed: _s.events.isEmpty ? null : _confirmNewSession,
            ),
            IconButton(
              icon: const Icon(Icons.tune, size: 20),
              tooltip: t(context, 'agent.settings.title'),
              visualDensity: VisualDensity.compact,
              onPressed: () => Navigator.push(
                context,
                MaterialPageRoute(
                    builder: (_) => const AgentSettingsScreen()),
              ),
            ),
          ],
        ],
      ),
    );
  }

  Future<void> _confirmNewSession() async {
    final ok = await showDialog<bool>(
      context: context,
      builder: (ctx) => AlertDialog(
        title: Text(t(ctx, 'agent.newSession')),
        content: Text(t(ctx, 'agent.newSession.body')),
        actions: [
          TextButton(
              onPressed: () => Navigator.pop(ctx, false),
              child: Text(t(ctx, 'agent.confirm.cancel'))),
          FilledButton(
              onPressed: () => Navigator.pop(ctx, true),
              child: Text(t(ctx, 'agent.newSession'))),
        ],
      ),
    );
    if (ok == true) await _s.clear();
  }

  /// Always reachable once anything has been written to the VESC. Stopping is
  /// the safe direction and never asks for confirmation.
  Widget _stopScriptBar(BuildContext context) {
    final scheme = Theme.of(context).colorScheme;
    return Padding(
      padding: const EdgeInsets.fromLTRB(12, 0, 12, 4),
      child: SizedBox(
        width: double.infinity,
        child: OutlinedButton.icon(
          icon: const Icon(Icons.dangerous_outlined),
          label: Text(t(context, 'agent.stopScript')),
          style: OutlinedButton.styleFrom(
            foregroundColor: scheme.error,
            side: BorderSide(color: scheme.error),
          ),
          onPressed: () => unawaited(_s.emergencyStop()),
        ),
      ),
    );
  }

  Widget _composer(BuildContext context) {
    final busy = _s.busy;
    return SafeArea(
      top: false,
      child: Padding(
        padding: const EdgeInsets.fromLTRB(12, 0, 12, 8),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            if (busy)
              Padding(
                padding: const EdgeInsets.only(bottom: 4),
                child: Text(
                  switch (_s.state) {
                    AgentRunState.cancelling => t(context, 'agent.stopping'),
                    AgentRunState.paused => t(context, 'agent.paused.bg'),
                    AgentRunState.awaitingUser =>
                      t(context, 'agent.confirm.approve'),
                    _ => t(context, 'agent.working'),
                  },
                  style: TextStyle(
                      fontSize: 11,
                      color: Theme.of(context).colorScheme.outline),
                ),
              ),
            Row(
              crossAxisAlignment: CrossAxisAlignment.end,
              children: [
                Expanded(
                  child: TextField(
                    controller: _input,
                    enabled: !busy,
                    minLines: 1,
                    maxLines: 5,
                    textInputAction: TextInputAction.newline,
                    decoration: InputDecoration(
                      hintText: t(context, 'agent.hint'),
                      border: const OutlineInputBorder(),
                      isDense: true,
                    ),
                  ),
                ),
                const SizedBox(width: 8),
                IconButton.filled(
                  icon: const Icon(Icons.send),
                  tooltip: t(context, 'agent.send'),
                  onPressed: busy ? null : _send,
                ),
              ],
            ),
          ],
        ),
      ),
    );
  }
}
