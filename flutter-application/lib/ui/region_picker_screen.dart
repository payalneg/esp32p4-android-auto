/// Picking a regional extract to build the routing graph from.
///
/// Geofabrik publishes the OSM planet cut into countries and provinces, and a
/// whole province is one file of a couple of hundred megabytes — the same
/// input scripts/mapgen reads, and the only practical way to have a routing
/// graph that covers more than a neighbourhood.
library;

import 'dart:async';

import 'package:flutter/material.dart';

import '../i18n/strings.dart';
import '../nav/geofabrik.dart';
import '../nav/location_service.dart';
import '../nav/map_data.dart';

class RegionPickerScreen extends StatefulWidget {
  const RegionPickerScreen({super.key});

  @override
  State<RegionPickerScreen> createState() => _RegionPickerScreenState();
}

class _RegionPickerScreenState extends State<RegionPickerScreen> {
  final _query = TextEditingController();
  GeofabrikIndex? _index;
  bool _loading = true;
  String? _errorKey;
  Map<String, String>? _errorArgs;
  List<GeofabrikRegion> _hits = const <GeofabrikRegion>[];

  /// Catalogue ids whose map is already kept on disk.
  Set<String> _savedIds = const <String>{};
  GeofabrikRegion? _near;
  String? _nearPath;

  @override
  void initState() {
    super.initState();
    _load();
  }

  @override
  void dispose() {
    _query.dispose();
    super.dispose();
  }

  Future<void> _load({bool refresh = false}) async {
    setState(() {
      _loading = true;
      _errorKey = null;
    });
    try {
      final index = await MapData.instance.regionCatalogue(refresh: refresh);
      if (mounted) setState(() => _index = index);
      unawaited(_loadSavedIds());
      unawaited(_findNear());
    } on GeofabrikException catch (e) {
      if (mounted) {
        setState(() {
          _errorKey = e.messageKey;
          _errorArgs = e.args;
        });
      }
    } finally {
      if (mounted) setState(() => _loading = false);
    }
  }

  /// Which rows already have a map on the phone.
  Future<void> _loadSavedIds() async {
    final archived = await MapData.instance.archivedRegions();
    if (!mounted) return;
    setState(() => _savedIds = <String>{
          for (final item in archived)
            if (item.id != null && item.id!.isNotEmpty) item.id!,
        });
  }

  /// The region containing where the rider is, without asking for anything
  /// and without downloading the geometry catalogue if it is not here yet.
  Future<void> _findNear() async {
    try {
      final GeoFix? fix = await LocationService().lastKnown();
      if (fix == null) return;
      final catalogue = await MapData.instance.cachedGeometryCatalogue();
      if (catalogue == null) return;
      final region = catalogue.regionAt(fix.position);
      if (region == null || !mounted) return;
      setState(() {
        _near = region;
        _nearPath = region.pathLabel(catalogue.byId);
      });
    } on Object {
      // A hint is optional; a platform without geolocation simply has none.
    }
  }

