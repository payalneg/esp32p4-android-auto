/// Picking a regional extract to build the routing graph from.
///
/// Geofabrik publishes the OSM planet cut into countries and provinces, and a
/// whole province is one file of a couple of hundred megabytes — the same
/// input scripts/mapgen reads, and the only practical way to have a routing
/// graph that covers more than a neighbourhood.
library;

import 'package:flutter/material.dart';

import '../i18n/strings.dart';
import '../nav/geofabrik.dart';
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

  Future<void> _load() async {
    setState(() {
      _loading = true;
      _errorKey = null;
    });
    try {
      final index =
          await GeofabrikIndex.fetch(userAgent: MapData.instance.userAgent);
      if (mounted) setState(() => _index = index);
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
                      onPressed: _load,
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
      return Center(
        child: Padding(
          padding: const EdgeInsets.all(32),
          child: Text(
            tf(context, 'mapdata.region.count',
                <String, Object?>{'n': index.regions.length}),
            textAlign: TextAlign.center,
            style: Theme.of(context).textTheme.bodyMedium,
          ),
        ),
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
          onTap: () => _confirm(region),
        );
      },
    );
  }

  /// A province is a long download and a heavy parse, so it is worth one
  /// deliberate tap rather than an accidental one.
  Future<void> _confirm(GeofabrikRegion region) async {
    final ok = await showDialog<bool>(
      context: context,
      builder: (ctx) => AlertDialog(
        title: Text(region.name),
        content: Text(t(ctx, 'mapdata.region.confirm')),
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
