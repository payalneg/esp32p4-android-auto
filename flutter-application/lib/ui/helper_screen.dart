/// Configurator for the VESC BLE Helper (XIAO ESP32-C3) — the board that
/// bridges BLE buttons and a cadence sensor to the VESC over CAN.
///
/// Ported from the helper's own app. The BLE work happens in the background
/// isolate ([HelperService]); this screen drives it through [HelperProxy], so
/// the head-unit link stays up while the helper is being configured.
///
/// Not here: the helper's LISP tab. The helper exposes a plain Nordic UART
/// service, so the app's own LISP editor already talks to the VESC through it
/// — pick the helper as the adapter there.
library;

import 'dart:async';
import 'dart:math' as math;
import 'dart:typed_data';

import 'package:flutter/material.dart';

import '../helper/helper_firmware.dart';
import '../helper/helper_protocol.dart';
import '../helper/helper_proxy.dart';
import '../helper/helper_service.dart' show HelperConnState;
import '../i18n/strings.dart';

class HelperScreen extends StatefulWidget {
  const HelperScreen({super.key});

  @override
  State<HelperScreen> createState() => _HelperScreenState();
}

class _HelperScreenState extends State<HelperScreen>
    with SingleTickerProviderStateMixin {
  final _h = HelperProxy.instance;
  late final TabController _tab;
  bool _logOpen = false;

  @override
  void initState() {
    super.initState();
    _tab = TabController(length: 4, vsync: this);
    _h.addListener(_onChange);
    // The isolate only pushes on change, so a screen opened after the helper
    // was already up would otherwise paint "not connected".
    unawaited(_h.refresh().catchError((_) {}));
  }

  @override
  void dispose() {
    _h.removeListener(_onChange);
    _tab.dispose();
    super.dispose();
  }

  void _onChange() {
    if (mounted) setState(() {});
  }

  void _snack(String msg) {
    if (!mounted) return;
    ScaffoldMessenger.of(context).showSnackBar(SnackBar(content: Text(msg)));
  }

  Future<void> _toggleConnection() async {
    try {
      if (_h.connected) {
        await _h.disconnect();
      } else {
        await _h.connect();
        if (!_h.connected && mounted) _snack(t(context, 'helper.notFound'));
      }
    } catch (e) {
      _snack('$e');
    }
  }

  @override
  Widget build(BuildContext context) {
    final busy = _h.scanning || _h.state == HelperConnState.connecting;
    return Scaffold(
      appBar: AppBar(
        title: Text(t(context, 'helper.title')),
        actions: [
          IconButton(
            icon: Icon(_h.connected ? Icons.link_off : Icons.link),
            tooltip: t(context,
                _h.connected ? 'helper.disconnect' : 'helper.connect'),
            onPressed: busy ? null : _toggleConnection,
          ),
        ],
        bottom: TabBar(
          controller: _tab,
          isScrollable: true,
          tabs: [
            Tab(text: t(context, 'helper.tab.status')),
            Tab(text: t(context, 'helper.tab.params')),
            Tab(text: t(context, 'helper.tab.bind')),
            Tab(text: t(context, 'helper.tab.fw')),
          ],
        ),
      ),
      body: Column(
        children: [
          _linkBar(context, busy),
          if (busy) const LinearProgressIndicator(),
          if (_h.otaProgress != null)
            LinearProgressIndicator(value: _h.otaProgress),
          Expanded(
            child: TabBarView(
              controller: _tab,
              children: [
                _StatusTab(h: _h),
                _ParamsTab(h: _h),
                _BindTab(h: _h),
                _FirmwareTab(h: _h),
              ],
            ),
          ),
          _logPanel(context),
        ],
      ),
    );
  }

  Widget _linkBar(BuildContext context, bool busy) {
    final scheme = Theme.of(context).colorScheme;
    final text = switch (_h.state) {
      HelperConnState.connected => _h.fwVersion == null
          ? t(context, 'helper.connected')
          : '${t(context, 'helper.connected')} · '
              '${t(context, 'helper.fw').replaceAll('{v}', _h.fwVersion!)}',
      HelperConnState.connecting => t(context, 'helper.connecting'),
      HelperConnState.scanning => t(context, 'helper.searching'),
      HelperConnState.idle => t(context, 'helper.disconnected'),
    };
    return Container(
      width: double.infinity,
      color: scheme.surfaceContainerHighest,
      padding: const EdgeInsets.symmetric(horizontal: 12, vertical: 8),
      child: Row(
        children: [
          Icon(
            _h.connected ? Icons.bluetooth_connected : Icons.bluetooth_disabled,
            size: 18,
            color: _h.connected ? scheme.primary : scheme.outline,
          ),
          const SizedBox(width: 8),
          Expanded(child: Text(text, style: Theme.of(context).textTheme.bodySmall)),
          if (!_h.connected && !busy)
            TextButton(
              onPressed: _toggleConnection,
              child: Text(t(context, 'helper.connect')),
            ),
        ],
      ),
    );
  }

  Widget _logPanel(BuildContext context) {
    if (_h.log.isEmpty) return const SizedBox.shrink();
    final scheme = Theme.of(context).colorScheme;
    return SafeArea(
      top: false,
      child: Theme(
        data: Theme.of(context).copyWith(dividerColor: Colors.transparent),
        child: ExpansionTile(
          dense: true,
          initiallyExpanded: _logOpen,
          onExpansionChanged: (v) => _logOpen = v,
          title: Text('${t(context, 'helper.log')} · ${_h.log.last}',
              maxLines: 1,
              overflow: TextOverflow.ellipsis,
              style: TextStyle(fontSize: 12, color: scheme.outline)),
          children: [
            ConstrainedBox(
              constraints: const BoxConstraints(maxHeight: 160),
              child: ListView(
                padding: const EdgeInsets.symmetric(horizontal: 12),
                children: [
                  for (final l in _h.log.reversed.take(80))
                    Text(l,
                        style: const TextStyle(
                            fontFamily: 'monospace', fontSize: 11)),
                ],
              ),
            ),
          ],
        ),
      ),
    );
  }
}