  void _search(String q) {
    final index = _index;
    setState(() => _hits =
        index == null ? const <GeofabrikRegion>[] : index.search(q));
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(title: Text(t(context, 'mapdata.region.title'))),
      body: Column(
        children: <Widget>[
          Padding(
            padding: const EdgeInsets.all(16),
            child: TextField(
              controller: _query,
              autofocus: true,
              decoration: InputDecoration(
                labelText: t(context, 'mapdata.region.search'),
                hintText: t(context, 'mapdata.region.hint'),
                prefixIcon: const Icon(Icons.search),
              ),
              onChanged: _search,
            ),
          ),
          if (_loading) const LinearProgressIndicator(),
          if (_errorKey != null)
            Padding(
              padding: const EdgeInsets.symmetric(horizontal: 16),
              child: Row(
                children: <Widget>[
                  Icon(Icons.cloud_off,
                      color: Theme.of(context).colorScheme.error),
                  const SizedBox(width: 12),
                  Expanded(
                    child: Text(tf(context, _errorKey!, _errorArgs ?? const {}),
                        style: TextStyle(
                            color: Theme.of(context).colorScheme.error)),
                  ),
                  TextButton(
                      onPressed: () => _load(refresh: true),
                      child: Text(t(context, 'mapdata.catalog.refresh'))),
                ],
              ),
            ),
          Expanded(child: _results(context)),
        ],
      ),
    );
  }

  Widget _results(BuildContext context) {
    final index = _index;
    if (index == null) return const SizedBox.shrink();
    if (_query.text.trim().isEmpty) {
      return ListView(
        children: <Widget>[
          if (_near != null)
            Card(
              margin: const EdgeInsets.fromLTRB(12, 4, 12, 4),
              child: ListTile(
                leading: const Icon(Icons.my_location),
                title: Text(tf(context, 'mapdata.region.near',
                    <String, Object?>{'region': _near!.name})),
                subtitle: Text(_nearPath ?? ''),
                trailing: _savedIds.contains(_near!.id)
                    ? const Icon(Icons.offline_pin)
                    : const Icon(Icons.chevron_right),
                onTap: () => _confirm(_near!),
              ),
            ),
          Padding(
            padding: const EdgeInsets.all(32),
            child: Text(
              tf(context, 'mapdata.region.count',
                  <String, Object?>{'n': index.regions.length}),
              textAlign: TextAlign.center,
              style: Theme.of(context).textTheme.bodyMedium,
            ),
          ),
        ],
      );
    }
    if (_hits.isEmpty) {
      return Center(child: Text(t(context, 'nav.search.empty')));
    }
    return ListView.builder(
      itemCount: _hits.length,
      itemBuilder: (ctx, i) {
        final region = _hits[i];
        return ListTile(
          leading: const Icon(Icons.public),
          title: Text(region.name),
          subtitle: Text(region.pathLabel(index.byId)),
          trailing: _savedIds.contains(region.id)
              ? const Icon(Icons.offline_pin)
              : null,
          onTap: () => _confirm(region),
        );
      },
    );
  }

  /// A province is a long download and a heavy parse, so it is worth one
  /// deliberate tap rather than an accidental one. A region that is already
  /// saved needs no download at all: the tap becomes a switch.
  Future<void> _confirm(GeofabrikRegion region) async {
    final saved = _savedIds.contains(region.id) ||
        await MapData.instance.hasArchivedRegion(region.id);
    if (!mounted) return;

    if (saved) {
      final ok = await showDialog<bool>(
        context: context,
        builder: (ctx) => AlertDialog(
          title: Text(region.name),
          content: Text(t(ctx, 'mapdata.region.saved.confirm')),
          actions: <Widget>[
            TextButton(
                onPressed: () => Navigator.pop(ctx, false),
                child: Text(t(ctx, 'home.forget.cancel'))),
            FilledButton(
                onPressed: () => Navigator.pop(ctx, true),
                child: Text(t(ctx, 'mapdata.region.switch'))),
          ],
        ),
      );
      if (ok != true || !mounted) return;
      Navigator.pop(context);
      await MapData.instance.activateRegion(MapData.regionKey(region.id));
      return;
    }

    final current = MapData.instance.regionName;
    final warn = current == null
        ? ''
        : '\n\n'
            '${tf(context, 'mapdata.region.confirm.replaces', <String, Object?>{'current': current})}';
    final ok = await showDialog<bool>(
      context: context,
      builder: (ctx) => AlertDialog(
        title: Text(region.name),
        content: Text('${t(ctx, 'mapdata.region.confirm')}$warn'),
        actions: <Widget>[
          TextButton(
              onPressed: () => Navigator.pop(ctx, false),
              child: Text(t(ctx, 'home.forget.cancel'))),
          FilledButton(
              onPressed: () => Navigator.pop(ctx, true),
              child: Text(t(ctx, 'mapdata.download'))),
        ],
      ),
    );
    if (ok != true || !mounted) return;
    Navigator.pop(context);
    await MapData.instance.buildFromRegion(region);
  }
}
