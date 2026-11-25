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
import 'region_picker_screen.dart';

class MapDataScreen extends StatefulWidget {
  const MapDataScreen({super.key});

  @override
  State<MapDataScreen> createState() => _MapDataScreenState();
}

class _MapDataScreenState extends State<MapDataScreen> {
  ({int bytes, int files})? _tileStats;
  List<ArchivedRegion>? _archived;
  bool _switching = false;
  MapDataState? _lastState;

  @override
  void initState() {
    super.initState();
    _refreshTiles();
    _refreshArchived();
  }

  Future<void> _refreshArchived() async {
    final list = await MapData.instance.archivedRegions();
    if (mounted) setState(() => _archived = list);
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
        listenable: MapData.instance,
        builder: (context, _) {
          // A finished build adds a saved map; the list here was read once.
          final state = MapData.instance.state;
          if (state == MapDataState.ready && _lastState != MapDataState.ready) {
            WidgetsBinding.instance
                .addPostFrameCallback((_) => _refreshArchived());
          }
          _lastState = state;
          return ListView(
            padding: const EdgeInsets.all(16),
            children: <Widget>[
              _statusCard(context),
              _regionCard(context),
              _savedMapsCard(context),
              _importCard(context),
              _tilesCard(context),
              if (state != MapDataState.absent) _deleteCard(context),
            ],
          );
        },
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
        if (data.regionName != null) {
          children.add(const SizedBox(height: 4));
          children.add(Text(
            tf(context, 'mapdata.region.current',
                <String, Object?>{'name': data.regionName!}),
            style: theme.textTheme.bodySmall,
          ));
        }
      case MapDataState.downloading:
        children.add(Text(tf(context, 'mapdata.downloading',
            <String, Object?>{'file': data.progressFile ?? ''})));
        children.add(const SizedBox(height: 8));
        children.add(const LinearProgressIndicator());
      case MapDataState.loading:
        // A province is minutes of parsing. Naming the stage and counting
        // what has gone by is the difference between waiting and wondering.
        final stage = data.stageKey;
        children.add(Text(stage == null
            ? t(context, 'mapdata.status.loading')
            : tf(context, stage, <String, Object?>{'n': data.stageCount})));
        children.add(const SizedBox(height: 8));
        children.add(LinearProgressIndicator(
            value: stage == null ? null : data.stageFraction));
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

  /// Maps kept on the phone. One is in use; the rest are one tap away, which
  /// is what makes crossing a border cheap instead of a re-download.
  Widget _savedMapsCard(BuildContext context) {
    final list = _archived;
    if (list == null || list.isEmpty) return const SizedBox.shrink();
    final active = MapData.instance.activeKey;
    final dataState = MapData.instance.state;
    final busy = _switching ||
        dataState == MapDataState.downloading ||
        dataState == MapDataState.loading;
    final theme = Theme.of(context);
    return Card(
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: <Widget>[
          ListTile(
            leading: const Icon(Icons.inventory_2_outlined),
            title: Text(t(context, 'mapdata.saved.title')),
            subtitle: Text(tf(
                context, 'mapdata.saved.subtitle',
                <String, Object?>{'n': list.length})),
          ),
          for (final item in list)
            ListTile(
              enabled: !busy,
              leading: Icon(
                item.key == active
                    ? Icons.check_circle
                    : Icons.offline_pin_outlined,
                color: item.key == active ? theme.colorScheme.primary : null,
              ),
              title: Text(_savedName(context, item)),
              subtitle: Text(
                '${(item.bytes / (1 << 20)).toStringAsFixed(1)} MB'
                '${item.key == active ? ' · ${t(context, 'mapdata.saved.active')}' : ''}',
              ),
              onTap: item.key == active || busy ? null : () => _switchTo(item),
              trailing: item.key == active || busy
                  ? null
                  : IconButton(
                      tooltip: t(context, 'mapdata.saved.delete'),
                      icon: const Icon(Icons.delete_outline, size: 20),
                      onPressed: () => _deleteArchived(item),
                    ),
            ),
          if (_switching)
            const Padding(
              padding: EdgeInsets.fromLTRB(16, 0, 16, 12),
              child: LinearProgressIndicator(),
            ),
        ],
      ),
    );
  }

  String _savedName(BuildContext context, ArchivedRegion item) {
    if (item.isImported) return t(context, 'mapdata.saved.imported');
    return item.name.isEmpty ? item.id ?? '' : item.name;
  }

  Future<void> _switchTo(ArchivedRegion item) async {
    if (_switching) return;
    final messenger = ScaffoldMessenger.of(context);
    final failure = t(context, 'mapdata.saved.switchFailed');
    setState(() => _switching = true);
    final ok = await MapData.instance.activateRegion(item.key);
    if (!mounted) return;
    setState(() => _switching = false);
    await _refreshArchived();
    if (!ok) {
      messenger.showSnackBar(SnackBar(content: Text(failure)));
    }
  }

  Future<void> _deleteArchived(ArchivedRegion item) async {
    final ok = await _confirmBody(
        context,
        tf(context, 'mapdata.saved.delete.confirm',
            <String, Object?>{'name': _savedName(context, item)}));
    if (!ok) return;
    await MapData.instance.deleteArchived(item.key);
    await _refreshArchived();
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

  Widget _deleteCard(BuildContext context) {
    return Card(
      child: ListTile(
        leading: Icon(Icons.delete_forever,
            color: Theme.of(context).colorScheme.error),
        title: Text(t(context, 'mapdata.delete')),
        onTap: () async {
          if (await _confirm(context, 'mapdata.delete.confirm')) {
            await MapData.instance.deleteAll();
            await _refreshArchived();
          }
        },
      ),
    );
  }

  Future<bool> _confirm(BuildContext context, String bodyKey) =>
      _confirmBody(context, t(context, bodyKey));

  Future<bool> _confirmBody(BuildContext context, String body) async {
    final ok = await showDialog<bool>(
      context: context,
      builder: (ctx) => AlertDialog(
        content: Text(body),
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