// ---------------------------------------------------------------------------
// Status
// ---------------------------------------------------------------------------

class _StatusTab extends StatelessWidget {
  const _StatusTab({required this.h});
  final HelperProxy h;

  static const _maxRpm = 120.0;

  @override
  Widget build(BuildContext context) {
    final s = h.status;
    if (s == null) {
      return _Empty(
        icon: h.connected ? Icons.hourglass_top : Icons.bluetooth_disabled,
        title: t(context,
            h.connected ? 'helper.status.waiting' : 'helper.disconnected'),
        hint: t(context, 'helper.intro'),
      );
    }
    final scheme = Theme.of(context).colorScheme;
    return ListView(
      padding: const EdgeInsets.all(12),
      children: [
        Card(
          child: Padding(
            padding: const EdgeInsets.all(16),
            child: Column(
              children: [
                Row(
                  children: [
                    Text(t(context, 'helper.status.cadence'),
                        style: Theme.of(context).textTheme.titleSmall),
                    const Spacer(),
                    if (!s.cadenceConnected)
                      Text(t(context, 'helper.status.offline'),
                          style: TextStyle(
                              fontSize: 12, color: scheme.outline)),
                  ],
                ),
                SizedBox(
                  height: 180,
                  child: CustomPaint(
                    painter: _GaugePainter(
                      fraction: s.cadenceConnected
                          ? (s.rpm.clamp(0.0, _maxRpm)) / _maxRpm
                          : 0,
                      track: s.cadenceConnected
                          ? scheme.primaryContainer
                          : scheme.surfaceContainerHighest,
                      fill: scheme.primary,
                    ),
                    child: Center(
                      child: Column(
                        mainAxisSize: MainAxisSize.min,
                        children: [
                          Text(
                            s.cadenceConnected ? '${s.rpm.round()}' : '—',
                            style: TextStyle(
                              fontSize: 52,
                              height: 1,
                              fontWeight: FontWeight.w600,
                              color: s.cadenceConnected
                                  ? scheme.onSurface
                                  : scheme.outline,
                            ),
                          ),
                          Text(t(context, 'helper.status.rpm'),
                              style: TextStyle(
                                  letterSpacing: 2, color: scheme.outline)),
                        ],
                      ),
                    ),
                  ),
                ),
              ],
            ),
          ),
        ),
        Row(
          children: [
            Expanded(
                child: _Stat(
                    label: t(context, 'helper.status.assist'),
                    value: s.assistA.toStringAsFixed(1),
                    unit: 'A')),
            const SizedBox(width: 8),
            Expanded(
                child: _Stat(
                    label: t(context, 'helper.status.level'),
                    value: '${s.level}')),
            const SizedBox(width: 8),
            Expanded(
                child: _Stat(
                    label: t(context, 'helper.status.battery'),
                    value: s.battKnown ? '${s.batt}' : '—',
                    unit: s.battKnown ? '%' : null)),
          ],
        ),
        const SizedBox(height: 8),
        Card(
          child: Padding(
            padding: const EdgeInsets.all(12),
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Text(t(context, 'helper.status.links'),
                    style: Theme.of(context).textTheme.titleSmall),
                const SizedBox(height: 8),
                Wrap(
                  spacing: 8,
                  runSpacing: 8,
                  children: [
                    _chip(
                      context,
                      Icons.pedal_bike,
                      t(context, 'helper.status.sensor'),
                      !s.cadenceBound
                          ? t(context, 'helper.status.notBound')
                          : s.cadenceConnected
                              ? t(context, 'helper.status.online')
                              : t(context, 'helper.status.offline'),
                      !s.cadenceBound ? null : s.cadenceConnected,
                    ),
                    _chip(
                      context,
                      Icons.settings_remote,
                      t(context, 'helper.status.remote'),
                      !s.remoteBound
                          ? t(context, 'helper.status.notBound')
                          : s.remoteConnected
                              ? t(context, 'helper.status.online')
                              : t(context, 'helper.status.offline'),
                      !s.remoteBound ? null : s.remoteConnected,
                    ),
                    _chip(
                        context,
                        Icons.cable,
                        t(context, 'helper.status.vesc'),
                        s.vescLink
                            ? t(context, 'helper.status.linkOk')
                            : t(context, 'helper.status.noData'),
                        s.vescLink),
                    _chip(
                        context,
                        Icons.directions_bike,
                        t(context, 'helper.status.pas'),
                        s.pasEnabled
                            ? t(context, 'helper.status.on')
                            : t(context, 'helper.status.off'),
                        s.pasEnabled ? true : null),
                  ],
                ),
              ],
            ),
          ),
        ),
        Card(
          child: Padding(
            padding: const EdgeInsets.all(12),
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Text(t(context, 'helper.status.buttons'),
                    style: Theme.of(context).textTheme.titleSmall),
                const SizedBox(height: 8),
                if (s.btnCount == 0)
                  Text(t(context, 'helper.status.noButtons'),
                      style:
                          TextStyle(fontSize: 12, color: scheme.outline))
                else
                  Wrap(
                    spacing: 8,
                    runSpacing: 8,
                    children: [
                      for (var i = 0; i < math.min(s.btnCount, kBtnUiSlots); i++)
                        Container(
                          width: 38,
                          height: 38,
                          decoration: BoxDecoration(
                            color: s.btnMask & (1 << i) != 0
                                ? scheme.primary
                                : scheme.surfaceContainerHighest,
                            borderRadius: BorderRadius.circular(10),
                          ),
                          child: Center(
                            child: Text(
                              String.fromCharCode(65 + i),
                              style: TextStyle(
                                fontWeight: FontWeight.w600,
                                color: s.btnMask & (1 << i) != 0
                                    ? scheme.onPrimary
                                    : scheme.outline,
                              ),
                            ),
                          ),
                        ),
                    ],
                  ),
              ],
            ),
          ),
        ),
        Card(
          child: Padding(
            padding: const EdgeInsets.all(12),
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Text(t(context, 'helper.status.throttle'),
                    style: Theme.of(context).textTheme.titleSmall),
                const SizedBox(height: 8),
                Wrap(
                  spacing: 8,
                  crossAxisAlignment: WrapCrossAlignment.center,
                  children: [
                    SegmentedButton<bool>(
                      segments: [
                        ButtonSegment(
                            value: true,
                            label: Text(t(context, 'helper.status.on')),
                            icon: const Icon(Icons.bolt)),
                        ButtonSegment(
                            value: false,
                            label: Text(t(context, 'helper.status.off')),
                            icon: const Icon(Icons.power_settings_new)),
                      ],
                      selected: {s.throttleOn},
                      showSelectedIcon: false,
                      onSelectionChanged: h.connected
                          ? (sel) => h.throttle(sel.first ? 1 : 0)
                          : null,
                    ),
                    OutlinedButton(
                      onPressed: h.connected ? () => h.throttle(0xFF) : null,
                      child: Text(t(context, 'helper.status.toggle')),
                    ),
                  ],
                ),
                if (!s.vescLink) ...[
                  const SizedBox(height: 8),
                  Row(
                    children: [
                      Icon(Icons.warning_amber, size: 16, color: scheme.error),
                      const SizedBox(width: 6),
                      Expanded(
                        child: Text(t(context, 'helper.status.noVescWarn'),
                            style: TextStyle(
                                fontSize: 12, color: scheme.error)),
                      ),
                    ],
                  ),
                ],
              ],
            ),
          ),
        ),
      ],
    );
  }

  /// [state]: true ok, false problem, null "not configured".
  Widget _chip(BuildContext context, IconData icon, String label,
      String detail, bool? state) {
    final scheme = Theme.of(context).colorScheme;
    final (bg, fg) = switch (state) {
      true => (scheme.secondaryContainer, scheme.onSecondaryContainer),
      false => (scheme.errorContainer, scheme.onErrorContainer),
      null => (scheme.surfaceContainerHighest, scheme.outline),
    };
    return Container(
      padding: const EdgeInsets.symmetric(horizontal: 10, vertical: 6),
      decoration:
          BoxDecoration(color: bg, borderRadius: BorderRadius.circular(20)),
      child: Row(
        mainAxisSize: MainAxisSize.min,
        children: [
          Icon(icon, size: 16, color: fg),
          const SizedBox(width: 6),
          Text('$label · $detail',
              style: TextStyle(
                  fontSize: 12, color: fg, fontWeight: FontWeight.w600)),
        ],
      ),
    );
  }
}

