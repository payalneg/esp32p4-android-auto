/// Every map and navigator preference in one window.
///
/// The map screen used to carry a style toggle, a profile menu and a ⋮ menu of
/// switches; the map-data screen carried the offline-map numbers. One screen
/// now owns all of it, so the map itself keeps only search and this gear.
library;

import 'package:flutter/material.dart';

import '../ble/ble_proxy.dart';
import '../i18n/strings.dart';
import '../nav/map_data.dart';
import '../nav/way_classes.dart';
import 'map_data_screen.dart';
import 'region_picker_screen.dart';
import '../settings/nav_settings.dart';

class MapSettingsScreen extends StatelessWidget {
  const MapSettingsScreen({
    super.key,
    this.onSaveArea,
  });

  /// Downloads the tiles around the rider. Only the live navigator can do
  /// that (it owns the position and the progress overlay), so the row is
  /// hidden when this screen is opened elsewhere.
  final VoidCallback? onSaveArea;

  @override
  Widget build(BuildContext context) {
    final s = NavSettings.instance;
    return Scaffold(
      appBar: AppBar(title: Text(t(context, 'nav.settings.title'))),
      body: ListenableBuilder(
        listenable: Listenable.merge(
            <Listenable>[NavSettings.instance, MapData.instance]),
        builder: (context, _) => ListView(
          padding: const EdgeInsets.all(12),
          children: <Widget>[
            _Section(
              title: t(context, 'nav.settings.section.navigation'),
              children: <Widget>[
                SwitchListTile(
                  secondary: const Icon(Icons.record_voice_over_outlined),
                  title: Text(t(context, 'nav.voice')),
                  value: s.voice,
                  onChanged: (v) => s.setVoice(v),
                ),
                SwitchListTile(
                  secondary: const Icon(Icons.vibration),
                  title: Text(t(context, 'nav.haptics')),
                  value: s.haptics,
                  onChanged: (v) => s.setHaptics(v),
                ),
                SwitchListTile(
                  secondary: const Icon(Icons.navigation_outlined),
                  title: Text(t(context, 'nav.trackUp')),
                  value: s.trackUp,
                  onChanged: (v) => s.setTrackUp(v),
                ),
                SwitchListTile(
                  secondary: const Icon(Icons.play_circle_outline),
                  title: Text(t(context, 'nav.simulator')),
                  value: s.simulator,
                  onChanged: (v) => s.setSimulator(v),
                ),
                ListTile(
                  leading: const Icon(Icons.route_outlined),
                  title: Text(t(context, 'mapdata.profile.default')),
                  trailing: DropdownButton<RideProfile>(
                    value: s.profile,
                    items: RideProfile.values
                        .map((p) => DropdownMenuItem<RideProfile>(
                            value: p, child: Text(t(context, p.i18nKey))))
                        .toList(),
                    onChanged: (p) => p == null ? null : s.setProfile(p),
                  ),
                ),
              ],
            ),
            _Section(
              title: t(context, 'nav.settings.section.map'),
              children: <Widget>[
                if (MapData.instance.hasGraphTiles)
                  SwitchListTile(
                    secondary: const Icon(Icons.polyline),
                    title: Text(t(context, 'nav.tiles.enable')),
                    subtitle: Text(t(
                        context,
                        s.useGraphTiles
                            ? 'nav.tiles.style.graph.sub'
                            : 'nav.tiles.style.osm.sub')),
                    value: s.useGraphTiles,
                    onChanged: (v) => s.setUseGraphTiles(v),
                  ),
                ListTile(
                  leading: const Icon(Icons.public),
                  title: Text(t(context, 'nav.settings.region')),
                  subtitle: Text(_regionLabel(context)),
                  trailing: const Icon(Icons.chevron_right),
                  onTap: () => Navigator.push(
                      context,
                      MaterialPageRoute(
                          builder: (_) => const RegionPickerScreen())),
                ),
                SwitchListTile(
                  secondary: const Icon(Icons.travel_explore),
                  title: Text(t(context, 'mapdata.auto.title')),
                  subtitle: Text(t(context, 'mapdata.auto.subtitle')),
                  value: s.autoRegion,
                  onChanged: (v) => s.setAutoRegion(v),
                ),
              ],
            ),
            _Section(
              title: t(context, 'nav.settings.section.offline'),
              children: <Widget>[
                _choice(
                  context,
                  title: t(context, 'mapdata.area.radius'),
                  value: s.areaRadiusKm,
                  items: const <double>[1, 2, 5, 20, 50, 100],
                  label: (km) => '${km.round()} km',
                  onChanged: s.setAreaRadiusKm,
                ),
                _choice(
                  context,
                  title: t(context, 'mapdata.area.budget'),
                  value: s.tileBudget,
                  items: const <int>[1000, 3000, 10000],
                  label: (n) => '$n',
                  onChanged: s.setTileBudget,
                ),
                _choice(
                  context,
                  title: t(context, 'mapdata.tiles.cap'),
                  value: s.tileCapMb,
                  items: const <int>[100, 300, 1000],
                  label: (mb) => '$mb MB',
                  onChanged: s.setTileCapMb,
                ),
                _choice(
                  context,
                  title: t(context, 'mapdata.corridor.max'),
                  value: s.corridorMaxTiles,
                  items: const <int>[100, 250, 600],
                  label: (n) => '$n',
                  onChanged: s.setCorridorMaxTiles,
                ),
              ],
            ),
            _Section(
              title: t(context, 'nav.settings.section.headUnit'),
              children: <Widget>[
                SwitchListTile(
                  secondary: const Icon(Icons.tv),
                  title: Text(t(context, 'nav.stream')),
                  subtitle: Text(t(context, 'nav.settings.stream.sub')),
                  value: s.streamToDisplay,
                  onChanged: (v) {
                    s.setStreamToDisplay(v);
                    BleProxy.instance.setNavStreamEnabled(v);
                  },
                ),
              ],
            ),
            _Section(
              title: t(context, 'nav.settings.section.data'),
              children: <Widget>[
                ListTile(
                  leading: const Icon(Icons.map_outlined),
                  title: Text(t(context, 'mapdata.title')),
                  subtitle: Text(t(context, 'nav.settings.openData.subtitle')),
                  trailing: const Icon(Icons.chevron_right),
                  onTap: () => Navigator.push(
                      context,
                      MaterialPageRoute(
                          builder: (_) => const MapDataScreen())),
                ),
                if (onSaveArea != null)
                  ListTile(
                    leading: const Icon(Icons.download_for_offline_outlined),
                    title: Text(t(context, 'mapdata.area.save')),
                    subtitle: Text(t(context, 'nav.settings.saveArea.sub')),
                    onTap: () {
                      Navigator.of(context).pop();
                      onSaveArea!.call();
                    },
                  ),
              ],
            ),
          ],
        ),
      ),
    );
  }

