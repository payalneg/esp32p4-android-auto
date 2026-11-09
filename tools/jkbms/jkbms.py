"""JK-BMS (Jikong) BLE client and frame decoder.

Implements the protocol used by the JK-BMS Android app / ESPHome integration:

    service       0000ffe0-0000-1000-8000-00805f9b34fb
    characteristic 0000ffe1-0000-1000-8000-00805f9b34fb (write + notify)

A request is a fixed 20-byte frame

    AA 55 90 EB <cmd> 00 00 00 00 00 00 00 00 00 00 00 00 00 00 <crc>

with <cmd> = 0x96 (cell info / status) or 0x97 (device info) and <crc> the
8-bit sum of the first 19 bytes.

The reply is a 300-byte frame

    55 AA EB 90 <type> <counter> <payload ...> <crc>

(type 0x01 settings, 0x02 cell info, 0x03 device info, 0x05 logbook).
Notifications are split across ATT packets, so the receiver must reassemble.

Decoding follows the layouts documented by syssi/esphome-jk-bms
(components/jk_bms_ble/jk_bms_ble.cpp, MIT). Three variants are supported:

    JK02_24S  24 cell slots, uint16 millivolt cells (most JK-B* BMS)
    JK02_32S  32 cell slots, same layout shifted by 32 bytes
              (JK-PB2A16S-20P etc.)
    JK04      older JK-B2A16S v3, IEEE-754 float cells, no SOC/Ah fields

The 24S/32S choice is auto-detected: both layouts carry an enabled-cell
bitmask, average cell voltage and delta right after the cell array, so the
decoder validates those against the populated slots.  This also handles a
16S pack that still uses the 32-cell frame layout (JK-BD6A24S6PD V20), where
cells 17..32 are empty and the old "cell 28 is present" shortcut failed.
See detect_variant().

Only the fields needed by the head unit are decoded (SOC, remaining/full Ah,
voltage, current, temperatures, cell extremes, cycles, SOH, MOSFET state);
the raw frame is kept so more can be added later.

No hardware is required to test the decoder: ``python -m unittest`` runs the
frame test vectors captured from real BMS units.
"""

from __future__ import annotations

import asyncio
import dataclasses
import struct
import time
from typing import Callable, Iterable, Optional

JK_SERVICE_UUID = "0000ffe0-0000-1000-8000-00805f9b34fb"
JK_CHAR_UUID = "0000ffe1-0000-1000-8000-00805f9b34fb"

FRAME_HEADER = b"\x55\xaa\xeb\x90"
REQUEST_HEADER = b"\xaa\x55\x90\xeb"

CMD_CELL_INFO = 0x96
CMD_DEVICE_INFO = 0x97

FRAME_SIZE = 300
MAX_FRAME_SIZE = 320

TYPE_SETTINGS = 0x01
TYPE_CELL_INFO = 0x02
TYPE_DEVICE_INFO = 0x03
TYPE_LOGBOOK = 0x05


# ---------------------------------------------------------------------------
# frame helpers
# ---------------------------------------------------------------------------


def checksum(data: bytes) -> int:
    """JK-BMS checksum: sum of the covered bytes, modulo 256."""
    return sum(data) & 0xFF


def build_request(command: int, value: int = 0, length: int = 0) -> bytes:
    """Build a 20-byte request frame."""
    frame = bytearray(20)
    frame[0:4] = REQUEST_HEADER
    frame[4] = command
    frame[5] = length
    frame[6:10] = int(value).to_bytes(4, "little")
    frame[19] = checksum(frame[:19])
    return bytes(frame)


def frame_is_valid(frame: bytes) -> bool:
    return (
        len(frame) == FRAME_SIZE
        and frame[:4] == FRAME_HEADER
        and checksum(frame[: FRAME_SIZE - 1]) == frame[FRAME_SIZE - 1]
    )