class _Stat extends StatelessWidget {
  const _Stat({required this.label, required this.value, this.unit});
  final String label;
  final String value;
  final String? unit;

  @override
  Widget build(BuildContext context) {
    final scheme = Theme.of(context).colorScheme;
    return Card(
      child: Padding(
        padding: const EdgeInsets.all(12),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Text(label,
                maxLines: 1,
                overflow: TextOverflow.ellipsis,
                style: TextStyle(fontSize: 11, color: scheme.outline)),
            const SizedBox(height: 4),
            Text.rich(TextSpan(
              text: value,
              style: const TextStyle(fontSize: 22, fontWeight: FontWeight.w600),
              children: [
                if (unit != null)
                  TextSpan(
                      text: ' $unit',
                      style: TextStyle(fontSize: 12, color: scheme.outline)),
              ],
            )),
          ],
        ),
      ),
    );
  }
}

class _GaugePainter extends CustomPainter {
  const _GaugePainter(
      {required this.fraction, required this.track, required this.fill});
  final double fraction;
  final Color track;
  final Color fill;

  static const _stroke = 16.0;
  static final _start = 135 * math.pi / 180;
  static final _sweep = 270 * math.pi / 180;

  @override
  void paint(Canvas canvas, Size size) {
    final side = math.min(size.width, size.height);
    final rect = Rect.fromCenter(
      center: Offset(size.width / 2, size.height / 2),
      width: side - _stroke,
      height: side - _stroke,
    );
    final paint = Paint()
      ..style = PaintingStyle.stroke
      ..strokeWidth = _stroke
      ..strokeCap = StrokeCap.round;
    canvas.drawArc(rect, _start, _sweep, false, paint..color = track);
    final f = fraction.clamp(0.0, 1.0);
    if (f > 0.004) {
      canvas.drawArc(rect, _start, _sweep * f, false, paint..color = fill);
    }
  }

