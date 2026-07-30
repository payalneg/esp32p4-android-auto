/// Settings for the AI assistant: provider, key, model and the per-session
/// limits that bound what it can spend and how many times it may reflash the
/// VESC.
library;

import 'package:flutter/material.dart';
import 'package:flutter/services.dart';

import '../agent/ai_models.dart';
import '../i18n/strings.dart';
import '../settings/agent_settings.dart';

class AgentSettingsScreen extends StatefulWidget {
  const AgentSettingsScreen({super.key});

  @override
  State<AgentSettingsScreen> createState() => _AgentSettingsScreenState();
}

class _AgentSettingsScreenState extends State<AgentSettingsScreen> {
  final _s = AgentSettings.instance;
  late final TextEditingController _key;
  late final TextEditingController _model;
  late final TextEditingController _baseUrl;
  bool _showKey = false;

  @override
  void initState() {
    super.initState();
    _key = TextEditingController(text: _s.apiKey ?? '');
    _model = TextEditingController(text: _s.model);
    _baseUrl = TextEditingController(text: _s.baseUrl);
    _s.addListener(_onSettings);
  }

  @override
  void dispose() {
    _s.removeListener(_onSettings);
    _key.dispose();
    _model.dispose();
    _baseUrl.dispose();
    super.dispose();
  }

  /// Switching provider rewrites model + endpoint to that provider's
  /// defaults, so the fields have to follow.
  void _onSettings() {
    if (!mounted) return;
    if (_model.text != _s.model) _model.text = _s.model;
    if (_baseUrl.text != _s.baseUrl) _baseUrl.text = _s.baseUrl;
    setState(() {});
  }

  void _snack(String key) {
    if (!mounted) return;
    ScaffoldMessenger.of(context)
        .showSnackBar(SnackBar(content: Text(t(context, key))));
  }

  Future<void> _saveKey() async {
    final v = _key.text.trim();
    FocusScope.of(context).unfocus();
    await _s.setApiKey(v);
    if (!mounted) return;
    _snack(v.isEmpty ? 'agent.settings.key.cleared' : 'agent.settings.key.set');
  }