class FrameAssembler:
    """Reassembles 300-byte frames from BLE notification chunks."""

    def __init__(self) -> None:
        self._buf = bytearray()

    def reset(self) -> None:
        self._buf.clear()

    def feed(self, chunk: bytes) -> list[bytes]:
        buf = self._buf
        buf.extend(chunk)

        # A fresh header means the previous frame was truncated/lost.
        start = buf.find(FRAME_HEADER)
        if start < 0:
            # Keep only a possible partial header tail.
            if len(buf) > 3:
                del buf[:-3]
            return []
        if start > 0:
            del buf[:start]

        frames: list[bytes] = []
        while len(buf) >= FRAME_SIZE:
            candidate = bytes(buf[:FRAME_SIZE])
            if frame_is_valid(candidate):
                frames.append(candidate)
                del buf[:FRAME_SIZE]
                continue
            # Bad CRC: skip this header and hunt for the next one.
            nxt = buf.find(FRAME_HEADER, 1)
            if nxt < 0:
                del buf[: FRAME_SIZE - 3]
                break
            del buf[:nxt]
        if len(buf) > MAX_FRAME_SIZE:
            self.reset()
        return frames


# ---------------------------------------------------------------------------
# decoded state
# ---------------------------------------------------------------------------


@dataclasses.dataclass
class BmsState:
    variant: str = "unknown"
    frame_type: int = 0
    counter: int = 0
    model: str = ""

    soc: Optional[float] = None          # percent 0..100
    remaining_ah: Optional[float] = None
    full_ah: Optional[float] = None
    cycles: Optional[int] = None
    soh: Optional[float] = None
    voltage: Optional[float] = None      # V
    current: Optional[float] = None      # A, positive = charging
    power: Optional[float] = None        # W, positive = charging
    temp1: Optional[float] = None        # deg C
    temp2: Optional[float] = None
    temp_mos: Optional[float] = None
    cell_min: Optional[float] = None     # V
    cell_max: Optional[float] = None
    cell_delta: Optional[float] = None
    cell_min_index: Optional[int] = None
    cell_max_index: Optional[int] = None
    cells: list[float] = dataclasses.field(default_factory=list)

    charging_mos: Optional[bool] = None
    discharging_mos: Optional[bool] = None
    balancing: Optional[bool] = None
    heating: Optional[bool] = None

    raw: bytes = b""

    def __str__(self) -> str:
        return format_state(self)


def _u16(data: bytes, off: int) -> int:
    return data[off] | (data[off + 1] << 8)


def _i16(data: bytes, off: int) -> int:
    v = _u16(data, off)
    return v - 0x10000 if v & 0x8000 else v


def _u32(data: bytes, off: int) -> int:
    return (
        data[off]
        | (data[off + 1] << 8)
        | (data[off + 2] << 16)
        | (data[off + 3] << 24)
    )


def _i32(data: bytes, off: int) -> int:
    v = _u32(data, off)
    return v - 0x100000000 if v & 0x80000000 else v


def _f32(data: bytes, off: int) -> float:
    return struct.unpack_from("<f", data, off)[0]


def _text(data: bytes, off: int, length: int) -> str:
    raw = data[off : off + length]
    return raw.split(b"\x00", 1)[0].decode("ascii", "replace").strip()


def _popcount32(value: int) -> int:
    return bin(value & 0xFFFFFFFF).count("1")


def _layout_score(
    data: bytes, *, cell_slots: int, mask_off: int, avg_off: int, delta_off: int
) -> Optional[int]:
    """How well a JK02 layout matches the frame's own summary fields.

    JK02_24S and JK02_32S differ by 32 bytes from the enabled-cell bitmask
    onward.  The old detector only looked at bytes 60..61 ("cell 28 is a real
    voltage on 32S"), which fails on a 16S pack in a 32S frame: cell 28 is
    empty, so bytes 60..61 are zero and the frame looked like 24S.  Validate
    the bitmask/average/delta block against the actual cells instead; it works
    for partially populated packs too.
    """
    mask = _u32(data, mask_off)
    count = _popcount32(mask)
    if not (1 <= count <= cell_slots):
        return None

    avg = _u16(data, avg_off)
    delta = _u16(data, delta_off)
    if not (1000 <= avg <= 5000) or delta > 250:
        return None

    cells = [_u16(data, 6 + 2 * i) for i in range(cell_slots)]
    live = [v for v in cells if 1000 <= v <= 5000]
    if len(live) != count:
        return None

    mean = sum(live) / len(live)
    if abs(avg - mean) > 20:
        return None
    if abs((max(live) - min(live)) - delta) > 20:
        return None
    return abs(avg - mean)