  @override
  bool shouldRepaint(_GaugePainter old) =>
      old.fraction != fraction || old.track != track || old.fill != fill;
}

// ---------------------------------------------------------------------------
// Parameters
// ---------------------------------------------------------------------------

class _ParamsTab extends StatefulWidget {
  const _ParamsTab({required this.h});
  final HelperProxy h;

  @override
  State<_ParamsTab> createState() => _ParamsTabState();
}

class _ParamsTabState extends State<_ParamsTab>
    with AutomaticKeepAliveClientMixin {
  final _startCurrent = TextEditingController();
  final _startDelay = TextEditingController();
  final _stopDelay = TextEditingController();
  final _minCadence = TextEditingController();
  final _fullCadence = TextEditingController();
  final _maxCurrent = TextEditingController();
  final _ramp = TextEditingController();
  final _ctrlId = TextEditingController();
  final _tgtId = TextEditingController();
  final _btnId = <TextEditingController>[];
  final _btnData = <TextEditingController>[];

  bool _enabled = true;
  bool _reverse = false;
  int _mode = 0;
  int _level = 1;
  int _levelCount = 3;
  int _kbps = 500;

  /// Which params revision the fields were filled from — so live pushes don't
  /// overwrite half-typed edits, but a fresh read does refill.
  Object? _filledFrom;

  @override
  bool get wantKeepAlive => true;

  @override
  void initState() {
    super.initState();
    for (var i = 0; i < kBtnUiSlots; i++) {
      _btnId.add(TextEditingController(text: '123'));
      _btnData.add(TextEditingController(
          text: (i + 1).toRadixString(16).padLeft(4, '0').toUpperCase()));
    }
    widget.h.addListener(_sync);
    _sync();
  }

  @override
  void dispose() {
    widget.h.removeListener(_sync);
    for (final c in [
      _startCurrent,
      _startDelay,
      _stopDelay,
      _minCadence,
      _fullCadence,
      _maxCurrent,
      _ramp,
      _ctrlId,
      _tgtId,
      ..._btnId,
      ..._btnData,
    ]) {
      c.dispose();
    }
    super.dispose();
  }

  void _sync() {
    final p = widget.h.params;
    if (p != null && !identical(p, _filledFrom)) {
      _filledFrom = p;
      _fill(p);
    }
    for (final b in widget.h.bindings.values) {
      if (b.idx >= kBtnUiSlots) continue;
      _btnId[b.idx].text = b.canId.toRadixString(16).toUpperCase();
      _btnData[b.idx].text = hexBytes(b.data);
    }
    if (mounted) setState(() {});
  }

  void _fill(PasParams p) {
    _startCurrent.text = '${p.startCurrentPct}';
    _startDelay.text = '${p.startDelayMs}';
    _stopDelay.text = '${p.stopDelayMs}';
    _minCadence.text = '${p.minCadenceRpm}';
    _fullCadence.text = '${p.fullCadenceRpm}';
    _maxCurrent.text = '${p.maxCurrentMa / 1000.0}';
    _ramp.text = '${p.rampUpMaps / 1000.0}';
    _ctrlId.text = '${p.controllerId}';
    _tgtId.text = '${p.targetVescId}';
    _kbps = p.canKbps;
    _enabled = p.enabled != 0;
    _reverse = p.reverse != 0;
    _mode = p.mode == 1 ? 1 : 0;
    _levelCount = p.levelCount.clamp(1, 9);
    _level = p.level.clamp(0, _levelCount);
  }

  Future<void> _write() async {
    try {
      int gi(TextEditingController c) => int.parse(c.text.trim());
      double gd(TextEditingController c) => double.parse(c.text.trim());

      final params = PasParams()
        ..enabled = _enabled ? 1 : 0
        ..reverse = _reverse ? 1 : 0
        ..level = _level
        ..levelCount = _levelCount
        ..mode = _mode
        ..startCurrentPct = gi(_startCurrent)
        ..startDelayMs = gi(_startDelay)
        ..stopDelayMs = gi(_stopDelay)
        ..minCadenceRpm = gi(_minCadence)
        ..fullCadenceRpm = gi(_fullCadence)
        ..maxCurrentMa = (gd(_maxCurrent) * 1000).round()
        ..rampUpMaps = (gd(_ramp) * 1000).round()
        ..controllerId = gi(_ctrlId)
        ..targetVescId = gi(_tgtId)
        ..canKbps = _kbps;

      final bindings = <ButtonBinding>[];
      for (var i = 0; i < kBtnUiSlots; i++) {
        final canId = int.parse(_btnId[i].text.trim(), radix: 16);
        var data = bytesFromHex(_btnData[i].text);
        if (data.length > 8) data = data.sublist(0, 8);
        // CAN 2.0B is needed above the 11-bit standard id range.
        bindings.add(ButtonBinding(i, canId > 0x7FF ? 1 : 0, canId, data));
      }

      await widget.h.setParams(params);
      for (final b in bindings) {
        await widget.h.setBinding(b);
      }
      if (mounted) {
        ScaffoldMessenger.of(context).showSnackBar(SnackBar(
            content: Text(t(context, 'helper.params.written'))));
      }
    } on FormatException {
      if (mounted) {
        ScaffoldMessenger.of(context).showSnackBar(SnackBar(
            content: Text(t(context, 'helper.params.badValue'))));
      }
    } catch (e) {
      if (mounted) {
        ScaffoldMessenger.of(context)
            .showSnackBar(SnackBar(content: Text('$e')));
      }
    }
  }

  Widget _num(TextEditingController c, String label,
      {double width = 165, bool hex = false}) {
    return SizedBox(
      width: width,
      child: TextField(
        controller: c,
        keyboardType: hex
            ? TextInputType.text
            : const TextInputType.numberWithOptions(decimal: true),
        decoration: InputDecoration(
            labelText: label, isDense: true, border: const OutlineInputBorder()),
      ),
    );
  }

  @override
  Widget build(BuildContext context) {
    super.build(context);
    final h = widget.h;
    if (!h.connected) {
      return _Empty(
          icon: Icons.bluetooth_disabled,
          title: t(context, 'helper.disconnected'),
          hint: t(context, 'helper.intro'));
    }
    if (h.params == null) {
      return _Empty(
          icon: Icons.hourglass_top,
          title: t(context, 'helper.params.waiting'),
          hint: '');
    }
    final outline = Theme.of(context).colorScheme.outline;

    return ListView(
      padding: const EdgeInsets.all(12),
      children: [
        _section(context, t(context, 'helper.params.can'), [
          Wrap(spacing: 8, runSpacing: 8, children: [
            _num(_ctrlId, t(context, 'helper.params.helperId'), width: 110),
            _num(_tgtId, t(context, 'helper.params.vescId'), width: 110),
            SizedBox(
              width: 150,
              child: DropdownButtonFormField<int>(
                value: _kbps,
                decoration: InputDecoration(
                    labelText: t(context, 'helper.params.bitrate'),
                    isDense: true,
                    border: const OutlineInputBorder()),
                items: [
                  for (final s in kCanSpeeds)
                    DropdownMenuItem(value: s, child: Text('$s')),
                ],
                onChanged: (v) => setState(() => _kbps = v ?? 500),
              ),
            ),
          ]),
        ]),
        _section(context, t(context, 'helper.params.pas'), [
          SwitchListTile(
            dense: true,
            contentPadding: EdgeInsets.zero,
            title: Text(t(context, 'helper.params.enabled')),
            value: _enabled,
            onChanged: (v) => setState(() => _enabled = v),
          ),
          SwitchListTile(
            dense: true,
            contentPadding: EdgeInsets.zero,
            title: Text(t(context, 'helper.params.reverse')),
            subtitle: Text(t(context, 'helper.params.reverse.desc'),
                style: TextStyle(fontSize: 11, color: outline)),
            value: _reverse,
            onChanged: (v) => setState(() => _reverse = v),
          ),
          const SizedBox(height: 8),
          Wrap(spacing: 8, runSpacing: 8, children: [
            SizedBox(
              width: 230,
              child: DropdownButtonFormField<int>(
                value: _mode,
                isExpanded: true,
                decoration: InputDecoration(
                    labelText: t(context, 'helper.params.mode'),
                    isDense: true,
                    border: const OutlineInputBorder()),
                items: [
                  DropdownMenuItem(
                      value: 0,
                      child: Text(t(context, 'helper.params.mode.switch'))),
                  DropdownMenuItem(
                      value: 1,
                      child: Text(t(context, 'helper.params.mode.prop'))),
                ],
                onChanged: (v) => setState(() => _mode = v ?? 0),
              ),
            ),
            SizedBox(
              width: 140,
              child: DropdownButtonFormField<int>(
                value: _levelCount,
                decoration: InputDecoration(
                    labelText: t(context, 'helper.params.levelCount'),
                    isDense: true,
                    border: const OutlineInputBorder()),
                items: [
                  for (var n = 1; n <= 9; n++)
                    DropdownMenuItem(value: n, child: Text('$n')),
                ],
                onChanged: (v) => setState(() {
                  _levelCount = v ?? _levelCount;
                  if (_level > _levelCount) _level = _levelCount;
                }),
              ),
            ),
            SizedBox(
              width: 150,
              child: DropdownButtonFormField<int>(
                value: _level,
                decoration: InputDecoration(
                    labelText: t(context, 'helper.params.level'),
                    isDense: true,
                    border: const OutlineInputBorder()),
                items: [
                  for (var n = 0; n <= _levelCount; n++)
                    DropdownMenuItem(
                        value: n,
                        child: Text(n == 0
                            ? t(context, 'helper.params.level.off')
                            : '$n')),
                ],
                onChanged: (v) => setState(() => _level = v ?? _level),
              ),
            ),
          ]),
          const SizedBox(height: 12),
          Wrap(spacing: 8, runSpacing: 8, children: [
            _num(_startCurrent, t(context, 'helper.params.startCurrent')),
            _num(_startDelay, t(context, 'helper.params.startDelay')),
            _num(_stopDelay, t(context, 'helper.params.stopDelay')),
            _num(_minCadence, t(context, 'helper.params.minCadence')),
            _num(_fullCadence, t(context, 'helper.params.fullCadence')),
            _num(_maxCurrent, t(context, 'helper.params.maxCurrent')),
            _num(_ramp, t(context, 'helper.params.ramp')),
          ]),
        ]),
        _section(context, t(context, 'helper.params.buttons'), [
          for (var i = 0; i < kBtnUiSlots; i++)
            Padding(
              padding: const EdgeInsets.symmetric(vertical: 4),
              child: Row(
                children: [
                  SizedBox(
                    width: 64,
                    child: Text(t(context, 'helper.params.button')
                        .replaceAll('{n}', String.fromCharCode(65 + i))),
                  ),
                  Expanded(
                      child: _num(_btnId[i], t(context, 'helper.params.canId'),
                          width: double.infinity, hex: true)),
                  const SizedBox(width: 8),
                  Expanded(
                      child: _num(
                          _btnData[i], t(context, 'helper.params.canData'),
                          width: double.infinity, hex: true)),
                ],
              ),
            ),
          const SizedBox(height: 6),
          Text(t(context, 'helper.params.buttonsHint'),
              style: TextStyle(fontSize: 11, color: outline)),
        ]),
        Row(
          mainAxisAlignment: MainAxisAlignment.center,
          children: [
            OutlinedButton.icon(
              icon: const Icon(Icons.refresh),
              label: Text(t(context, 'helper.params.read')),
              onPressed: () => widget.h.getParams(),
            ),
            const SizedBox(width: 16),
            FilledButton.icon(
              icon: const Icon(Icons.save_outlined),
              label: Text(t(context, 'helper.params.write')),
              onPressed: _write,
            ),
          ],
        ),
        const SizedBox(height: 12),
      ],
    );
  }

  Widget _section(BuildContext context, String title, List<Widget> children) {
    return Card(
      child: Padding(
        padding: const EdgeInsets.all(12),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Text(title, style: Theme.of(context).textTheme.titleSmall),
            const SizedBox(height: 8),
            ...children,
          ],
        ),
      ),
    );
  }
}