  String _regionLabel(BuildContext context) {
    final name = MapData.instance.regionName;
    if (name == null) return t(context, 'mapdata.region.subtitle');
    return tf(context, 'mapdata.region.current', <String, Object?>{'name': name});
  }

  Widget _choice<T>(BuildContext context,
      {required String title,
      required T value,
      required List<T> items,
      required String Function(T) label,
      required Future<void> Function(T) onChanged}) {
    return ListTile(
      title: Text(title),
      trailing: DropdownButton<T>(
        value: value,
        items: items
            .map((v) => DropdownMenuItem<T>(value: v, child: Text(label(v))))
            .toList(),
        onChanged: (v) {
          if (v != null) onChanged(v);
        },
      ),
    );
  }
}

class _Section extends StatelessWidget {
  const _Section({required this.title, required this.children});
  final String title;
  final List<Widget> children;

  @override
  Widget build(BuildContext context) {
    return Card(
      clipBehavior: Clip.antiAlias,
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: <Widget>[
          Padding(
            padding: const EdgeInsets.fromLTRB(16, 12, 16, 4),
            child: Text(
              title,
              style: Theme.of(context).textTheme.titleSmall?.copyWith(
                    color: Theme.of(context).colorScheme.primary,
                  ),
            ),
          ),
          ...children,
        ],
      ),
    );
  }
}
