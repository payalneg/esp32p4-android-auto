# Navigation display

The head unit eases the rider's bearing across the shortest arc, including
359° → 0°. Position correction uses elapsed time, and the predicted GPS target
advances with the rider between phone updates. Composition targets 60 ms per
frame; the existing two-buffer handoff applies backpressure when a frame takes
longer. Prediction stops after 1.5 seconds without VIEW updates, and reconnecting
does not extrapolate across the disconnected interval.

From firmware 1.3.51, repeated VIEW coordinates are keepalives, not a new GPS
anchor: resetting a moving prediction to the same old fix twice per second
caused periodic braking/backward corrections with a 1 Hz GPS source. Distinct
positions reset the anchor; repeating one position cannot renew the 1.5-second
prediction distance budget. An explicit zero speed eases back to the last fix.
Fractional e7 travel is retained between frames rather than rounded away.

The absolute Mercator centre now uses double precision once per projection;
route/label points still use local float differences. At zoom 18 the old float
centre advanced in multi-pixel steps even for continuously moving input.
Native regression tests reproduce the old stair steps and repeated-fix rewind
in `tools/tests/test_nav_motion.c`; `scripts/nav_proj_check.c` calls the actual
production centre projection rather than retaining a copied old formula.

VIEW state is published in a protected latest-value slot and consumed by the
map worker. A full event queue cannot discard the newest pose, and BLE cannot
replace the map centre halfway through composition. A queued STOP older than
a resumed VIEW does not stop the new stream.

The worker waits only for the remainder of the 60 ms interval after composing,
with at least one RTOS tick to yield on overruns. Unchanged view/camera/marker
poses do not trigger another map bitmap; tile and route arrivals still force
redraws. Fresh VIEW messages keep the link active even while a parked map is
not producing frames.

Map buffers become reusable after LVGL's refresh-complete monitor callback,
rather than after an extra screen timer tick. The producer is blocked before
the pending flag is cleared for the swap. HUD styles and object visibility
are updated only when their state changes; LVGL v8 invalidates even when
clearing HIDDEN on an already-visible label.

JC4880 bench verification on 2026-11-20 compared the original installed
1.3.47 firmware with 1.3.49 after rebooting both: empty RAM tile cache, the
same 25 NVT1 tiles, 169 route points and 160 identical position/course/speed
inputs at nominal 2 Hz, 5 m/s and 1x time over real Bluetooth. Map-frame
counter rates increased from 3.4 to 9.9 FPS in NORTH and from 4.4 to 7.7 FPS
during the AUTO turn. Identical stationary poses stopped producing map frames.
Seven screen return cycles, clearing the route while NAV was hidden, a moving
GPS gap and screenshot CRC validation also passed. The phone navigation stack
was bypassed in this replay; these short runs do not establish real-road or
overnight endurance, per-frame p95 latency or a video-based tearing assessment.
Other P4 profiles were compiled but not tested on hardware.

## Position delivery

From app 0.3.59, VIEW has its own deadline timer, so downloading a tile or
waiting for its ACK does not suspend position updates. The existing 14-byte
VIEW carries the latest position, course and speed; firmware 1.3.49 already
accepts it during a tile transfer. No new wire format is required.

Navigation GATT writes are serialized. A single pending VIEW slot replaces
superseded positions and is serviced before the next bulk write. An
acknowledged DATA checkpoint after approximately 2 KiB bounds the map backlog
inside the Bluetooth controller. Native writes already in progress cannot be
preempted. Bulk ACK timeouts start after END, with listeners registered before
BEGIN so immediate replies are preserved.

Automated checks cover blocked tile downloads, tile/route ACK waits, newest
position priority, congestion recovery, controller checkpoints and stopping the
position timer while a bulk operation is still suspended. Physical validation
of this app change was not completed because the head unit and phone had been
disconnected; the frame-rate results above belong to the earlier firmware test.

Route colours on both the phone and head unit:

| Colour | Graph classes |
| --- | --- |
| Blue | Ordinary roads, service roads and synthetic links |
| Green | Dedicated cycleways, including paths classified as bicycle-designated |
| Orange | Pedestrian streets, footways, sidewalks, crossings and steps |
| Purple | Other paths and tracks |

The router keeps one type per geometry span, rather than estimating boundaries
from edge lengths. Reverse traversal reverses the type sequence with the geometry.
Types travel through the UI/service handoff, so putting the phone away preserves
the colours.

## BLE compatibility

STATE mode bit 2 advertises typed route support. When present, ROUTE_BEGIN may
have a sixth byte of `1`; each following point is nine bytes:
`[i32 lat_e7][i32 lon_e7][u8 outgoing_type]`, little endian. Types are road `0`,
cycleway `1`, footway `2`, other `3`; the last point's type is unused.
Without this capability the app sends the original eight-byte points. New
firmware accepts both layouts. A zero-point route still clears the route.

## Activity badge

A single-line badge shows phone/app/GPS waits, Bluetooth map or route reception,
SD map loading, missing map tiles and map errors. Brief activity messages remain
visible for 1.2 seconds after the latest chunk. The 480 px panel places the badge
above the speed and battery readouts to avoid overlap; wide panels place it
between them.

Hardware verification is still needed for perceived smoothness, frame timing
under SD/Bluetooth load, and text placement on both panel sizes.
