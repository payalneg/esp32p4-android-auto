# Navigation display

The head unit eases the rider's bearing across the shortest arc, including
359° → 0°. Position correction uses elapsed time, and the predicted GPS target
advances with the rider between phone updates. Composition targets 60 ms per
frame; the existing two-buffer handoff applies backpressure when a frame takes
longer. Prediction stops after 1.5 seconds without VIEW updates, and reconnecting
does not extrapolate across the disconnected interval.

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