// ---------------------------------------------------------------------------
// Pairing
// ---------------------------------------------------------------------------

class _BindTab extends StatefulWidget {
  const _BindTab({required this.h});
  final HelperProxy h;

  @override
  State<_BindTab> createState() => _BindTabState();
}

class _BindTabState extends State<_BindTab>
    with AutomaticKeepAliveClientMixin {
  int? _selected;
  bool _scanned = false;

  @override
  bool get wantKeepAlive => true;

  @override
  void initState() {
    super.initState();
    widget.h.addListener(_onChange);
  }

  @override
  void dispose() {
    widget.h.removeListener(_onChange);
    super.dispose();
  }

  void _onChange() {
    if (mounted) setState(() {});
  }

  void _scan(int what) {
    setState(() {
      _selected = null;
      _scanned = true;
    });
    widget.h.scanRemotes(what);
  }

  static IconData _rssiIcon(int rssi) => rssi >= -60
      ? Icons.signal_cellular_alt
      : rssi >= -75
          ? Icons.signal_cellular_alt_2_bar
          : Icons.signal_cellular_alt_1_bar;

  @override
  Widget build(BuildContext context) {
    super.build(context);
    final h = widget.h;
    final scheme = Theme.of(context).colorScheme;
    final hits = h.scanHits;

    return Column(
      children: [
        Padding(
          padding: const EdgeInsets.fromLTRB(12, 12, 12, 8),
          child: Wrap(
            spacing: 8,
            runSpacing: 4,
            children: [
              FilledButton.tonalIcon(
                icon: const Icon(Icons.settings_remote),
                label: Text(t(context, 'helper.bind.scanButton')),
                onPressed: h.connected ? () => _scan(kWhatButton) : null,
              ),
              FilledButton.tonalIcon(
                icon: const Icon(Icons.pedal_bike),
                label: Text(t(context, 'helper.bind.scanCadence')),
                onPressed: h.connected ? () => _scan(kWhatCadence) : null,
              ),
            ],
          ),
        ),
        Expanded(
          child: hits.isEmpty
              ? _Empty(
                  icon: Icons.bluetooth_searching,
                  title: t(context,
                      _scanned ? 'helper.bind.scanning' : 'helper.bind.empty'),
                  hint: _scanned ? '' : t(context, 'helper.bind.emptyHint'),
                )
              : ListView.separated(
                  padding: const EdgeInsets.symmetric(horizontal: 12),
                  itemCount: hits.length,
                  separatorBuilder: (_, __) => const SizedBox(height: 6),
                  itemBuilder: (context, i) {
                    final hit = hits[i];
                    final selected = _selected == i;
                    return ListTile(
                      shape: RoundedRectangleBorder(
                          borderRadius: BorderRadius.circular(14)),
                      tileColor: scheme.surfaceContainerLow,
                      selectedTileColor: scheme.secondaryContainer,
                      selected: selected,
                      leading: CircleAvatar(
                        backgroundColor: selected
                            ? scheme.secondary
                            : scheme.surfaceContainerHighest,
                        foregroundColor:
                            selected ? scheme.onSecondary : scheme.outline,
                        child: Icon(
                            hit.what == kWhatButton
                                ? Icons.settings_remote
                                : Icons.pedal_bike,
                            size: 20),
                      ),
                      title: Text(hit.name.isEmpty
                          ? t(context, 'helper.bind.unnamed')
                          : hit.name),
                      subtitle: Text('${t(context, hit.what == kWhatButton ? 'helper.bind.button' : 'helper.bind.cadence')} · ${hit.mac}'),
                      trailing: Column(
                        mainAxisAlignment: MainAxisAlignment.center,
                        children: [
                          Icon(_rssiIcon(hit.rssi),
                              size: 18, color: scheme.outline),
                          Text('${hit.rssi} dBm',
                              style: TextStyle(
                                  fontSize: 10, color: scheme.outline)),
                        ],
                      ),
                      onTap: () => setState(() => _selected = i),
                    );
                  },
                ),
        ),
        SafeArea(
          top: false,
          child: Padding(
            padding: const EdgeInsets.all(12),
            child: Wrap(
              spacing: 8,
              runSpacing: 4,
              alignment: WrapAlignment.center,
              children: [
                FilledButton.icon(
                  icon: const Icon(Icons.link),
                  label: Text(t(context, 'helper.bind.bind')),
                  onPressed: _selected == null
                      ? null
                      : () async {
                          await h.bind(hits[_selected!]);
                          if (context.mounted) {
                            ScaffoldMessenger.of(context).showSnackBar(SnackBar(
                                content:
                                    Text(t(context, 'helper.bind.bound'))));
                          }
                        },
                ),
                OutlinedButton(
                  onPressed:
                      h.connected ? () => h.unbind(kWhatCadence) : null,
                  child: Text(t(context, 'helper.bind.unbindCadence')),
                ),
                OutlinedButton(
                  onPressed: h.connected ? () => h.unbind(kWhatButton) : null,
                  child: Text(t(context, 'helper.bind.unbindButtons')),
                ),
              ],
            ),
          ),
        ),
      ],
    );
  }
}

