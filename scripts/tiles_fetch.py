#!/usr/bin/env python3
"""Fill the head unit's microSD tile cache with a whole region at once.

The head unit caches map tiles on its card under /sdcard/navtiles in the
slippy layout {z}/{x}/{y}.png (main/nav_tile_store.c), which is also the shape
the phone app keeps its own cache in. Riding around fills that cache tile by
tile over BLE; this fills it from a computer, where there is bandwidth and a
card reader.

Two ways out:

  --out DIR        write the tree locally, then copy it onto the card
  --upload URL     push each tile to a running head unit over Wi-Fi, through
                   the file server it already has (POST /files/api/upload)

And one way to ask before committing to anything:

  --dry-run        how many tiles and roughly how many megabytes
  --dry-run --probe   ...with the average tile size actually measured

The tile source is yours to name (--url). Whoever serves the tiles sets the
rules for bulk downloads: OSM's own servers forbid them, so a region-sized run
belongs on a source that allows it (your own render, or a provider you have a
key with). --rate exists so a small run stays polite.

Examples:

  # How big is Malopolska at z16, really?
  scripts/tiles_fetch.py --url https://tile.example/{z}/{x}/{y}.png \\
      --bbox 49.07,19.08,50.59,21.55 --zoom 16 --dry-run --probe

  # 5 km around home, three levels, straight onto the card in the reader
  scripts/tiles_fetch.py --url https://tile.example/{z}/{x}/{y}.png \\
      --around 50.02,19.91 --radius-km 5 --zoom 15-17 \\
      --out /Volumes/NAVI/navtiles

  # The same, onto a head unit on the same Wi-Fi
  scripts/tiles_fetch.py --url https://tile.example/{z}/{x}/{y}.png \\
      --around 50.02,19.91 --radius-km 5 --zoom 15-17 \\
      --upload http://192.168.1.50
"""

import argparse
import math
import os
import queue
import random
import sys
import threading
import time
import urllib.error
import urllib.parse
import urllib.request

DEFAULT_UA = ("aa-bridge-tiles/1.0 "
              "(+https://github.com/payalneg/esp32p4-android-auto)")

# What one tile costs when nobody has measured. Urban OSM-Carto PNGs run
# 10-25 KB; fields and forest are a fraction of that.
ASSUMED_TILE_BYTES = 15 * 1024

# Tiles to pull for --probe. Small enough to be nothing to any server.
PROBE_TILES = 20


# ---- slippy map arithmetic -------------------------------------------------

def deg2num(lat, lon, z):
    lat = max(min(lat, 85.05112878), -85.05112878)
    n = 1 << z
    x = int((lon + 180.0) / 360.0 * n)
    r = math.radians(lat)
    y = int((1.0 - math.asinh(math.tan(r)) / math.pi) / 2.0 * n)
    return max(0, min(n - 1, x)), max(0, min(n - 1, y))


def tile_span_m(lat, z):
    """Ground covered by one 256 px tile, in metres."""
    return 40075016.686 * math.cos(math.radians(lat)) / (1 << z)


def bbox_tiles(bbox, z):
    """Inclusive tile ranges covering a (minlat, minlon, maxlat, maxlon) box."""
    minlat, minlon, maxlat, maxlon = bbox
    x0, y0 = deg2num(maxlat, minlon, z)   # north-west
    x1, y1 = deg2num(minlat, maxlon, z)   # south-east
    return min(x0, x1), max(x0, x1), min(y0, y1), max(y0, y1)


def around_bbox(lat, lon, radius_km):
    dlat = radius_km / 111.32
    dlon = radius_km / (111.32 * max(math.cos(math.radians(lat)), 0.01))
    return (lat - dlat, lon - dlon, lat + dlat, lon + dlon)


def parse_zooms(spec):
    out = []
    for part in spec.split(","):
        part = part.strip()
        if not part:
            continue
        if "-" in part:
            a, b = part.split("-", 1)
            out.extend(range(int(a), int(b) + 1))
        else:
            out.append(int(part))
    bad = [z for z in out if not 0 <= z <= 20]
    if bad:
        sys.exit(f"tiles_fetch: zoom out of range: {bad}")
    return sorted(set(out))


def plan(bbox, zooms):
    """[(z, x0, x1, y0, y1, count)] coarsest level first."""
    rows = []
    for z in zooms:
        x0, x1, y0, y1 = bbox_tiles(bbox, z)
        rows.append((z, x0, x1, y0, y1, (x1 - x0 + 1) * (y1 - y0 + 1)))
    return rows


