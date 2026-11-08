#!/usr/bin/env python3
"""Read state from a JK-BMS over Bluetooth LE.

Examples
--------
    python read_bms.py --scan
    python read_bms.py --address AA:BB:CC:DD:EE:FF
    python read_bms.py --name JK-B --watch 1 --cells
    python read_bms.py --address ... --json
    python read_bms.py --address ... --device-info

Exit codes: 0 success, 2 no adapter / no matching device, 3 protocol error.
"""

from __future__ import annotations

import argparse
import asyncio
import dataclasses
import json
import sys
import time

import jkbms


def _clean(obj):
    if dataclasses.is_dataclass(obj):
        obj = dataclasses.asdict(obj)
    if isinstance(obj, dict):
        return {k: _clean(v) for k, v in obj.items() if k != "raw"}
    if isinstance(obj, list):
        return [_clean(v) for v in obj]
    if isinstance(obj, float):
        return round(obj, 3)
    return obj


async def find_address(args) -> str | None:
    print(f"Scanning {args.timeout:.0f} s for JK-BMS devices...", file=sys.stderr)
    hits = await jkbms.scan(args.timeout, show_all=args.all_devices)
    if not hits:
        print("No JK-BMS found. Make sure it is not connected to the phone app "
              "and is in range.", file=sys.stderr)
        return None

    for i, h in enumerate(hits):
        print(f"  [{i}] {h.address}  {h.rssi:4d} dBm  {h.name}", file=sys.stderr)

    if args.name:
        wanted = args.name.upper()
        match = next((h for h in hits if wanted in h.name.upper()), None)
        if match:
            return match.address
        print(f"No scan hit name contains {args.name!r}.", file=sys.stderr)
        return None

    if len(hits) == 1:
        return hits[0].address

    print("More than one device found — pass --address or --name.",
          file=sys.stderr)
    return None


async def run(args) -> int:
    address = args.address or await find_address(args)
    if not address:
        return 2

    client = jkbms.JkBmsClient(address, name_hint=args.name or "")
    try:
        await client.connect()
    except Exception as exc:  # noqa: BLE001
        print(f"Connect failed: {exc}", file=sys.stderr)
        return 2

    print(f"Connected to {address}", file=sys.stderr)
    try:
        if args.device_info:
            info = await client.read_device_info()
            if args.json:
                print(json.dumps(_clean(info), indent=2))
            else:
                for key, value in info.items():
                    if key != "raw":
                        print(f"  {key:20s}: {value}")
            return 0

        if args.watch:
            def show(st: jkbms.BmsState) -> None:
                if args.json:
                    print(json.dumps(_clean(st)))
                else:
                    print(f"--- {time.strftime('%H:%M:%S')} ---")
                    print(jkbms.format_state(st, show_cells=args.cells))
                sys.stdout.flush()

            await client.stream(show, interval=args.watch, count=args.count)
            return 0

        state = await client.read_state()
        # One device-info round trip gives the model string for the header.
        try:
            info = await client.read_device_info(timeout=2.0)
            state.model = info.get("model", "")
        except TimeoutError:
            pass
        if args.json:
            print(json.dumps(_clean(state), indent=2))
        else:
            print(jkbms.format_state(state, show_cells=args.cells))
        return 0
    except TimeoutError as exc:
        print(f"No reply from BMS: {exc}", file=sys.stderr)
        return 3
    finally:
        await client.disconnect()


def main() -> int:
    p = argparse.ArgumentParser(description=__doc__,
                                formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("--scan", action="store_true",
                   help="scan for JK-BMS devices and print them, then exit")
    p.add_argument("--address", "-a", help="BLE address of the BMS")
    p.add_argument("--name", "-n", help="substring of the BMS name (case-insensitive)")
    p.add_argument("--timeout", type=float, default=8.0,
                   help="scan / reply timeout in seconds (default 8)")
    p.add_argument("--all-devices", action="store_true",
                   help="during scan, list every named BLE device, not just JK/BMS")
    p.add_argument("--watch", type=float, metavar="SECONDS",
                   help="keep polling every SECONDS and print each reading")
    p.add_argument("--count", type=int, default=0,
                   help="with --watch, stop after N readings (0 = forever)")
    p.add_argument("--json", action="store_true", help="machine-readable output")
    p.add_argument("--cells", action="store_true", help="print every cell voltage")
    p.add_argument("--device-info", action="store_true",
                   help="read model / firmware / serial instead of the status")
    args = p.parse_args()

    if args.scan:
        try:
            hits = asyncio.run(jkbms.scan(args.timeout, show_all=args.all_devices))
        except Exception as exc:  # noqa: BLE001
            print(f"BLE scan failed: {exc}", file=sys.stderr)
            return 2
        if not hits:
            print("No matching devices.")
            return 2
        for h in hits:
            print(f"{h.address}  {h.rssi:4d} dBm  {h.name}")
        return 0

    try:
        return asyncio.run(run(args))
    except KeyboardInterrupt:
        return 130


if __name__ == "__main__":
    raise SystemExit(main())
