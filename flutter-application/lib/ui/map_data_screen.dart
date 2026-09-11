/// Where the navigator's data comes from and how much space it takes.
///
/// What the navigator is running on, and how much room it takes.
///
/// Areas themselves are downloaded from the map screen — you want to see the
/// ground you are asking for — so this screen shows what arrived, lets files
/// built on a desktop be imported instead, and manages the tile cache.
library;

import 'package:file_picker/file_picker.dart';
import 'package:flutter/material.dart';

import '../i18n/strings.dart';
import '../nav/map_data.dart';
import '../nav/way_classes.dart';
import 'region_picker_screen.dart';
import '../settings/nav_settings.dart';

class MapDataScreen extends StatefulWidget {
  const MapDataScreen({super.key});

  @override
  State<MapDataScreen> createState() => _MapDataScreenState();
}

class _MapDataScreenState extends State<MapDataScreen> {
  ({int bytes, int files})? _tileStats;


  @override
  void initState() {
    super.initState();
    _refreshTiles();
  }



  Future<void> _refreshTiles() async {
    final stats = await MapData.instance.tiles?.stats();
    if (mounted) setState(() => _tileStats = stats);
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(title: Text(t(context, 'mapdata.title'))),
      body: ListenableBuilder(
        listenable: Listenable.merge(
            <Listenable>[MapData.instance, NavSettings.instance]),
        builder: (context, _) => ListView(
          padding: const EdgeInsets.all(16),
          children: <Widget>[
            _statusCard(context),
            _regionCard(context),
            _importCard(context),
            _tilesCard(context),
            _profileCard(context),
            if (MapData.instance.state != MapDataState.absent) _deleteCard(context),
          ],
        ),
      ),
    );
  }

  Widget _statusCard(BuildContext context) {
    final data = MapData.instance;
    final graph = data.graph;
    final theme = Theme.of(context);
    final children = <Widget>[];

    switch (data.state) {
      case MapDataState.ready:
        children.add(Text(
          tf(context, 'mapdata.status.ready', <String, Object?>{
            'nodes': graph?.nodeCount ?? 0,
            'edges': graph?.edgeCount ?? 0,
            'mb': ((data.graphFile?.existsSync() ?? false)
                    ? data.graphFile!.lengthSync() / (1 << 20)
                    : 0)
                .toStringAsFixed(1),
          }),
          style: theme.textTheme.titleMedium,
        ));
        children.add(const SizedBox(height: 4));
        children.add(Text(
          data.index == null
              ? t(context, 'mapdata.status.noplaces')
              : tf(context, 'mapdata.status.places',
                  <String, Object?>{'places': data.index!.length}),
          style: theme.textTheme.bodySmall,
        ));
      case MapDataState.downloading:
        children.add(Text(tf(context, 'mapdata.downloading',
            <String, Object?>{'file': data.progressFile ?? ''})));
        children.add(const SizedBox(height: 8));
        children.add(const LinearProgressIndicator());
      case MapDataState.loading:
        children.add(Text(t(context, 'mapdata.status.loading')));
        children.add(const SizedBox(height: 8));
        children.add(const LinearProgressIndicator());
      case MapDataState.error:
        children.add(Text(
          data.messageKey == null
              ? t(context, 'settings.mapdata.subtitle.error')
              : tf(context, data.messageKey!, data.messageArgs ?? const {}),
          style: TextStyle(color: theme.colorScheme.error),
        ));
      case MapDataState.absent:
        children.add(Text(t(context, 'mapdata.status.none')));
    }

    return Card(
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
            crossAxisAlignment: CrossAxisAlignment.start, children: children),
      ),
    );
  }

  /// Where a routing graph comes from.
  Widget _regionCard(BuildContext context) {
    return Card(
      child: ListTile(
        leading: const Icon(Icons.public),
        title: Text(t(context, 'mapdata.region.title')),
        subtitle: Text(t(context, 'mapdata.region.subtitle')),
        trailing: const Icon(Icons.chevron_right),
        onTap: () => Navigator.push(context,
            MaterialPageRoute(builder: (_) => const RegionPickerScreen())),
      ),
    );
  }

  Widget _importCard(BuildContext context) {
    return Card(
      child: Column(
        children: <Widget>[
          ListTile(
            leading: const Icon(Icons.folder_open),
            title: Text(t(context, 'mapdata.import.title')),
            dense: true,
          ),
          ListTile(
            leading: const Icon(Icons.file_open_outlined),
            title: Text(t(context, 'mapdata.import.graph')),
            onTap: () => _pick(MapData.instance.importGraph),
          ),
          ListTile(
            leading: const Icon(Icons.manage_search),
            title: Text(t(context, 'mapdata.import.index')),
            onTap: () => _pick(MapData.instance.importIndex),
          ),
        ],
      ),
    );
  }

  /// Picks a file by path, never by bytes: the graph is 17 MB and `withData`
  /// would haul all of it through the platform channel.
  Future<void> _pick(Future<void> Function(String path) then) async {
    final picked = await FilePicker.platform.pickFiles();
    final path = picked?.files.single.path;
    if (path != null) await then(path);
  }

  Widget _tilesCard(BuildContext context) {
    final stats = _tileStats;
    final settings = NavSettings.instance;
    return Card(
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: <Widget>[
          ListTile(
            leading: const Icon(Icons.grid_view),
            title: Text(t(context, 'mapdata.tiles.title')),
            subtitle: Text(stats == null || stats.files == 0
                ? t(context, 'mapdata.tiles.empty')
                : tf(context, 'mapdata.tiles.size', <String, Object?>{
                    'mb': (stats.bytes / (1 << 20)).toStringAsFixed(1),
                    'n': stats.files,
                  })),
          ),
          ListTile(
            title: Text(t(context, 'mapdata.area.radius')),
            trailing: DropdownButton<double>(
              value: settings.areaRadiusKm,
              items: const <double>[1, 2, 5, 20, 50, 100]
                  .map((km) => DropdownMenuItem<double>(
                      value: km, child: Text('${km.round()} km')))
                  .toList(),
              onChanged: (v) => v == null ? null : settings.setAreaRadiusKm(v),
            ),
          ),
          ListTile(
            title: Text(t(context, 'mapdata.area.budget')),
            trailing: DropdownButton<int>(
              value: settings.tileBudget,
              items: const <int>[1000, 3000, 10000]
                  .map((n) =>
                      DropdownMenuItem<int>(value: n, child: Text('$n')))
                  .toList(),
              onChanged: (v) => v == null ? null : settings.setTileBudget(v),
            ),
          ),
          ListTile(
            title: Text(t(context, 'mapdata.tiles.cap')),
            trailing: DropdownButton<int>(
              value: settings.tileCapMb,
              items: const <int>[100, 300, 1000]
                  .map((mb) =>
                      DropdownMenuItem<int>(value: mb, child: Text('$mb MB')))
                  .toList(),
              onChanged: (v) => v == null ? null : settings.setTileCapMb(v),
            ),
          ),
          ListTile(
            title: Text(t(context, 'mapdata.corridor.max')),
            trailing: DropdownButton<int>(
              value: settings.corridorMaxTiles,
              items: const <int>[100, 250, 600]
                  .map((n) =>
                      DropdownMenuItem<int>(value: n, child: Text('$n')))
                  .toList(),
              onChanged: (v) =>
                  v == null ? null : settings.setCorridorMaxTiles(v),
            ),
          ),
          Align(
            alignment: Alignment.centerRight,
            child: Padding(
              padding: const EdgeInsets.only(right: 8, bottom: 8),
              child: TextButton.icon(
                icon: const Icon(Icons.delete_outline, size: 18),
                label: Text(t(context, 'mapdata.tiles.clear')),
                onPressed: (stats?.files ?? 0) == 0
                    ? null
                    : () async {
                        if (await _confirm(context, 'mapdata.tiles.clear.confirm')) {
                          await MapData.instance.tiles?.clear();
                          await _refreshTiles();
                        }
                      },
              ),
            ),
          ),
        ],
      ),
    );
  }

  Widget _profileCard(BuildContext context) {
    return Card(
      child: ListTile(
        leading: const Icon(Icons.route_outlined),
        title: Text(t(context, 'mapdata.profile.default')),
        trailing: DropdownButton<RideProfile>(
          value: NavSettings.instance.profile,
          items: RideProfile.values
              .map((p) => DropdownMenuItem<RideProfile>(
                  value: p, child: Text(t(context, p.i18nKey))))
              .toList(),
          onChanged: (p) =>
              p == null ? null : NavSettings.instance.setProfile(p),
        ),
      ),
    );
  }

  Widget _deleteCard(BuildContext context) {
    return Card(
      child: ListTile(
        leading: Icon(Icons.delete_forever,
            color: Theme.of(context).colorScheme.error),
        title: Text(t(context, 'mapdata.delete')),
        onTap: () async {
          if (await _confirm(context, 'mapdata.delete.confirm')) {
            await MapData.instance.deleteAll();
          }
        },
      ),
    );
  }

  Future<bool> _confirm(BuildContext context, String bodyKey) async {
    final ok = await showDialog<bool>(
      context: context,
      builder: (ctx) => AlertDialog(
        content: Text(t(ctx, bodyKey)),
        actions: <Widget>[
          TextButton(
              onPressed: () => Navigator.pop(ctx, false),
              child: Text(t(ctx, 'home.forget.cancel'))),
          FilledButton(
              onPressed: () => Navigator.pop(ctx, true),
              child: Text(t(ctx, 'home.forget.ok'))),
        ],
      ),
    );
    return ok ?? false;
  }
}