def detect_variant(data: bytes, hint: str = "") -> str:
    """Return 'JK02_32S', 'JK02_24S' or 'JK04' for a cell-info frame."""
    hint = (hint or "").upper()
    if "JK04" in hint:
        return "JK04"

    # JK04 stores IEEE-754 floats for the cell voltages; JK02 uint16 mV.
    first = _u16(data, 6)
    first_float = _f32(data, 6)
    if not (1000 <= first <= 5000) and 1.0 <= first_float <= 5.0:
        return "JK04"

    # Validate both layouts and pick the better match.  A 16S pack in a 32S
    # frame leaves cells 17..32 empty, so the old "is cell 28 present?"
    # shortcut was not enough.
    score32 = _layout_score(
        data, cell_slots=32, mask_off=70, avg_off=74, delta_off=76
    )
    score24 = _layout_score(
        data, cell_slots=24, mask_off=54, avg_off=58, delta_off=60
    )
    if score32 is not None and (score24 is None or score32 <= score24):
        return "JK02_32S"
    if score24 is not None:
        return "JK02_24S"

    # Fallback for frames whose summary fields are inconsistent: 32S has a
    # real voltage in cell 28's slot, 24S has the (small) delta there.
    if _u16(data, 60) > 1000:
        return "JK02_32S"
    return "JK02_24S"


def _cell_extremes(cells: Iterable[float]):
    best_min = (100.0, 0)
    best_max = (-100.0, 0)
    for i, v in enumerate(cells, start=1):
        if v <= 0:
            continue
        if v < best_min[0]:
            best_min = (v, i)
        if v > best_max[0]:
            best_max = (v, i)
    if best_max[0] < 0:
        return None, None, None, None
    return best_min[0], best_max[0], best_min[1], best_max[1]


def parse_cell_info(data: bytes, hint: str = "") -> BmsState:
    if len(data) != FRAME_SIZE or data[:4] != FRAME_HEADER:
        raise ValueError("not a 300-byte JK-BMS frame")
    if data[4] != TYPE_CELL_INFO:
        raise ValueError(f"not a cell-info frame (type 0x{data[4]:02X})")

    variant = detect_variant(data, hint)
    st = BmsState(variant=variant, frame_type=data[4], counter=data[5], raw=data)

    if variant == "JK04":
        cells: list[float] = []
        for i in range(24):
            v = _f32(data, 6 + i * 4)
            if 0.5 < v < 5.0:
                cells.append(round(v, 3))
            else:
                cells.append(0.0)
        st.cells = cells
        st.voltage = round(sum(cells), 3)
        st.charging_mos = None  # not exposed by this old frame
        st.balancing = data[220] != 0
        st.temp_mos = None
        st.temp1 = None
        st.temp2 = None
        return st

    off = 16 if variant == "JK02_32S" else 0
    o = off * 2  # byte offset applied to the later (already 24S) offsets
    cell_count = 24 + off // 2

    cells = []
    for i in range(cell_count):
        cells.append(round(_u16(data, 6 + i * 2) / 1000.0, 3))
    st.cells = cells

    st.voltage = round(_u32(data, 118 + o) / 1000.0, 3)
    st.current = round(_i32(data, 126 + o) / 1000.0, 3)
    st.power = round(st.voltage * st.current, 1)

    st.temp1 = round(_i16(data, 130 + o) / 10.0, 1)
    st.temp2 = round(_i16(data, 132 + o) / 10.0, 1)
    if variant == "JK02_32S":
        st.temp_mos = round(_i16(data, 112 + o) / 10.0, 1)
    else:
        st.temp_mos = round(_i16(data, 134 + o) / 10.0, 1)

    st.balancing = data[140 + o] != 0
    soc = data[141 + o]
    st.soc = float(soc) if soc <= 100 else None
    st.remaining_ah = round(_u32(data, 142 + o) / 1000.0, 3)
    st.full_ah = round(_u32(data, 146 + o) / 1000.0, 3)
    st.cycles = _u32(data, 150 + o)
    soh = data[158 + o]
    st.soh = float(soh) if 0 < soh <= 100 else None

    st.charging_mos = bool(data[166 + o])
    st.discharging_mos = bool(data[167 + o])
    st.heating = bool(data[183 + o]) if len(data) > 183 + o else None

    cmin, cmax, cimin, cimax = _cell_extremes(cells)
    st.cell_min, st.cell_max = cmin, cmax
    st.cell_min_index, st.cell_max_index = cimin, cimax
    if cmin is not None and cmax is not None:
        st.cell_delta = round(cmax - cmin, 3)

    return st


