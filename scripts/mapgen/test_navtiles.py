"""Tests for the NVT1 packer/parser and the offline demo package.

Run from this directory:

    python -m unittest test_navtiles -v
"""
from __future__ import annotations

import tempfile
import unittest
from pathlib import Path

import navtiles
import nvtile_format as nvt


class FormatTest(unittest.TestCase):
    def test_zigzag_roundtrip(self):
        for value in (0, 1, -1, 2, -2, 127, -128, 4096, -4096, 100000):
            self.assertEqual(nvt.unzigzag(nvt.zigzag(value)), value)

    def test_pack_parse_roundtrip(self):
        layers = {
            nvt.LAYER_GREEN: [
                nvt.Feature(nvt.GEOM_POLYGON, 0, [[(-10, 20), (100, 20),
                                                   (100, 200), (-10, 200)]],
                            name="", rank=0),
            ],
            nvt.LAYER_ROAD: [
                nvt.Feature(nvt.GEOM_LINE, nvt.ROAD_PRIMARY,
                            [[(0, 0), (500, 100), (1000, 90)]],
                            name="Grodzka", rank=0),
            ],
            nvt.LAYER_LABEL: [
                nvt.Feature(nvt.GEOM_POINT, nvt.LABEL_STREET,
                            [[(500, 100)]], name="Grodzka", rank=3),
            ],
        }
        data = nvt.pack_tile(layers, 17)
        z, parsed = nvt.parse_tile(data)
        self.assertEqual(z, 17)
        self.assertEqual(set(parsed), set(layers))
        road = parsed[nvt.LAYER_ROAD][0]
        self.assertEqual(road.name, "Grodzka")
        self.assertEqual(road.rings[0], [(0, 0), (500, 100), (1000, 90)])
        label = parsed[nvt.LAYER_LABEL][0]
        self.assertEqual((label.name, label.cls, label.rank),
                         ("Grodzka", nvt.LABEL_STREET, 3))

    def test_polygon_holes_survive(self):
        data = nvt.pack_tile({
            nvt.LAYER_GREEN: [
                nvt.Feature(nvt.GEOM_POLYGON, 0,
                            [[(0, 0), (100, 0), (100, 100), (0, 100)],
                             [(20, 20), (80, 20), (80, 80), (20, 80)]],
                            name="Planty", rank=1),
            ],
        }, 16)
        _, parsed = nvt.parse_tile(data)
        self.assertEqual(len(parsed[nvt.LAYER_GREEN][0].rings), 2)

    def test_broken_tile_is_rejected(self):
        data = nvt.pack_tile({
            nvt.LAYER_ROAD: [
                nvt.Feature(nvt.GEOM_LINE, nvt.ROAD_RESIDENTIAL,
                            [[(0, 0), (10, 10)]], name="", rank=0),
            ],
        }, 15)
        self.assertEqual(nvt.parse_tile(data)[0], 15)
        with self.assertRaises(ValueError):
            nvt.parse_tile(data[:-1])
        with self.assertRaises(ValueError):
            nvt.parse_tile(b"XXXX" + data[4:])

    def test_long_feature_fits_the_firmware_budget(self):
        # A river with hundreds of vertices must not make the C reader
        # reject a whole tile.
        ring = [(i, (i * 37) % 4000) for i in range(400)]
        fitted = navtiles.fit_rings([ring])
        self.assertLessEqual(sum(len(r) for r in fitted), 256)
        self.assertGreaterEqual(len(fitted[0]), 2)


class DemoPackageTest(unittest.TestCase):
    def test_demo_builds_parseable_tiles(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp) / "navtiles"
            features = navtiles.build_demo()
            bbox = (50.075, 19.900, 50.045, 19.985)
            tiles, nbytes = navtiles.build_tiles(
                features, bbox, 13, 16, root, verbose=False)
            self.assertGreater(tiles, 20)
            self.assertGreater(nbytes, 10_000)
            self.assertTrue((root / "vector.idx").exists())
            index = nvt.read_index(root / "vector.idx")
            self.assertEqual((index["zmin"], index["zmax"]), (13, 16))
            self.assertEqual(index["tiles"], tiles)

            seen_labels = 0
            for p in root.glob("*/*/*.vtile"):
                z, layers = nvt.parse_tile(p.read_bytes())
                self.assertIn(z, (13, 14, 15, 16))
                self.assertTrue(layers)
                for features in layers.values():
                    for f in features:
                        # The firmware's budgets, not just the packer's.
                        self.assertLessEqual(len(f.rings), 4)
                        self.assertLessEqual(
                            sum(len(r) for r in f.rings), 256)
                        for ring in f.rings:
                            for x, y in ring:
                                self.assertLessEqual(abs(x), 1 << 20)
                                self.assertLessEqual(abs(y), 1 << 20)
                seen_labels += len(layers.get(nvt.LAYER_LABEL, ()))
            self.assertGreater(seen_labels, 0)

    def test_demo_has_the_landmarks(self):
        names = {f.name for f in navtiles.build_demo().items if f.name}
        for name in ("Rynek Główny", "Wawel", "Wisła", "Planty",
                     "Grodzka", "Floriańska"):
            self.assertIn(name, names)


if __name__ == "__main__":
    unittest.main()