// ---------------------------------------------------------------------------
// Firmware
// ---------------------------------------------------------------------------

class _FirmwareTab extends StatefulWidget {
  const _FirmwareTab({required this.h});
  final HelperProxy h;

  @override
  State<_FirmwareTab> createState() => _FirmwareTabState();
}

class _FirmwareTabState extends State<_FirmwareTab> {
  final _fw = HelperFirmware();

  BundledHelperFirmware? _bundled;
  HelperRelease? _latest;
  bool _checking = false;
  bool _flashing = false;
  double? _downloadProgress;
  String? _error;

  /// What flashing would actually write: the bundled image unless a newer one
  /// was fetched from GitHub.
  bool get _useDownload =>
      _latest != null &&
      (_bundled == null || _latest!.version != _bundled!.version);

  @override
  void initState() {
    super.initState();
    widget.h.addListener(_onChange);
    // The bundled image is the one that works in a garage with no signal, so
    // load it first and never block on the network.
    unawaited(_fw.loadBundled().then((b) {
      if (mounted) setState(() => _bundled = b);
    }));
  }

  @override
  void dispose() {
    widget.h.removeListener(_onChange);
    super.dispose();
  }

  void _onChange() {
    if (mounted) setState(() {});
  }

  Future<void> _check() async {
    setState(() {
      _checking = true;
      _error = null;
    });
    try {
      final r = await _fw.fetchLatest();
      if (mounted) setState(() => _latest = r);
    } catch (e) {
      if (mounted) setState(() => _error = '$e');
    } finally {
      if (mounted) setState(() => _checking = false);
    }
  }