def parse_device_info(data: bytes) -> dict:
    if len(data) != FRAME_SIZE or data[:4] != FRAME_HEADER:
        raise ValueError("not a 300-byte JK-BMS frame")
    if data[4] != TYPE_DEVICE_INFO:
        raise ValueError(f"not a device-info frame (type 0x{data[4]:02X})")
    return {
        "type": TYPE_DEVICE_INFO,
        "counter": data[5],
        "model": _text(data, 6, 16),
        "hardware_version": _text(data, 22, 8),
        "software_version": _text(data, 30, 8),
        "uptime_s": _u32(data, 38),
        "power_on_count": _u32(data, 42),
        "device_name": _text(data, 46, 16),
        "manufacturing_date": _text(data, 78, 6),
        "serial_number": _text(data, 86, 11),
        "raw": data,
    }


def parse_settings_capacity(data: bytes) -> Optional[float]:
    """Nominal battery capacity from a settings frame (type 0x01), if valid."""
    if len(data) != FRAME_SIZE or data[:4] != FRAME_HEADER:
        return None
    if data[4] != TYPE_SETTINGS:
        return None
    cap = _u32(data, 130) / 1000.0
    return cap if 0.0 < cap < 10000.0 else None


def parse_frame(data: bytes, hint: str = "") -> dict:
    """Dispatch one reassembled frame to the matching parser."""
    if data[:4] != FRAME_HEADER:
        raise ValueError("bad header")
    typ = data[4]
    if typ == TYPE_CELL_INFO:
        return dataclasses.asdict(parse_cell_info(data, hint))
    if typ == TYPE_DEVICE_INFO:
        return parse_device_info(data)
    return {"type": typ, "counter": data[5], "raw": data}


# ---------------------------------------------------------------------------
# pretty printing
# ---------------------------------------------------------------------------


