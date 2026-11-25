# Offline maps that follow the rider

The navigator has two pieces of downloaded data: one or more **routing
graphs** (Geofabrik regional extracts, built into RGF2 on the phone) and a
**raster tile cache** for the ground under the map. Tiles fill in on their own
while the phone has signal. The routing graph does not — it is hundreds of
megabytes and has to be chosen. That choice is now made for you.

## What happens

1. Every position fix is tested against every map on the phone
   (`MapData.covers`): the active graph's envelope plus a widened snap radius
   (3 km), and the envelope of every saved map. The envelope alone would claim
   the next province across a border; the snap radius alone would claim a lake
   in the middle of a province.
2. A fix outside it starts a region lookup, at most once per 5 minutes and
   5 km. The phone fetches Geofabrik's catalogue **with outlines**
   (`index-v1.json`, 3.5 MB) the first time; it is cached in the app's support
   directory and refreshed weekly. A stale copy is used when the network is
   gone.
3. `GeofabrikIndex.regionAt` finds the smallest outline containing the point:
   deepest region wins (province over country), and at equal depth the tighter
   envelope wins.
4. A card appears above the map: *"Offline map for Lesser Poland — you are in
   Poland / Lesser Poland"*, with **Download** and **Not now**.
5. **Download** builds the graph from that extract, then saves the tile
   pyramid around the rider at the current *Area radius* — the same area
   download the menu offers. The result is a map that routes and draws with no
   signal.
6. **Not now** snoozes that region for three days (persisted). A different
   region is offered normally.
7. A route that cannot be finished because the **destination** is in a region
   that is not downloaded is offered the same way, labelled for the
   destination. Downloading it keeps the current map, and the route is retried
   once the build finishes.

The lookup and the offer are skipped when: offers are off (Map data → *Offer
maps for where I am*), a build is already running, the position is covered, the
region is the one already downloaded, the region was snoozed, or the region has
no roads.

## Saved maps

A download replaces the **active** graph, but not the map that was there:
before anything overwrites the active files, the current graph is copied to
`nav/regions/<key>/` (graph, search index and its source). That is what makes
the second trip cheap:

- The offer for a region that is already saved says so, and its button becomes
  **Switch** — a file copy, not a download.
- *Map data → Saved maps* lists every kept region with its size, marks the one
  in use, and switches on tap. Deleting one is refused for the active map.
- A graph imported from a desktop build is kept as **Imported map** under
  `imported`, so the next region download cannot silently destroy the file
  that was just picked.
- *Delete map data* removes the active graph and every saved map, as its
  confirmation now says.

The active graph lives in the old flat location (`nav/route_graph.bin`), so
installs from before saved maps keep working: the first new download archives
whatever was there.

## Map style

The ground under the map has two renderers, and the one on screen is a tap
away:

- **OpenStreetMap** (the default) — raster tiles from `tile.openstreetmap.org`,
  cached on the phone. Needs a connection unless the area was saved.
- **Offline map** — tiles drawn on the phone from the downloaded region's own
  geometry (roads, water, buildings, bike lanes and labels). Nothing is fetched
  to draw them, so it works with no signal at all.

The switch is the layers button in the navigator's app bar (and *Draw the
offline map* under *Map data → Map tiles*). It is offered only when the phone
holds geometry to draw — currently the Lesser Poland extract. The choice is
persisted and applies to the head unit's feed too, so both screens show the
same style. Both styles keep their own tiles under `tiles/osm/` (graph tiles
in `krakow-v2/`), so switching back and forth neither re-downloads nor
overwrites the other one.

## Routing across saved maps

A route no longer stops at the edge of the active map. When either endpoint is
outside the active graph's envelope, the saved maps that meet the start–goal
box are read in an isolate and merged:

- Nodes and edges are concatenated with their indices rebased.
- Vertices from different regions within 12 m of each other are joined with a
  bidirectional `link` edge — two extracts cut at the same border share no ids,
  so that stitch is what makes the network continuous.
- A graph whose envelope sits inside a larger saved one is dropped, so a
  country saved beside its province does not duplicate every road.
- Each profile's components are no longer limited to the single biggest one:
  a substantial network in any region can be snapped to. The router returns
  immediately when the ends are in different components, so the fallback is
  cheap.
- Search indexes are merged too, sorted by the folded key, so a place in the
  next province is found without switching maps first.

The merged graph is built on demand and kept in memory until the set of saved
maps changes. The tile layer still follows the active map's position, and the
merged graph is not persisted — it is derived data.

## Files

- `lib/nav/region_suggester.dart` — when to ask, and what to ask for.
- `lib/nav/geofabrik.dart` — catalogue parsing, outlines, point-in-polygon,
  disk cache (`GeofabrikIndex.regionAt`, `RegionShape`).
- `lib/nav/map_data.dart` — `covers()`, the source region remembered in
  `graph_source.json`, the catalogue cache paths, the saved-map library
  (`saveActiveMap`, `activateRegion`, `archivedRegions`), and the on-demand
  merge that backs cross-border routing (`route`).
- `lib/nav/rgf2.dart` — the RGF2 reader plus `mergeBytes`: rebasing indices,
  dropping contained envelopes and stitching border vertices.
- `lib/nav/router.dart` — A*, with an early give-up when the two ends are on
  different components.
- `lib/nav/search_index.dart` — `loadMerged`, one sorted index over every
  saved region.
- `lib/ui/navigator_screen.dart` — the banner, the throttled lookup, the
  download-or-switch action, and the map-style button.
- `lib/ui/map_data_screen.dart` — the saved-map list, the offer switch and the
  map-style switch.
- `lib/nav/tile_cache.dart` — one cache for both styles; `useGraphTiles` picks
  the renderer and `fileFor` keeps the two tile stores apart.
- `lib/nav/krakow_tiles.dart`, `lib/nav/krakow_tile_painter.dart` — the offline
  geometry buckets and the renderer that turns them into tiles.
- `lib/ui/region_picker_screen.dart` — "Near you" from a cached position, and
  a pin on regions that need no download. The hint only appears when the
  geometry catalogue has already been fetched for an offer; the picker never
  spends 3.5 MB just to show a row.

## Known limitations

- **Cross-border routing needs both sides saved.** The stitch joins regions
  that meet at a border; a route through a region that is not downloaded still
  has a gap. A route also cannot cross open water or two networks that are
  genuinely disconnected.
- The stitch radius is 12 m. Extracts clipped with a coarser boundary can
  leave a wider gap; the offer to download the neighbouring region still
  appears, but a route may not join until it is.
- The merged graph lives in memory only, and is rebuilt after a restart or a
  library change. Two or three provinces are fine; a country-sized graph
  beside them can be heavy, which is why contained graphs are dropped before
  reading.
- The outlines are simplified, so a point within a few hundred metres of a
  border may resolve to either side. The offer is a suggestion; the picker is
  still there for the exact choice.
- Region sizes are not shown before the first download: Geofabrik's catalogue
  carries no file size, and a HEAD request per region is not reliable. After a
  region is saved its real size is listed.