  /// Flash the bundled image, or the newer one from GitHub if a check found
  /// it. Downloads are not cached: a helper gets flashed once in a blue moon,
  /// and a stale cache is worse than a 600 KB download.
  Future<void> _flash() async {
    final bundled = _bundled;
    final release = _latest;
    if (bundled == null && release == null) return;
    setState(() {
      _flashing = true;
      _error = null;
      _downloadProgress = _useDownload ? 0 : null;
    });
    try {
      final Uint8List image;
      if (_useDownload) {
        image = await _fw.download(release!, onProgress: (f) {
          if (mounted) setState(() => _downloadProgress = f);
        });
      } else {
        image = bundled!.bytes;
      }
      if (!mounted) return;
      setState(() => _downloadProgress = null);
      await widget.h.flashFirmware(image);
      if (mounted) {
        ScaffoldMessenger.of(context).showSnackBar(
            SnackBar(content: Text(t(context, 'helper.fw.done'))));
      }
    } catch (e) {
      if (mounted) setState(() => _error = '$e');
    } finally {
      if (mounted) {
        setState(() {
          _flashing = false;
          _downloadProgress = null;
        });
      }
    }
  }

  @override
  Widget build(BuildContext context) {
    final h = widget.h;
    final scheme = Theme.of(context).colorScheme;
    final installed = h.fwVersion;
    final latest = _latest?.version;
    final bundled = _bundled?.version;
    // "Up to date" means: the helper already runs what we would flash it with.
    final wouldFlash = _useDownload ? latest : bundled;
    final upToDate =
        installed != null && wouldFlash != null && installed == wouldFlash;
    final unknown = t(context, 'helper.fw.unknown');

    return ListView(
      padding: const EdgeInsets.all(16),
      children: [
        Card(
          child: Padding(
            padding: const EdgeInsets.all(16),
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Text(t(context, 'helper.fw.installed')
                    .replaceAll('{v}', installed ?? unknown)),
                const SizedBox(height: 4),
                Text(t(context, 'helper.fw.bundled')
                    .replaceAll('{v}', bundled ?? unknown)),
                if (_bundled != null)
                  Text('${(_bundled!.sizeBytes / 1024).round()} KB',
                      style: TextStyle(fontSize: 11, color: scheme.outline)),
                if (_latest != null && latest != bundled) ...[
                  const SizedBox(height: 4),
                  Text(t(context, 'helper.fw.latest')
                      .replaceAll('{v}', latest ?? unknown)),
                  Text('${(_latest!.sizeBytes / 1024).round()} KB · $kHelperRepo',
                      style: TextStyle(fontSize: 11, color: scheme.outline)),
                ],
                const SizedBox(height: 12),
                if (_checking)
                  Text(t(context, 'helper.fw.checking'),
                      style: TextStyle(fontSize: 12, color: scheme.outline))
                else if (wouldFlash != null)
                  Text(
                    t(context,
                        upToDate ? 'helper.fw.upToDate' : 'helper.fw.available'),
                    style: TextStyle(
                        color: upToDate ? scheme.primary : Colors.orange),
                  ),
                if (_error != null) ...[
                  const SizedBox(height: 8),
                  Text(_error!,
                      style: TextStyle(fontSize: 12, color: scheme.error)),
                ],
              ],
            ),
          ),
        ),
        const SizedBox(height: 8),
        Text(t(context, 'helper.fw.warn'),
            style: TextStyle(fontSize: 12, color: scheme.outline)),
        const SizedBox(height: 16),
        if (_downloadProgress != null) ...[
          LinearProgressIndicator(value: _downloadProgress),
          const SizedBox(height: 8),
          Text(t(context, 'helper.fw.downloading'),
              textAlign: TextAlign.center,
              style: TextStyle(fontSize: 12, color: scheme.outline)),
        ] else if (h.otaProgress != null) ...[
          LinearProgressIndicator(value: h.otaProgress),
          const SizedBox(height: 8),
          Text(t(context, 'helper.fw.flashing'),
              textAlign: TextAlign.center,
              style: TextStyle(fontSize: 12, color: scheme.outline)),
        ] else
          Row(
            children: [
              Expanded(
                child: OutlinedButton.icon(
                  icon: const Icon(Icons.refresh),
                  label: Text(t(context, 'helper.fw.check')),
                  onPressed: _checking || _flashing ? null : _check,
                ),
              ),
              const SizedBox(width: 12),
              Expanded(
                child: FilledButton.icon(
                  icon: const Icon(Icons.system_update),
                  label: Text(t(context, 'helper.fw.flash')),
                  onPressed: !h.connected ||
                          _flashing ||
                          (_bundled == null && _latest == null)
                      ? null
                      : _flash,
                ),
              ),
            ],
          ),
      ],
    );
  }
}

class _Empty extends StatelessWidget {
  const _Empty({required this.icon, required this.title, required this.hint});
  final IconData icon;
  final String title;
  final String hint;

  @override
  Widget build(BuildContext context) {
    final scheme = Theme.of(context).colorScheme;
    return Center(
      child: Padding(
        padding: const EdgeInsets.all(24),
        child: Column(
          mainAxisSize: MainAxisSize.min,
          children: [
            Icon(icon, size: 44, color: scheme.outline),
            const SizedBox(height: 12),
            Text(title, style: Theme.of(context).textTheme.titleMedium),
            if (hint.isNotEmpty) ...[
              const SizedBox(height: 6),
              Text(hint,
                  textAlign: TextAlign.center,
                  style: TextStyle(fontSize: 12, color: scheme.outline)),
            ],
          ],
        ),
      ),
    );
  }
}