def format_state(st: BmsState, show_cells: bool = False) -> str:
    lines = []
    model = f" ({st.model})" if st.model else ""
    lines.append(f"JK-BMS {st.variant}{model}")
    if st.soc is not None:
        lines.append(f"  SOC              : {st.soc:.0f} %")
    if st.remaining_ah is not None and st.full_ah is not None:
        lines.append(
            f"  Capacity         : {st.remaining_ah:.2f} Ah / {st.full_ah:.2f} Ah"
        )
    if st.voltage is not None:
        lines.append(f"  Voltage          : {st.voltage:.2f} V")
    if st.current is not None:
        direction = "charge" if st.current > 0 else "discharge"
        lines.append(f"  Current          : {st.current:+.2f} A ({direction})")
        if st.power is not None:
            lines.append(f"  Power            : {st.power:+.1f} W")
    if st.temp1 is not None or st.temp2 is not None or st.temp_mos is not None:
        parts = []
        if st.temp_mos is not None:
            parts.append(f"MOS {st.temp_mos:.1f}")
        if st.temp1 is not None:
            parts.append(f"T1 {st.temp1:.1f}")
        if st.temp2 is not None:
            parts.append(f"T2 {st.temp2:.1f}")
        lines.append("  Temperatures     : " + ", ".join(parts) + " C")
    if st.cell_min is not None and st.cell_max is not None:
        lines.append(
            f"  Cells            : {st.cell_min:.3f} .. {st.cell_max:.3f} V "
            f"(delta {st.cell_delta * 1000:.0f} mV)"
        )
    extra = []
    if st.cycles is not None:
        extra.append(f"cycles {st.cycles}")
    if st.soh is not None:
        extra.append(f"SOH {st.soh:.0f}%")
    if st.charging_mos is not None:
        extra.append(f"charge MOS {'on' if st.charging_mos else 'off'}")
    if st.discharging_mos is not None:
        extra.append(f"discharge MOS {'on' if st.discharging_mos else 'off'}")
    if st.balancing:
        extra.append("balancing")
    if st.heating:
        extra.append("heating")
    if extra:
        lines.append("  Status           : " + ", ".join(extra))
    if show_cells and st.cells:
        cells = [f"{i + 1}:{v:.3f}" for i, v in enumerate(st.cells) if v > 0]
        lines.append("  Cell voltages    : " + " ".join(cells))
    return "\n".join(lines)


# ---------------------------------------------------------------------------
# BLE client
# ---------------------------------------------------------------------------


@dataclasses.dataclass
class ScanHit:
    address: str
    name: str
    rssi: int


def looks_like_jk(name: Optional[str], service_uuids: Iterable[str] = ()) -> bool:
    n = (name or "").upper()
    if "JK" in n or "BMS" in n or "JIKONG" in n:
        return True
    for u in service_uuids:
        if "ffe0" in u.lower():
            return True
    return False


async def scan(timeout: float = 8.0, show_all: bool = False) -> list[ScanHit]:
    """Scan for JK-BMS advertisers (or every named device with show_all)."""
    from bleak import BleakScanner

    found = await BleakScanner.discover(timeout=timeout, return_adv=True)
    hits: list[ScanHit] = []
    for address, (dev, adv) in found.items():
        name = dev.name or adv.local_name or ""
        uuids = list(adv.service_uuids or [])
        if not show_all and not looks_like_jk(name, uuids):
            continue
        if not name and not show_all:
            continue
        hits.append(ScanHit(address=address, name=name, rssi=adv.rssi))
    hits.sort(key=lambda h: h.rssi, reverse=True)
    return hits


