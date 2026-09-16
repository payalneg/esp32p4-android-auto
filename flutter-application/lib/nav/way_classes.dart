/// Road classes and ride profiles — the contract with the routing graph.
///
/// The index of a name in [kWayClasses] IS the `way_class` byte stored per
/// edge in an RGF2 file (see scripts/mapgen/graph_format.py). Never reorder
/// or remove: append only. test/nav/way_classes_test.dart parses
/// scripts/mapgen/profiles.py and fails if the two sides drift.
///
/// Speed is deliberately NOT baked into the graph. The graph says what kind of
/// way an edge is; the profile says what that costs. Switching profile is
/// therefore free — no 17 MB rebuild, and forbidden classes are simply never
/// expanded by A*.
library;

import 'dart:typed_data';

/// Index == the way_class byte in RGF2. Order mirrors profiles.WAY_CLASSES.
const List<String> kWayClasses = <String>[
  'cycleway', // 0  dedicated bike path (and footway/path with bicycle=designated)
  'tertiary', // 1
  'residential', // 2  also unclassified and living_street
  'secondary', // 3
  'primary', // 4
  'service', // 5  driveways, yards, parking aisles
  'path', // 6
  'track', // 7  unpaved
  'pedestrian', // 8  pedestrian streets
  'footway', // 9  footpaths in parks and squares
  'sidewalk', // 10 pavement alongside a road (footway=sidewalk)
  'crossing', // 11 pedestrian crossing (footway=crossing)
  'steps', // 12
  'link', // 13 synthetic sidewalk<->road connector, see builder.py
];

const int kWayClassCount = 14;

/// Classes you ride on the pavement rather than the carriageway. Used for the
/// "sidewalks NN%" figure and for connectivity diagnostics.
const List<String> kFootClasses = <String>[
  'pedestrian',
  'footway',
  'sidewalk',
  'crossing',
  'steps',
];

/// Line groups — the carriageway, the bike path and the pavement beside it are
/// parallel lines in OSM, tied together every 30-50 m by a driveway or a
/// crossing. By distance alone hopping between them is almost free, which is
/// what made a route saw between road and pavement for the sake of a few
/// metres. A* pays [kGroupSwitchPenaltyS] to leave one group for another, so
/// there and back costs 20 s and a hop has to save more than that to be taken.
///
/// [kGroupConnector] is not a group of its own: a crossing (or a synthetic
/// link) carries the previous group through, so road -> crossing -> pavement
/// pays the switch once, and pavement -> crossing -> pavement pays none.
const int kGroupRoad = 0;
const int kGroupCycle = 1;
const int kGroupFoot = 2;
const int kGroupOther = 3;
const int kGroupConnector = 4;

/// Nothing ridden yet — the state a search starts in, where no group has been
/// chosen and the first edge is free whatever it is.
const int kGroupNone = 255;

/// Group per way class, indexed like [kWayClasses].
final Uint8List kWayClassGroup = Uint8List.fromList(<int>[
  kGroupCycle, // 0  cycleway
  kGroupRoad, // 1  tertiary
  kGroupRoad, // 2  residential
  kGroupRoad, // 3  secondary
  kGroupRoad, // 4  primary
  kGroupRoad, // 5  service
  kGroupOther, // 6  path
  kGroupOther, // 7  track
  kGroupFoot, // 8  pedestrian
  kGroupFoot, // 9  footway
  kGroupFoot, // 10 sidewalk
  kGroupConnector, // 11 crossing
  kGroupFoot, // 12 steps
  kGroupConnector, // 13 link
]);

/// Seconds charged for leaving one line group for another. Measured over 200
/// routes across Krakow: at 0 s a route changes line 2.4 times per km, 0.7 of
/// those excursions shorter than 80 m; at 10 s it is 1.0 and 0.14, and the
/// route takes 2.5 % longer.
const double kGroupSwitchPenaltyS = 10.0;

/// Seconds charged for stepping onto a crossing — a kerb and a look both ways,
/// which the 6 m of geometry (2.7 s of riding) does not pay for by itself.
const double kCrossingPenaltyS = 10.0;

/// Ride profiles. [pythonName] is the key in profiles.PROFILES — the parity
/// test matches on it, and it is what a saved preference stores.
enum RideProfile {
  bicycle('Велосипед', 'nav.profile.bicycle'),
  scooter('Самокат', 'nav.profile.scooter'),
  sidewalksOnly('Только тротуары', 'nav.profile.sidewalks'),
  calm('Спокойный', 'nav.profile.calm');

  const RideProfile(this.pythonName, this.i18nKey);

  final String pythonName;
  final String i18nKey;

  static RideProfile? byName(String name) {
    for (final p in RideProfile.values) {
      if (p.name == name) return p;
    }
    return null;
  }
}

const RideProfile kDefaultProfile = RideProfile.scooter;

/// km/h per way class; `null` means the class is forbidden for that profile.
/// Copied verbatim from profiles.PROFILES — index order is [kWayClasses].
const Map<RideProfile, List<double?>> kProfileSpeedsKmh =
    <RideProfile, List<double?>>{
  RideProfile.bicycle: <double?>[
    18, 16, 15, 15, 13, 13, 12, 12, 6, 5, 5, 5, 2, 4,
  ],
  // Roads like a bicycle, but the pavement is a real option rather than a
  // penalty — which is how a scooter is actually ridden in town.
  RideProfile.scooter: <double?>[
    20, 18, 18, 16, 14, 14, 12, 10, 12, 12, 15, 8, null, 5,
  ],
  // Pavements and paths only: the carriageway is off limits.
  RideProfile.sidewalksOnly: <double?>[
    18, null, null, null, null, 10, 12, 10, 12, 12, 15, 8, null, 5,
  ],
  // Leisurely: keep away from big roads, steps allowed.
  RideProfile.calm: <double?>[
    18, 12, 15, 8, 5, 13, 14, 12, 12, 10, 12, 8, 2, 4,
  ],
};

/// Speeds in m/s indexed by way class. Forbidden classes come back as NaN so
/// the hot loop in A* can branch on `isNaN` without boxing a nullable double.
Float64List speedsMs(RideProfile profile) {
  final kmh = kProfileSpeedsKmh[profile]!;
  final out = Float64List(kWayClassCount);
  for (var i = 0; i < kWayClassCount; i++) {
    final v = kmh[i];
    out[i] = v == null ? double.nan : v / 3.6;
  }
  return out;
}

/// Fastest allowed class, m/s — the divisor of the A* heuristic. Using the
/// profile maximum keeps the heuristic admissible, so the route is optimal
/// rather than merely plausible.
double maxSpeedMs(RideProfile profile) {
  var best = 0.0;
  for (final v in kProfileSpeedsKmh[profile]!) {
    if (v != null && v / 3.6 > best) best = v / 3.6;
  }
  return best;
}

/// Bit i set when class i is one of [kFootClasses].
final int kFootClassMask = () {
  var mask = 0;
  for (final name in kFootClasses) {
    mask |= 1 << kWayClasses.indexOf(name);
  }
  return mask;
}();