  @override
  Widget build(BuildContext context) {
    final theme = Theme.of(context);
    final small = theme.textTheme.bodySmall
        ?.copyWith(color: theme.colorScheme.outline);

    return Scaffold(
      appBar: AppBar(title: Text(t(context, 'agent.settings.title'))),
      body: ListView(
        padding: const EdgeInsets.all(16),
        children: [
          Card(
            child: Padding(
              padding: const EdgeInsets.all(16),
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  Text(t(context, 'agent.settings.provider'),
                      style: theme.textTheme.titleSmall),
                  const SizedBox(height: 8),
                  SegmentedButton<AiProvider>(
                    segments: [
                      for (final p in AiProvider.values)
                        ButtonSegment(value: p, label: Text(p.label)),
                    ],
                    selected: {_s.provider},
                    showSelectedIcon: false,
                    onSelectionChanged: (v) => _s.setProvider(v.first),
                  ),
                  if (_s.provider.keyUrl.isNotEmpty) ...[
                    const SizedBox(height: 8),
                    Text(
                        t(context, 'agent.settings.keyFrom')
                            .replaceAll('{url}', _s.provider.keyUrl),
                        style: small),
                  ],
                ],
              ),
            ),
          ),
          // Short how-to first: without a key nothing on this screen matters.
          if (!_s.configured && _s.provider.keyUrl.isNotEmpty)
            Card(
              color: theme.colorScheme.surfaceContainerHigh,
              child: Padding(
                padding: const EdgeInsets.all(16),
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    Text(t(context, 'agent.settings.howto'),
                        style: theme.textTheme.titleSmall),
                    const SizedBox(height: 10),
                    for (var i = 1; i <= 4; i++)
                      Padding(
                        padding: const EdgeInsets.only(bottom: 8),
                        child: Row(
                          crossAxisAlignment: CrossAxisAlignment.start,
                          children: [
                            Text('$i.  ',
                                style: theme.textTheme.bodySmall?.copyWith(
                                    fontWeight: FontWeight.bold)),
                            Expanded(
                              child: Text(
                                t(context, 'agent.settings.howto.$i')
                                    .replaceAll('{url}', _s.provider.keyUrl),
                                style: theme.textTheme.bodySmall
                                    ?.copyWith(height: 1.4),
                              ),
                            ),
                          ],
                        ),
                      ),
                    InkWell(
                      onTap: () async {
                        await Clipboard.setData(
                            ClipboardData(text: 'https://${_s.provider.keyUrl}'));
                        if (!context.mounted) return;
                        ScaffoldMessenger.of(context).showSnackBar(SnackBar(
                            content: Text(
                                t(context, 'agent.settings.howto.copied'))));
                      },
                      child: Padding(
                        padding: const EdgeInsets.symmetric(vertical: 6),
                        child: Row(
                          children: [
                            Icon(Icons.copy,
                                size: 14, color: theme.colorScheme.primary),
                            const SizedBox(width: 6),
                            Flexible(
                              child: Text(
                                _s.provider.keyUrl,
                                style: TextStyle(
                                    fontSize: 13,
                                    fontFamily: 'monospace',
                                    color: theme.colorScheme.primary),
                              ),
                            ),
                          ],
                        ),
                      ),
                    ),
                    Text(t(context, 'agent.settings.howto.tapCopy'),
                        style: small),
                  ],
                ),
              ),
            ),
          Card(
            child: Padding(
              padding: const EdgeInsets.all(16),
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  TextField(
                    controller: _key,
                    obscureText: !_showKey,
                    autocorrect: false,
                    enableSuggestions: false,
                    decoration: InputDecoration(
                      labelText: t(context, 'agent.settings.key'),
                      hintText: t(context, 'agent.settings.key.hint'),
                      border: const OutlineInputBorder(),
                      suffixIcon: Row(
                        mainAxisSize: MainAxisSize.min,
                        children: [
                          IconButton(
                            icon: Icon(_showKey
                                ? Icons.visibility_off
                                : Icons.visibility),
                            onPressed: () =>
                                setState(() => _showKey = !_showKey),
                          ),
                          IconButton(
                            icon: const Icon(Icons.check),
                            onPressed: _saveKey,
                          ),
                        ],
                      ),
                    ),
                    onSubmitted: (_) => _saveKey(),
                  ),
                  const SizedBox(height: 8),
                  Row(
                    crossAxisAlignment: CrossAxisAlignment.start,
                    children: [
                      Icon(
                          _s.usingEmbeddedKey
                              ? Icons.lock_open
                              : Icons.check_circle_outline,
                          size: 14,
                          color: _s.usingEmbeddedKey
                              ? theme.colorScheme.tertiary
                              : theme.colorScheme.primary),
                      const SizedBox(width: 6),
                      Expanded(
                        child: Text(
                          t(
                              context,
                              _s.usingEmbeddedKey
                                  ? 'agent.settings.key.embedded'
                                  : 'agent.settings.key.own'),
                          style: small?.copyWith(
                              color: _s.usingEmbeddedKey
                                  ? theme.colorScheme.tertiary
                                  : theme.colorScheme.primary),
                        ),
                      ),
                    ],
                  ),
                  Text(t(context, 'agent.settings.key.stored'), style: small),
                  Text(t(context, 'agent.settings.key.tip'), style: small),
                  SwitchListTile(
                    contentPadding: EdgeInsets.zero,
                    title: Text(t(context, 'agent.settings.key.remember')),
                    subtitle: _s.rememberKey
                        ? null
                        : Text(t(context, 'agent.settings.key.rememberOff'),
                            style: small),
                    value: _s.rememberKey,
                    onChanged: (v) => _s.setRememberKey(v),
                  ),
                ],
              ),
            ),
          ),
          Card(
            child: Padding(
              padding: const EdgeInsets.all(16),
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  TextField(
                    controller: _model,
                    autocorrect: false,
                    decoration: InputDecoration(
                      labelText: t(context, 'agent.settings.model'),
                      border: const OutlineInputBorder(),
                    ),
                    onChanged: _s.setModel,
                  ),
                  const SizedBox(height: 8),
                  // Free text with shortcuts: model ids churn, and a hardcoded
                  // dropdown would rot within months.
                  Wrap(
                    spacing: 8,
                    children: [
                      if (_s.provider.defaultModel.isNotEmpty)
                        ActionChip(
                          label: Text(t(context, 'agent.settings.model.fast')),
                          onPressed: () {
                            _model.text = _s.provider.defaultModel;
                            _s.setModel(_model.text);
                          },
                        ),
                      if (_s.provider.strongModel.isNotEmpty)
                        ActionChip(
                          label:
                              Text(t(context, 'agent.settings.model.strong')),
                          onPressed: () {
                            _model.text = _s.provider.strongModel;
                            _s.setModel(_model.text);
                          },
                        ),
                    ],
                  ),
                  const SizedBox(height: 8),
                  Text(
                    _s.provider == AiProvider.openrouter
                        ? t(context, 'agent.settings.pricing.provider')
                        : (_s.pricing == null
                            ? '—'
                            : t(context, 'agent.settings.pricing')
                                .replaceAll('{date}', _s.pricing!.updated)),
                    style: small,
                  ),
                  if (_s.spendTotal > 0)
                    Text(
                        t(context, 'agent.settings.spendTotal').replaceAll(
                            '{usd}', '\$${_s.spendTotal.toStringAsFixed(3)}'),
                        style: small),
                ],
              ),
            ),
          ),
          Card(
            child: ExpansionTile(
              title: Text(t(context, 'agent.settings.advanced')),
              childrenPadding: const EdgeInsets.fromLTRB(16, 0, 16, 16),
              children: [
                TextField(
                  controller: _baseUrl,
                  autocorrect: false,
                  keyboardType: TextInputType.url,
                  decoration: InputDecoration(
                    labelText: t(context, 'agent.settings.baseUrl'),
                    border: const OutlineInputBorder(),
                  ),
                  onChanged: _s.setBaseUrl,
                ),
                SwitchListTile(
                  contentPadding: EdgeInsets.zero,
                  title: Text(t(context, 'agent.settings.strict')),
                  subtitle:
                      Text(t(context, 'agent.settings.strict.desc'), style: small),
                  value: _s.strict,
                  onChanged: (v) => _s.setStrict(v),
                ),
                SwitchListTile(
                  contentPadding: EdgeInsets.zero,
                  title: Text(t(context, 'agent.settings.thinking')),
                  subtitle: Text(t(context, 'agent.settings.thinking.desc'),
                      style: small),
                  value: _s.thinking,
                  onChanged: (v) => _s.setThinking(v),
                ),
                _slider(
                  label: t(context, 'agent.settings.temperature'),
                  value: _s.temperature,
                  min: 0,
                  max: 1.5,
                  divisions: 15,
                  format: (v) => v.toStringAsFixed(1),
                  onChanged: _s.setTemperature,
                ),
              ],
            ),
          ),
          Card(
            child: Padding(
              padding: const EdgeInsets.all(16),
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  Text(t(context, 'agent.settings.budget'),
                      style: theme.textTheme.titleSmall),
                  _slider(
                    label: t(context, 'agent.settings.maxSteps'),
                    value: _s.maxSteps.toDouble(),
                    min: 4,
                    max: 60,
                    divisions: 28,
                    format: (v) => v.round().toString(),
                    onChanged: (v) => _s.setMaxSteps(v.round()),
                  ),
                  _slider(
                    label: t(context, 'agent.settings.maxFlashes'),
                    value: _s.maxFlashes.toDouble(),
                    min: 0,
                    max: 20,
                    divisions: 20,
                    format: (v) => v.round().toString(),
                    onChanged: (v) => _s.setMaxFlashes(v.round()),
                  ),
                  _slider(
                    label: t(context, 'agent.settings.spendCap'),
                    value: _s.spendCap,
                    min: 0,
                    max: 5,
                    divisions: 20,
                    format: (v) => v == 0
                        ? t(context, 'agent.settings.spendCap.off')
                        : '\$${v.toStringAsFixed(2)}',
                    onChanged: _s.setSpendCap,
                  ),
                ],
              ),
            ),
          ),
        ],
      ),
    );
  }

  Widget _slider({
    required String label,
    required double value,
    required double min,
    required double max,
    required int divisions,
    required String Function(double) format,
    required void Function(double) onChanged,
  }) {
    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        Row(
          mainAxisAlignment: MainAxisAlignment.spaceBetween,
          children: [
            Text(label, style: Theme.of(context).textTheme.bodyMedium),
            Text(format(value), style: Theme.of(context).textTheme.bodySmall),
          ],
        ),
        Slider(
          value: value.clamp(min, max),
          min: min,
          max: max,
          divisions: divisions,
          onChanged: (v) => setState(() => onChanged(v)),
        ),
      ],
    );
  }
}