class JkBmsClient:
    """Minimal async BLE client for one JK-BMS.

    Example:
        async with JkBmsClient("AA:BB:CC:DD:EE:FF") as bms:
            state = await bms.read_state()
            print(state)
    """

    def __init__(self, address: str, *, name_hint: str = "", retries: int = 3):
        self.address = address
        self.name_hint = name_hint
        self.retries = retries
        self._client = None
        self._assembler = FrameAssembler()
        self._queue: asyncio.Queue[bytes] = asyncio.Queue()
        self._notify_char = None
        self._write_char = None
        self._write_needs_response = True
        self._connected = False

    async def __aenter__(self) -> "JkBmsClient":
        await self.connect()
        return self

    async def __aexit__(self, *exc) -> None:
        await self.disconnect()

    @property
    def connected(self) -> bool:
        return self._connected

    async def connect(self) -> None:
        from bleak import BleakClient

        last_error: Optional[Exception] = None
        for attempt in range(1, self.retries + 1):
            try:
                client = BleakClient(self.address, timeout=20.0)
                await client.connect()
                service = client.services.get_service(JK_SERVICE_UUID)
                if service is None:
                    raise RuntimeError(
                        "JK BMS service 0xFFE0 not found "
                        "(is this a JK-BMS, and is another app connected?)"
                    )

                notify_candidates = []
                write_candidates = []
                for ch in service.characteristics:
                    props = {p.lower() for p in ch.properties}
                    if "notify" in props or "indicate" in props:
                        notify_candidates.append(ch)
                    if "write" in props or "write-without-response" in props:
                        write_candidates.append(ch)
                if not notify_candidates or not write_candidates:
                    raise RuntimeError(
                        "JK BMS characteristic 0xFFE1 lacks notify/write properties"
                    )

                # FFF1 appears twice on some units (one write-only, one
                # notify-only); pick the first of each kind.
                self._notify_char = notify_candidates[0]
                self._write_char = write_candidates[0]
                self._write_needs_response = (
                    "write" in {p.lower() for p in self._write_char.properties}
                )

                self._assembler.reset()
                await client.start_notify(self._notify_char, self._on_notify)
                self._client = client
                self._connected = True

                # Some JK firmware (e.g. BD6A24S6PD V20, 20.28) answers the
                # first command with a settings/device-info dump and ignores
                # an immediate 0x96, then starts streaming cell info.  Prime
                # it with 0x97 and give it a moment so read_state() succeeds
                # on the first try.
                try:
                    await self._request(CMD_DEVICE_INFO)
                    await asyncio.sleep(2.0)
                except Exception:  # noqa: BLE001 - warm-up is best effort
                    pass
                self._assembler.reset()
                while not self._queue.empty():
                    self._queue.get_nowait()
                return
            except Exception as exc:  # noqa: BLE001 - report the real cause
                last_error = exc
                try:
                    await client.disconnect()  # type: ignore[union-attr]
                except Exception:
                    pass
                if attempt < self.retries:
                    await asyncio.sleep(1.0 * attempt)
        raise RuntimeError(f"cannot connect to {self.address}: {last_error}")

    async def disconnect(self) -> None:
        client = self._client
        self._client = None
        self._connected = False
        if client is not None:
            try:
                await client.stop_notify(self._notify_char)
            except Exception:
                pass
            try:
                await client.disconnect()
            except Exception:
                pass

    def _on_notify(self, _sender, data: bytearray) -> None:
        for frame in self._assembler.feed(bytes(data)):
            self._queue.put_nowait(frame)

    async def _request(self, command: int) -> None:
        if not self._client or not self._write_char:
            raise RuntimeError("not connected")
        frame = build_request(command)
        await self._client.write_gatt_char(
            self._write_char, frame, response=self._write_needs_response
        )

    async def _wait_frame(self, frame_type: int, timeout: float) -> bytes:
        deadline = asyncio.get_running_loop().time() + timeout
        while True:
            remaining = deadline - asyncio.get_running_loop().time()
            if remaining <= 0:
                raise TimeoutError("timed out waiting for a JK-BMS reply")
            frame = await asyncio.wait_for(self._queue.get(), remaining)
            if frame[4] == frame_type:
                return frame

    async def read_state(self, timeout: float = 5.0) -> BmsState:
        # Drop anything stale from a previous request.
        while not self._queue.empty():
            self._queue.get_nowait()
        await self._request(CMD_CELL_INFO)
        frame = await self._wait_frame(TYPE_CELL_INFO, timeout)
        return parse_cell_info(frame, self.name_hint)

    async def read_device_info(self, timeout: float = 5.0) -> dict:
        while not self._queue.empty():
            self._queue.get_nowait()
        await self._request(CMD_DEVICE_INFO)
        frame = await self._wait_frame(TYPE_DEVICE_INFO, timeout)
        return parse_device_info(frame)

    async def stream(
        self,
        callback: Callable[[BmsState], None],
        *,
        interval: float = 1.0,
        count: int = 0,
    ) -> None:
        """Poll the BMS and call callback with every decoded state.

        count=0 means run until cancelled.
        """
        n = 0
        while count == 0 or n < count:
            try:
                callback(await self.read_state())
            except TimeoutError:
                print(f"[{time.strftime('%H:%M:%S')}] no reply (retrying)")
            n += 1
            if count == 0 or n < count:
                await asyncio.sleep(interval)