# ---- fetching --------------------------------------------------------------

class Rate:
    """One shared gap between requests, whatever the thread."""

    def __init__(self, per_second):
        self.gap = 1.0 / per_second if per_second > 0 else 0.0
        self.lock = threading.Lock()
        self.next_at = 0.0

    def wait(self):
        if self.gap <= 0:
            return
        with self.lock:
            now = time.monotonic()
            if now < self.next_at:
                delay = self.next_at - now
            else:
                delay = 0.0
                self.next_at = now
            self.next_at += self.gap
        if delay > 0:
            time.sleep(delay)


class Blocked(Exception):
    """The server said no to us, not to this tile — stop the whole run."""


def tile_url(template, z, x, y):
    url = template.replace("{z}", str(z)).replace("{x}", str(x)) \
                  .replace("{y}", str(y))
    if "{s}" in url:
        url = url.replace("{s}", random.choice("abc"))
    return url


def fetch_tile(template, z, x, y, ua, rate, timeout, tries=3):
    last = None
    for attempt in range(tries):
        rate.wait()
        req = urllib.request.Request(tile_url(template, z, x, y),
                                     headers={"User-Agent": ua})
        try:
            with urllib.request.urlopen(req, timeout=timeout) as r:
                return r.read()
        except urllib.error.HTTPError as e:
            if e.code in (403, 429):
                raise Blocked(f"HTTP {e.code} from the tile server")
            if e.code == 404:
                return None            # nothing rendered there; not an error
            last = e
        except Exception as e:          # noqa: BLE001 - network is network
            last = e
        time.sleep(0.5 * (attempt + 1))
    raise RuntimeError(f"z{z}/{x}/{y}: {last}")


# ---- sinks -----------------------------------------------------------------

class DirSink:
    """Write the tree somewhere — a card in a reader, or a staging folder."""

    def __init__(self, root):
        self.root = root

    def check(self):
        """Refuse a card that is not actually there.

        Without this a mistyped volume name quietly creates the tree inside
        /Volumes on the Mac, and the run looks like it worked while the card
        stays empty.
        """
        parent = os.path.dirname(os.path.abspath(self.root.rstrip("/")))
        if not os.path.isdir(parent):
            sys.exit(f"tiles_fetch: {parent} is not there — is the card in?")
        os.makedirs(self.root, exist_ok=True)
        if not os.access(self.root, os.W_OK):
            sys.exit(f"tiles_fetch: cannot write to {self.root}")

    def have(self, z, x, y):
        p = os.path.join(self.root, str(z), str(x), f"{y}.png")
        return os.path.exists(p) and os.path.getsize(p) > 0

    def put(self, z, x, y, data):
        d = os.path.join(self.root, str(z), str(x))
        os.makedirs(d, exist_ok=True)
        tmp = os.path.join(d, f"{y}.part")
        with open(tmp, "wb") as f:
            f.write(data)
        os.replace(tmp, os.path.join(d, f"{y}.png"))


class HeadUnitSink:
    """Push tiles to a running head unit over its own file API.

    One HTTP request per tile, so this is for a city rather than a voivodeship
    — a card reader is the right tool for gigabytes. Directories are made on
    demand and remembered, the way the browser UI does it.
    """

    def __init__(self, base, remote_root, timeout):
        self.base = base.rstrip("/")
        self.root = remote_root.rstrip("/")
        self.timeout = timeout
        self.made = set()
        self.lock = threading.Lock()

    def _post(self, path, body, content_type):
        req = urllib.request.Request(self.base + path, data=body,
                                     headers={"Content-Type": content_type},
                                     method="POST")
        with urllib.request.urlopen(req, timeout=self.timeout) as r:
            return r.read()

    def _mkdir(self, remote_dir):
        with self.lock:
            if remote_dir in self.made:
                return
            self.made.add(remote_dir)
        import json
        try:
            self._post("/files/api/mkdir",
                       json.dumps({"path": remote_dir}).encode(),
                       "application/json")
        except urllib.error.HTTPError:
            pass        # already there is the usual answer, and it is fine

    def check(self):
        """Is there a card in the head unit at all?

        The firmware only caches tiles when one is mounted, and the file API
        answers a missing card with an error per upload — so ask once, plainly,
        instead of pushing a region into a stream of 500s.
        """
        root = self.root.split("/")[1] if self.root.startswith("/") else ""
        probe = "/" + root if root else "/sdcard"
        q = urllib.parse.quote(probe, safe="")
        try:
            req = urllib.request.Request(
                f"{self.base}/files/api/list?path={q}")
            with urllib.request.urlopen(req, timeout=self.timeout) as r:
                r.read()
        except urllib.error.HTTPError as e:
            sys.exit(f"tiles_fetch: the head unit has no card in it "
                     f"({probe}: HTTP {e.code}) — put one in first")
        except Exception as e:      # noqa: BLE001
            sys.exit(f"tiles_fetch: no answer from {self.base} ({e})")
        self._mkdir(self.root)

    def have(self, z, x, y):
        return False    # the head unit overwrites; asking costs a round trip

    def put(self, z, x, y, data):
        remote = f"{self.root}/{z}/{x}/{y}.png"
        q = urllib.parse.quote(remote, safe="")
        try:
            self._post(f"/files/api/upload?path={q}", data,
                       "application/octet-stream")
            return
        except urllib.error.HTTPError:
            pass
        # Most likely the directory is not there yet.
        self._mkdir(f"{self.root}/{z}")
        self._mkdir(f"{self.root}/{z}/{x}")
        self._post(f"/files/api/upload?path={q}", data,
                   "application/octet-stream")


