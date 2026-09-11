/// Turning raw OSM tags into the road classes the router understands.
///
/// A faithful port of `_collect_bike_way` in scripts/mapgen/builder.py — the
/// generator on the desktop and the builder on the phone have to agree, or the
/// same city would route differently depending on where its graph was made.
library;

import 'way_classes.dart';

/// highway=* → class name. Anything absent never enters the graph.
const Map<String, String> kHighwayClass = <String, String>{
  'cycleway': 'cycleway',
  'tertiary': 'tertiary',
  'tertiary_link': 'tertiary',
  'residential': 'residential',
  'unclassified': 'residential',
  'living_street': 'residential',
  'secondary': 'secondary',
  'secondary_link': 'secondary',
  'primary': 'primary',
  'primary_link': 'primary',
  'service': 'service',
  'path': 'path',
  'track': 'track',
  'pedestrian': 'pedestrian',
  'footway': 'footway',
  'steps': 'steps',
};

/// Which way an edge may be ridden: both, as drawn, or reversed.
enum WayDirection { both, forward, backward }

/// The road-class index for a way, or null when it is not part of the network.
///
/// Note what is deliberately *not* filtered here: `bicycle=no` and
/// `use_sidepath` keep their way in the graph. The class already says what
/// kind of road it is, and whether it may be ridden is the profile's call —
/// a scooter on the pavement wants different answers from a bicycle.
int? classifyWay(Map<String, String> tags) {
  final highway = tags['highway'];
  if (highway == null) return null;
  var cls = kHighwayClass[highway];
  if (cls == null) return null;

  final bicycle = tags['bicycle'];
  final access = tags['access'];
  if ((access == 'no' || access == 'private') && bicycle != 'yes') return null;

  // A pavement and a crossing are their own classes: on a scooter they are
  // real options, not penalised footpaths.
  if (highway == 'footway') {
    final footway = tags['footway'];
    if (footway == 'sidewalk') {
      cls = 'sidewalk';
    } else if (footway == 'crossing') {
      cls = 'crossing';
    }
  } else if (highway == 'path' && tags['path'] == 'crossing') {
    cls = 'crossing';
  }

  // A footpath you are explicitly allowed to ride is functionally a cycleway.
  if ((cls == 'footway' ||
          cls == 'path' ||
          cls == 'pedestrian' ||
          cls == 'sidewalk') &&
      (bicycle == 'yes' || bicycle == 'designated')) {
    cls = 'cycleway';
  }
  return kWayClasses.indexOf(cls);
}

/// One-way rules as they apply to a bicycle or a scooter, which is not the
/// same as for a car: a contraflow cycle lane makes a one-way street two-way
/// again.
WayDirection wayDirection(Map<String, String> tags) {
  final oneway = tags['oneway'];
  final onewayBicycle = tags['oneway:bicycle'];
  final contraflow = tags.entries.any((e) =>
      e.key.startsWith('cycleway') && e.value.startsWith('opposite'));

  if (onewayBicycle == 'no' || contraflow) return WayDirection.both;
  if (oneway == 'yes' ||
      oneway == '1' ||
      oneway == 'true' ||
      onewayBicycle == 'yes') {
    return WayDirection.forward;
  }
  if (oneway == '-1') return WayDirection.backward;
  return WayDirection.both;
}

/// A searchable place pulled from an object's tags: a street address, or a
/// named POI. Mirrors `_extract_search` in builder.py.
({String display, String kind})? searchEntry(Map<String, String> tags) {
  final street = tags['addr:street'];
  final housenumber = tags['addr:housenumber'];
  if (street != null && housenumber != null) {
    return (display: '$street $housenumber', kind: 'address');
  }
  final name = tags['name'];
  final kind = tags['shop'] ?? tags['amenity'] ?? tags['tourism'];
  if (name != null && kind != null) return (display: name, kind: kind);
  return null;
}