# ---- the run ---------------------------------------------------------------

def human(n):
    for unit in ("B", "KB", "MB", "GB", "TB"):
        if n < 1024 or unit == "TB":
            return f"{n:.1f} {unit}" if unit != "B" else f"{int(n)} B"
        n /= 1024.0
    return ""


def do_dry_run(args, rows, bbox):
    total = sum(r[5] for r in rows)
    per_tile = ASSUMED_TILE_BYTES
    measured = ""
    if args.probe:
        rate = Rate(args.rate)
        mid_lat = (bbox[0] + bbox[2]) / 2.0
        got, size = 0, 0
        for z, x0, x1, y0, y1, _ in rows:
            for _ in range(max(1, PROBE_TILES // len(rows))):
                x = random.randint(x0, x1)
                y = random.randint(y0, y1)
                try:
                    data = fetch_tile(args.url, z, x, y, args.user_agent, rate,
                                      args.timeout)
                except Blocked as e:
                    sys.exit(f"tiles_fetch: {e}")
                except RuntimeError:
                    continue
                if data:
                    got += 1
                    size += len(data)
        if got:
            per_tile = size / got
            measured = f" (measured over {got} tiles)"
        _ = mid_lat

    print(f"source   {args.url}")
    print(f"bbox     {bbox[0]:.4f},{bbox[1]:.4f} .. {bbox[2]:.4f},{bbox[3]:.4f}")
    lat = (bbox[0] + bbox[2]) / 2.0
    for z, x0, x1, y0, y1, n in rows:
        span = tile_span_m(lat, z)
        print(f"  z{z:<3} {n:>10,} tiles   tile = {span:6.0f} m   "
              f"{human(n * per_tile):>10}")
    print(f"total    {total:,} tiles, about {human(total * per_tile)}"
          f"{measured}")
    if args.rate > 0:
        hours = total / args.rate / 3600.0
        print(f"time     {hours:.1f} h at {args.rate} tiles/s")


def do_fetch(args, rows, sink):
    jobs = queue.Queue(maxsize=4096)
    rate = Rate(args.rate)
    counts = {"done": 0, "skipped": 0, "empty": 0, "failed": 0}
    counts_lock = threading.Lock()
    stop = threading.Event()
    total = sum(r[5] for r in rows)
    started = time.monotonic()

    def bump(key, n=1):
        with counts_lock:
            counts[key] += n
            done = counts["done"] + counts["skipped"] + counts["empty"] + \
                counts["failed"]
        if done % 50 == 0 or done == total:
            el = time.monotonic() - started
            speed = done / el if el > 0 else 0
            left = (total - done) / speed if speed > 0 else 0
            sys.stderr.write(
                f"\r{done:,}/{total:,}  ok={counts['done']:,} "
                f"skip={counts['skipped']:,} empty={counts['empty']:,} "
                f"fail={counts['failed']:,}  {speed:.1f}/s  "
                f"eta {left / 60:.0f} min   ")
            sys.stderr.flush()

    def worker():
        while not stop.is_set():
            try:
                z, x, y = jobs.get(timeout=0.5)
            except queue.Empty:
                if stop.is_set():
                    return
                continue
            try:
                if sink.have(z, x, y):
                    bump("skipped")
                    continue
                data = fetch_tile(args.url, z, x, y, args.user_agent, rate,
                                  args.timeout)
                if data is None:
                    bump("empty")
                    continue
                sink.put(z, x, y, data)
                bump("done")
            except Blocked as e:
                sys.stderr.write(f"\ntiles_fetch: {e} — stopping\n")
                stop.set()
            except Exception as e:      # noqa: BLE001
                if args.verbose:
                    sys.stderr.write(f"\nz{z}/{x}/{y}: {e}\n")
                bump("failed")
            finally:
                jobs.task_done()

    threads = [threading.Thread(target=worker, daemon=True)
               for _ in range(args.jobs)]
    for t in threads:
        t.start()

    # Coarse levels first: they are cheap and they are what makes a map look
    # like a map before the detail lands — the same order the head unit reads
    # its cache in.
    for z, x0, x1, y0, y1, _ in rows:
        for y in range(y0, y1 + 1):
            for x in range(x0, x1 + 1):
                if stop.is_set():
                    break
                jobs.put((z, x, y))
            if stop.is_set():
                break
        if stop.is_set():
            break

    jobs.join()
    stop.set()
    for t in threads:
        t.join(timeout=1.0)
    sys.stderr.write("\n")
    print(f"done: {counts['done']:,} fetched, {counts['skipped']:,} already "
          f"there, {counts['empty']:,} empty, {counts['failed']:,} failed")
    return 1 if counts["failed"] else 0


def main():
    ap = argparse.ArgumentParser(
        description="Fill the head unit's microSD tile cache.",
        epilog=__doc__.split("Examples:", 1)[1],
        formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--url", required=True,
                    help="tile URL template with {z}/{x}/{y} (and optional {s})")
    ap.add_argument("--bbox", help="minlat,minlon,maxlat,maxlon")
    ap.add_argument("--around", help="lat,lon — with --radius-km")
    ap.add_argument("--radius-km", type=float, default=5.0)
    ap.add_argument("--zoom", default="15-17",
                    help="levels, e.g. 16 or 14-17 or 14,16 (default 15-17)")
    ap.add_argument("--out", help="write the tree here")
    ap.add_argument("--upload", metavar="URL",
                    help="push to a head unit, e.g. http://192.168.1.50")
    ap.add_argument("--remote-root", default="/sdcard/navtiles",
                    help="where the tiles live on the card (default "
                         "/sdcard/navtiles)")
    ap.add_argument("--dry-run", action="store_true",
                    help="count the tiles and estimate the size")
    ap.add_argument("--probe", action="store_true",
                    help="with --dry-run: measure the average tile instead of "
                         "assuming 15 KB")
    ap.add_argument("--rate", type=float, default=2.0,
                    help="requests per second, shared by all workers "
                         "(default 2)")
    ap.add_argument("--jobs", type=int, default=2,
                    help="parallel workers (default 2)")
    ap.add_argument("--timeout", type=float, default=20.0)
    ap.add_argument("--user-agent", default=DEFAULT_UA)
    ap.add_argument("-v", "--verbose", action="store_true",
                    help="print every failure, not just the tally")
    args = ap.parse_args()

    if bool(args.bbox) == bool(args.around):
        sys.exit("tiles_fetch: give exactly one of --bbox and --around")
    if args.bbox:
        try:
            nums = [float(v) for v in args.bbox.split(",")]
            minlat, minlon, maxlat, maxlon = nums
        except ValueError:
            sys.exit("tiles_fetch: --bbox wants minlat,minlon,maxlat,maxlon")
        bbox = (min(minlat, maxlat), min(minlon, maxlon),
                max(minlat, maxlat), max(minlon, maxlon))
    else:
        try:
            lat, lon = (float(v) for v in args.around.split(","))
        except ValueError:
            sys.exit("tiles_fetch: --around wants lat,lon")
        bbox = around_bbox(lat, lon, args.radius_km)

    rows = plan(bbox, parse_zooms(args.zoom))

    if args.dry_run:
        do_dry_run(args, rows, bbox)
        return 0
    if bool(args.out) == bool(args.upload):
        sys.exit("tiles_fetch: give exactly one of --out and --upload "
                 "(or --dry-run)")

    sink = DirSink(args.out) if args.out \
        else HeadUnitSink(args.upload, args.remote_root, args.timeout)
    # No card, no run: the whole point is what ends up on it.
    sink.check()
    total = sum(r[5] for r in rows)
    print(f"{total:,} tiles, about {human(total * ASSUMED_TILE_BYTES)} at "
          f"{args.rate} tiles/s -> "
          f"{args.out or (args.upload + args.remote_root)}")
    return do_fetch(args, rows, sink)


if __name__ == "__main__":
    sys.exit(main())
