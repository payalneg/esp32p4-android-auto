# Changelog

Two version trains ship together and are bumped in lockstep by
`scripts/release.sh`: the **ESP32-P4 firmware** (`1.x.y`, `version.txt`) and the
**Flutter companion app** (`0.x.y`, `pubspec.yaml`). The external BT-agent
firmware is versioned separately and is only bumped when `tools/bt_agent/`
changes.

Entries below name the firmware version; the app version of the same release is
the one recorded in the release commit.

## v1.3.19 / app 0.3.19 — 2026-09-12

### Two things the first ride with the navigator showed

- **Blank squares on the map while panning.** The tiles were there — decoded
  and waiting in the store — but their squares stayed empty. Composing a view
  needs the back buffer, and that buffer is held from the moment a frame is
  committed until the screen has shown it, 50 to 100 ms. Tiles arrive faster
  than that, so most of them asked for a redraw that was quietly dropped, and
  nothing asked again: the sweep that would have caught it only runs after
  120 ms of silence, which a stream of tiles never gives. A dropped redraw is
  now remembered and taken as soon as the buffer frees.
- **The stop button sat on top of the distance left.** The buttons were pinned
  a fixed distance off the bottom of the phone screen and the info bar grew up
  to meet them — with a download button in it, or a longer sentence, the two
  overlapped. They are one bottom-anchored column now, so neither has to guess
  how tall the other is.

## v1.3.18 / app 0.3.18 — 2026-09-12

### The controller can be reached over Bluetooth instead of CAN

- The head unit now talks to the VESC over Bluetooth as well as over the CAN
  bus: it connects as a GATT central to a **VESC Express** adapter over the
  Nordic UART Service — the same link VESC Tool uses — so a display can be
  fitted with no CAN wiring at all. Pick the transport in **Settings -> VESC
  link**; it applies immediately, no reboot. The adapter is paired by scanning
  from that same screen, and its address lives in its own NVS namespace, so it
  survives the settings Reset button the way the sensors do.
- Nothing above the transport changed. The dashboard, the quick-action drawer,
  pedal assist, the LISP editor, the config menu and VESC Tool bridged from the
  phone all go through one new seam (`vesc_link`), and replies from either
  transport land in the same dispatcher they always did.
- A VESC Express sits on the CAN bus next to the controller, so requests are
  wrapped in `COMM_FORWARD_CAN` addressed to Target VESC ID — that is what
  VESC Tool's "scan CAN bus" does. The switch on the link screen turns it off
  for a setup where the adapter itself is the target.
- Pedal assist keeps its 20 Hz setpoint stream, with one rule the CAN path
  never needed: a setpoint older than 100 ms is dropped rather than sent. A
  stalled link would otherwise flush a burst of currents the rider asked for a
  second ago; dropping them lets the script's own 0.4 s window coast the motor,
  which is what a lost CAN frame did anyway. The count is on the link screen —
  if it climbs while riding, the link is not keeping up.
- The LISP script gains a reply-id sentinel: 255 means the request did not
  arrive over CAN, and the script answers on the interface it came in on. The
  CAN path is byte-identical. **An older script on the controller still works
  over CAN, but over Bluetooth it will not answer the drawer or the
  cruise/profile readout** — reflash `lisp/main.lisp` when switching.
- Two things Bluetooth cannot do: the head unit no longer shows up in VESC
  Tool's CAN scan (it is not on the bus), and a VESC Express holds one link at
  a time, so VESC Tool on the phone must disconnect first. Second-head
  temperatures, broadcast on CAN, are polled once a second instead.
- Not yet verified on hardware.

### Navigator picture from the phone

- The head unit can now show the companion app's navigator: the phone renders
  the map — it has the route, the graph and the tiles — and streams what to
  show as small JPEG frames over the existing Bluetooth link. Two new
  characteristics on the NotifBridge service (`...000B` control, `...000C`
  data) carry one frame at a time; the P4 decodes with its hardware JPEG
  engine, scales to 800x480 with the PPA and shows the picture through a
  single `lv_img`. The head unit draws no map of its own.
- New on-device setting, **Settings -> Phone screen**: what the 3-finger hold
  brings up from the dashboard, Android Auto (default) or Navigator. Only one
  of them can own the panel, so it is one choice rather than two switches. An
  Android Auto session is untouched in Navigator mode — its video is dropped
  the same way it already is while the dashboard is up.
- The phone is told when the navigator screen is and is not the live screen,
  and sends nothing while it is not — a parked bike or a rider looking at the
  dashboard costs no air time. Unchanged pictures are not re-sent either.
### The head unit draws the map itself

- Sending a whole picture for every few pixels of movement was always going
  to be a slideshow: a frame is 20 KB, the link carries 13-15 KB a second, and
  it stops entirely when the phone's screen goes dark. The head unit now keeps
  the map tiles the phone has already downloaded — each one crosses the link
  once — and the phone says only where the rider is, twelve bytes at a time.
  The map is redrawn on the head unit from memory, so it moves as often as the
  position arrives and keeps moving with the phone in a pocket.
- New messages on the same characteristics: TILE_BEGIN/TILE_END carry one
  tile (PNG or JPEG, the format travels with it), VIEW carries the rider's
  position, zoom and heading. The picture path is untouched and still works.
- Tiles are decoded on arrival and held as RGB565 in PSRAM, up to 96 of them
  (12 MB of the 25 MB free), least-recently-used first out. PNG is decoded by
  libpng, which the LVGL image decoder already brings in; JPEG goes through
  the hardware decoder.
- Measured on real tiles across three zooms: PNG as OpenStreetMap serves it
  averages 24 KB, JPEG q80 averages 18 KB. PNG is kept anyway — the phone
  forwards what it already holds without transcoding, and coloured labels stay
  crisp. A screenful is 15-20 tiles, so about 30 seconds on first arrival;
  keeping up at riding speed costs around 1 KB/s.
- Verified on a Guition JC4880 with a phone: a screenful of 30 tiles (the
  panel plus a ring) lands in about a minute at 13 KB/s, a PNG decodes in
  30-35 ms, and composing the whole 800x480 view from what is held takes
  17-19 ms — so the map redraws on every position update with room to spare.
- Four things the bench turned up, all fixed: the head unit's screen state
  could be announced before the app had subscribed, leaving it convinced the
  display was showing something else; tiles were chosen in square rings while
  the panel is a wide rectangle, so off-screen corners went over before the
  ground either side of the rider; an acknowledgement sent right after sixty
  chunk writes could be dropped by a congested host, and the phone then sent
  the whole tile again; and the bridge re-bound to whichever connection wrote
  last, which with two links from one phone flipped the owner several times a
  second. The binding is now sticky until its owner goes quiet for five
  seconds, and the app cancels a queued connection before asking for another
  so the second link stops happening in the first place.

### Picking where to go on the head unit

- Tap the navigator map on the head unit and it offers that spot as a
  destination; confirm and the phone routes there and starts guiding. The
  head unit composed the view, so it already knows which patch of ground
  every pixel is — the tap becomes a coordinate without asking the phone
  anything. It travels back as a nine-byte notification on the same
  characteristic the tiles use.
- Groundwork for entering an address there too: this is the reverse channel
  that was missing, and the search will use it.

### A map that never goes bare, and moves

- The rider's speed and battery charge now sit over the navigator map, in the
  same typeface Android Auto gets (Antonio), read from the same place, with
  the cruise indicator lighting up beside the speed when cruise is engaged.
- Two blurred fallback layers under the detail tiles, three and six zoom
  levels out. One tile of the widest is twenty kilometres across, so a couple
  of them blanket a whole region: move faster than 25 KB tiles can arrive —
  or jump somewhere nothing was cached — and the map goes chunky rather than
  empty. The phone sends them before anything sharp, and the ring around the
  panel last of all.
- The map now moves between position updates instead of stepping twice a
  second: the phone includes its speed, and the head unit carries the view
  forward at the last heading and redraws about seven times a second.
- Composing a frame costs 26 ms, down from 155 ms when the layers were first
  added. Three things got it there: blitting per tile rather than per screen
  pixel, writing one row of a scaled band and copying it down the rest, and
  drawing the blurred layers only in the gaps the detail tiles leave — with
  the map caught up, the coarse passes cost nothing at all.
- Tiles no longer wait on position updates while the map is filling: a
  screenful lands in about 8 seconds rather than 15, and the whole ring in 21
  rather than 41. A tile that is neither cached nor reachable is skipped
  within the pass instead of costing one.
- Both ends now ask the radio for the fastest link they can (a 251-byte
  link-layer packet and the 2M PHY). Both are accepted, and neither changed
  anything: 25 KB takes 49 writes and 780 ms, which is one ATT write per
  connection interval. The phone's stack sends one and waits, so the round
  trip is the limit and air time never was.

### The line, the turn, and a dot that stays put

- The route now travels to the head unit as a polyline (up to 2000 points,
  a tenth of a micro-degree each) and is drawn there under the rider — a blue
  line inside a white casing, the way a map draws a road, so it reads over
  both the sharp tiles and the blurred fallback.
- The manoeuvre is drawn as well: a plate in the top-left corner with the
  arrow for the turn ahead (eleven kinds, from a slight bend to a U-turn and
  the finish flag), how far it is in metres or kilometres, and underneath it
  what is left of the whole route as "3.2 km | 11 min". Off route, the plate
  turns red and says so. The arrows are drawn as a single polyline with the
  head retraced, because LVGL draws one line at a time and a separate
  arrowhead came out detached.
- The rider's dot no longer jumps. Dead reckoning between updates always
  drifts from where the phone says the rider is, and snapping back on every
  arrival was visible twice a second; the view now eases a quarter of the way
  towards the reported position on each redraw and only jumps when the two
  disagree by more than a couple of hundred metres. Frames are also spaced by
  the timer alone — a position arriving mid-frame used to trigger an extra
  redraw and the motion came in pairs.
- Drawing the line cost 40 ms of the frame at first: a Mercator projection is
  a logarithm and a tangent in double precision, this chip has no
  double-precision hardware, and every point was being projected twice.
  Points outside a screen-and-a-margin box are now discarded on latitude and
  longitude alone, the rest are projected once, and a segment that runs off
  the panel is clipped before it is walked pixel by pixel. `navstat` times the
  line on its own, which is how the numbers below are known rather than
  guessed: **composing the map is 18-26 ms and the line adds 5-15 ms** of
  scattered writes on top, a little more with a long route on screen. It is
  cache misses, not arithmetic — the same line drawn as bars, as squares or
  swept along rows costs much the same.
- When the head unit has to drop a tile to make room it now tells the phone,
  which sends that tile again next time it is needed. Without it an eviction
  left ground that stayed blurred for the rest of the ride. The store holds 64
  tiles (8 MB), down from 96 — a screenful plus its ring is 30, and the spare
  PSRAM is worth more than the extra history.

### No double precision left on the frame path

- This chip has hardware for single precision and emulates double in
  software, and the navigator kept learning that the hard way — 40 ms a frame
  for the route projection, 28 ms for the heading arrow. So the doubles are
  gone from everything that runs per frame or per point.
- Positions are now tenths of a micro-degree in `int32_t` — the unit the wire
  already uses, about eleven millimetres — all the way through the view, the
  dead reckoning, the destination taps and the search results. Integers also
  fix the accumulation: the easing towards each reported position is now an
  exact quarter of an exact difference, eight times a second, with no drift
  of its own.
- Tiles are addressed in `int64` arithmetic, exactly. The one logarithm left
  on the frame path is the view centre's own latitude, in single precision;
  everything else — the route's points, the tap that becomes a destination —
  is placed relative to that centre with two multiplies, which is both exact
  and free.
- `scripts/nav_proj_check.c` holds the formulas next to the double versions
  they replaced and prints the disagreement: **under a pixel for the view
  centre up to zoom 17 (1.8 px at 18, where a float can no longer hold 67
  million pixels), and exactly zero for everything placed relative to it.**
  It earned its keep immediately — the first run found a constant ten times
  too large, which would have put the latitude past a right angle and the
  whole map at NaN.
- Composing a frame is **26-27 ms** now, from 33-40 before.

### A bug hunt around the edges

Five things the probing turned up, all fixed:

- **A quiet moment could park the navigator for good.** The head unit sent its
  screen state only to the link holding the bridge binding. Switch the panel
  to the dashboard and back, and in that silence a second phone's
  notification took the binding — so the phone the rider was navigating with
  never heard that the screen was live again, and since it only writes when
  it believes the screen is live, it never wrote again either. State and
  "my tile store is empty" now go to every subscribed link, because they are
  facts about the panel rather than answers to anyone in particular; and the
  phone greets the head unit every five seconds while it is told the screen
  is not its own.
- **"Looking..." could hang for ever** when no answer came back. Seven seconds
  and the panel says "No answer from the phone".
- **A stale verdict stayed under a shrinking query**: deleting back to one or
  two letters left "Nothing found" on screen. It returns to the prompt.
- **Coordinates could not be typed at all.** LVGL's letters layer has a
  decimal point but no digits, and its symbol layer has digits but no point.
  A "123" button switches to the number pad, which has both — and a house
  number in a street search was equally impossible before. "50.06 19.93" on
  the panel now comes back as a place to go to.
- **The keyboard remembered the number pad.** Look up one pair of
  coordinates, and every search after it typed digits into a box expecting a
  street name — three characters never accumulated, so nothing was ever sent
  and the panel simply sat there. Found by a soak, which stopped getting
  answers after its first coordinate round. It opens on letters now.
- **A coordinate between -1 and 0 lost its minus** in the log and in
  `navstat`: the integer part of -0.5 is zero, and printing that as a number
  drops the sign. The sign is printed on its own now.

### Two phones, one panel

- A spare phone with the app installed is enough to break the navigator, and
  on the bench it did. It held both of the head unit's peripheral slots (its
  older app opened two links at once), kept the notification bridge bound by
  sending a steady trickle of notifications, and the phone the rider was
  actually navigating with could not write a single byte: its greeting came
  back refused, and the app — told nothing else — reported "Display firmware
  is too old" and switched the navigator off.
- Three fixes, one per link in that chain. A navigator write now takes the
  bridge binding on the spot rather than waiting for the owner to go quiet:
  whoever is drawing the panel outranks whoever is sending notifications. A
  refused greeting no longer means "no support" — the app keeps the channel
  and greets again every five seconds until the head unit answers. And the
  app cannot open two links any more, which is what filled both slots.
- The head unit also stopped trusting its own link count: it was kept by
  adding and subtracting on events, one missed disconnect left it
  permanently high, and since it decides whether to keep advertising, the
  head unit then quietly stopped being findable. It asks the stack now.

### Which way the rider is pointing — and why the map stays north-up

- The marker is an arrow along the course now, with a white casing, and falls
  back to the plain dot when the rider is standing still (a parked bike
  pointing somewhere definite is a lie).
- Heading-up was measured before being decided against. The panel has no
  hardware for an arbitrary-angle rotation — the PPA turns in 90-degree steps
  — so track-up means warping all 384000 pixels between two PSRAM buffers
  every frame. A new bench command, `navwarp [degrees] [block]`, does exactly
  that and times it: **38 ms at 30 degrees and 118 ms at 90**, against a frame
  that composes in 33-40 ms and an LVGL flush that already costs 57 ms on the
  same core. So the map stays north-up and the marker carries the heading.
- The arrow itself cost 28 ms a frame at first, which is the same lesson as
  the route line: its three edge tests per pixel were in double, and this chip
  has no double-precision hardware. Vertices in double (three a frame), fill
  in integers — and `navstat` now splits the frame into composing and the
  cache flush, which is how the 28 ms was found rather than guessed at.

### Typing an address on the head unit

- A **FIND** button on the navigator screen opens a keyboard on the panel.
  Type a street, a place or a pair of coordinates; the phone looks it up in
  the offline index that came with its map — so this works with no signal at
  all — and sends back up to six answers, nearest to the rider first, each
  with how far away it is. Tap one and the phone routes there, the same path a
  tap on the map already took.
- Two new messages: `0x16 SEARCH` carries the typed text up (the only
  head-to-phone message with text in it), and `0x0B FOUND_BEGIN` carries the
  answers down — count, byte length, then one entry per result. A count of
  zero is a valid answer and the panel says "Nothing found".
- Names are cut to what the panel can hold without splitting a UTF-8
  character in half — both halves of that matter, a dangling continuation
  byte and a lead byte whose tail was cut are equally broken, and either draws
  as a box.
- The keyboard gets Montserrat rather than our subsetted font: its backspace,
  enter and hide keys are FontAwesome glyphs, and without them the bottom row
  was four empty boxes (the same trap as the zoom buttons).

### A head unit that rebooted got no tiles at all

- Found while testing the search: reboot the head unit with its navigator
  screen up, and the phone reconnected, sent positions — and not one tile,
  for ever. Its visibility never changed, and the only signal that a store is
  gone was a visibility change.
- The head unit now says so itself: alongside STATE it sends `0x17 EMPTY`
  whenever its tile store is empty, and the phone forgets everything it
  thinks it has sent. Verified: `reboot` on the panel, hands off the phone,
  39 tiles and a full screen within half a minute.

### Telling the head unit a ride is over

- Alexey, watching the panel: "а куда делась синяя линия". The line had gone
  off screen and the plate above it read "0.0 km | 0 min" in arrival red —
  because the phone had no way to say that the ride was finished. It sends
  a route when there is one and nothing when there is not, so the head unit
  kept drawing the last one.
- A route of zero points now means "forget it": the line and the manoeuvre
  plate come off, and the panel is a plain map again. The phone sends it
  whenever the route it was drawing goes away — arrival, a cleared route, a
  cancelled ride.

### Zoom, on both screens

- Two buttons on the head unit's map, `+` and `-`, from z14 to z18, with the
  level shown between them. The rider is the one looking at that screen, so
  the panel changes level immediately and the phone is told afterwards — it is
  the only end that can fetch tiles. A new notice on the same characteristic
  (`0x15 ZOOM`) carries the level; the phone switches everything it sends,
  detail tiles, fallback layers and position, to it.
- Nothing goes bare in between. The composer now also tries the level either
  side of the one it is drawing — one out gets doubled, one in gets halved —
  so the two seconds before new tiles arrive show a coarser or softer map
  instead of an empty slate. Verified on hardware: z17 to z18 drew from the
  tiles already held, and z16 filled the whole panel from 3 real tiles plus
  halved z17 ground.
- The phone's own map gets the same pair of buttons, above the follow control.
  Pinch still works; two thumb-sized buttons are what works one-handed.
- The translucent plates behind the readouts sit a little taller: Antonio's
  digits reach the top of their line box, and a two-pixel pad made the grey
  look cut off.

### Two wires that were never connected

- The head unit's eviction notices and its destination taps both stopped at
  the phone's background isolate: the subscriptions were declared, cancelled
  on disconnect — and never actually made. The eviction one is what made a
  long ride go blank. The head unit holds 64 tiles, a ride outgrows that in a
  few kilometres, and the feed never sends the same tile twice; with the
  notices lost, the panel dropped ground it still needed and the phone
  believed it had already been sent. The symptom was a map that stopped
  filling while the position kept updating — `tiles 0/12` in `navstat` with 64
  tiles in the store. Both are wired now, and a 3 km simulated ride holds
  12/12 with 154 evictions behind it.
- A tile the phone cannot get is no longer written off for the session. Two
  failures used to retire it permanently, which is fine for a tile the head
  unit refuses and wrong for the far more common case — not cached, no signal
  yet. It now waits thirty seconds and is asked for again.

### The mirror in the app is gone

- The navigator screen no longer carries a thumbnail of a second, smaller map
  rendered for the head unit. The head unit draws its own map from the tiles,
  so the picture path had nothing left to do: the frame streamer, the JPEG
  encoder and that second map are all deleted from the phone. What is left in
  its place is a one-line badge, and only when something is actually wrong —
  a firmware too old to speak the tile protocol, or a display showing another
  screen.
- That also takes out one cause of repainting: the screen rebuilt its whole
  widget tree on every feed status change — once per tile sent — the
  thumbnail's own map included. Nothing subscribes to the feed's counters any
  more.
- And the blink itself, which turned out to be something else: while a region
  downloaded, the map asked its tile layer to reload every hundred tiles. A
  reload drops and re-creates *every* tile on screen, so for one frame the map
  was bare — caught on a screen recording as a single frame at luminance 16
  against 160 either side, several times per download. Nothing resets the
  layer now; instead a tile that fails retries itself three times over
  twenty seconds, re-reading the cache each time in case the bulk download has
  landed it. Same recording after the change: 250 tiles downloaded, no frame
  below 145.
- The head unit still accepts pictures (`FRAME_*` and `navtest` are
  untouched), so the path is retired on the phone rather than removed from
  the protocol.

### After a head-unit reboot the phone waited for ever

- A head unit reboots on every firmware flash and every power cycle of the
  bike. It came back advertising and the phone did not reconnect — its log
  showed no connection attempt at all, for minutes, until the app was
  force-stopped. The code trusted Android's own autoConnect queue to survive
  a remote disconnect; it does not always, and certainly not after we cancel
  it ourselves to avoid a second link.
- A watchdog now re-asks for the link once it has been missing for two
  twenty-second ticks. Two, not one, and not on the disconnect event itself:
  with autoConnect that event fires as a matter of course, and re-arming on
  each one tore down a healthy link every few seconds — tiles slowed to one
  per twenty seconds and flutter_blue_plus began logging its two-second
  disconnect gap. Measured on the bench: `reboot` on the head unit, hands off
  the phone, connected again eight seconds after it finished booting with the
  tile feed running; 60 tiles in the following half minute and no link
  cycling at all.
- The tile store is back to 96 tiles (12 MB). Sixty-four looked like enough
  and was not: at zoom 18 a screenful plus its ring covers four times the
  ground, and a 1.6 km ride evicted 890 tiles — every eviction is reported to
  the phone, and a tile still wanted comes straight back, so the link spent
  the ride re-sending ground it had already sent.

### Reboots under a phone: the Bluetooth host ran out of stack

- The head unit restarted every few minutes with the navigator up. The panic
  text — captured by leaving a logger on the console rather than guessing —
  named the `nimble_host` task and a stack-protection fault, which is the
  hardware stack guard rather than a corrupted heap.
- Cause: every GATT write on the NotifBridge service is parsed on that task,
  and each branch of the callback declared its own flatten buffer — a
  notification chunk, an OTA chunk, a file chunk, a tile chunk. The compiler
  laid them out side by side, so about 1.5 KB of the task's 4 KB stack went on
  buffers that are never live at the same time. Adding the tile channel was
  what pushed a notification arriving mid-stream over the edge.
  All four branches now share one static buffer (the task is single and each
  callee copies what it keeps), and the task itself gets 6 KB. `navstat`
  reports its worst-case margin so the next squeeze is visible before it is a
  reboot.

- Debug bridge: `uimode [vesc|aa|nav|toggle]` reaches every full-screen mode
  without the 3-finger hold, `navstat` reports the frame and tile streams (mode, tiles
  accepted and rejected, decode and compose times, and which tiles the view
  is still missing), and `navtest` puts
  a locally-made frame through the decode-and-scale path so the picture chain
  can be checked without a phone. The bridge's console now follows the board:
  on one whose console is the USB-Serial-JTAG port (the Guition JC4880 brings
  out no UART0 header) the REPL binds there instead of UART0.
- Verified end to end on a Guition JC4880 and a phone: frames arrive within
  a second of the navigator screen coming up, decode and scale to the full
  panel in 7-9 ms, and land at 10-26 KB each. Panning the map continuously
  moves about 0.6 frames a second; a map that is not moving sends nothing at
  all. Switching the head unit to the dashboard stops the phone (it captions
  the preview "Display is on another screen"), switching back resumes within
  a second, and a head-unit reboot reconnects on its own.

## v1.3.17 / app 0.3.17 — 2026-09-09

### Waveshare microphone actually works

- 1.3.16 shipped the Android Auto microphone dead on the Waveshare 4.3: the
  two MEMS mics sit on ES7210 inputs MIC1 and MIC3 (MIC2 is the echo-cancel
  reference fed from the speaker output, MIC4 is unconnected), while the code
  read MIC1/MIC2 as a plain I2S stereo pair — a mode in which MIC3 never
  reaches the ESP32-P4 at all. Found by tapping the mics with all four inputs
  captured at once; only ADC1 and ADC3 moved. The ADC is now driven the way
  Waveshare's own demo does it (three inputs selected, which puts the chip in
  TDM so all four channels arrive on one line), the two mics are averaged
  into the mono stream, and the input gain went from 24 to 30 dB. Guition
  JC4880 (ES8311 ADC) is unchanged and worked already.

### Touch

- Android Auto ignores contacts shorter than 250 ms: a press is reported to
  the phone only once the finger has stayed down that long (at the touch-down
  point, then caught up to where the finger is), and a shorter contact sends
  nothing. Stops vibration, knuckles and raindrops from tapping things;
  fast flicks under the threshold are lost — deliberate.
- Groundwork for a smaller Android Auto viewport inside the 800×480 frame
  (video margins): touch is reported relative to the phone's content area and
  the touch-screen descriptor advertises that area. Margins ship at 0×0, so
  nothing changes on screen.

## v1.3.16 / app 0.3.16 — 2026-09-07

### Microphone to Android Auto

- The head unit's own microphone now feeds the Android Auto voice input. The
  phone has been asking for it all along (AVInputOpenRequest on the mic
  channel, previously logged as "not handled") — the request is now answered
  and, while a voice session is open, the on-board mic is streamed as
  16 kHz / 16-bit / mono PCM in 40 ms chunks. Gives Google Assistant voice
  input through the unit; the spoken reply still plays on the phone because
  the Speech audio channel is not advertised. Phone calls are unaffected
  (they ride Bluetooth HFP, which the BT agent does not do).
- Waveshare 4.3: MIC1 of the two on-board MEMS mics via the ES7210 ADC.
  Guition JC4880: mic on the ES8311's own ADC, compile-only. Kconfig
  `AA_MIC_ENABLE` (default on) removes the whole path — the phone then gets
  an "open failed" answer instead of silence.

## v1.3.15 / app 0.3.15 — 2026-09-06

Curated Android Auto branch: only the fixes and the changes that showed a
clear effect, without the experimental in-tree decoder. Not the 1.3.12-1.3.14
line (that stays on the video-perf branch).

### Android Auto reconnect and the Connect button

- A dropped session no longer strands the head unit. The socket gained TCP
  keepalive and receive/send timeouts, so a phone that leaves the AP silently
  (out of range, pocket) is noticed within ~15 s instead of hanging the session
  forever. A clean goodbye (ByeBye) is told apart from a lost link, and the
  phone is given a 3 s grace to restart projection on its own before the head
  unit kicks it off the AP.
- The BT agent leaves the air for the whole session (agent 0.6.4 -> 0.6.6),
  the way the reference dongles power their radio off, so a stray Bluetooth
  event can no longer make the phone restart projection mid-ride.
- **Connect now works.** Two dead paths fixed in agent 0.6.7: the phone it
  pages is the one that last ran Android Auto (not whichever paired last), and
  a Connect tap with the link half-up but no session tears it down so the phone
  re-runs the wireless setup instead of doing nothing.
- Agent updates no longer wipe the agent's own pairing. The OTA used to erase
  the agent's NVS on every write, so after each update the phone had to be
  re-paired; it now skips the NVS region.

### Screen

- The Android Auto idle screen shows the link state — Disconnected /
  Connecting... / Connected — in colour, with the step in progress as the
  subtitle and the IP / port on a dim third line.
- The backlight goes dark during flash erases and bulk writes instead of
  letting them tear the panel blue, and comes back a moment after the last one.
- The LVGL worker no longer busy-spins on core 0; it was starving Bluetooth and
  the idle task badly enough to trip the watchdog.
- A touch is ignored for 1 s after a dashboard <-> Android Auto switch so a
  stray finger doesn't land on the wrong screen.
- Boot-time "battery charged — reset trip?" prompt: the dashboard asks before
  resetting the trip instead of doing it silently.

## v1.3.11 / app 0.3.11 — 2026-09-02

### Faster firmware updates over Bluetooth

Why a BLE update took 10+ minutes: a 4.4 MB image went out as ~18 000
writes of 244 bytes, the phone stack issues them one per ATT round trip, and
four things kept each round trip slow:

- **Connection interval.** Nothing asked for a fast link, so the transfer ran
  at Android's default ~45 ms interval. The LISP editor already requests
  high priority (7.5–15 ms) for its transfers; the OTA path did not. Now the
  app requests it for the duration of the update, and the firmware asks from
  its side too on BEGIN (`ble_gap_update_params`, 11.25–15 ms — Android
  refuses anything shorter), reverting on failure.
- **Sensor initiator scan.** With a PAS or wheel-speed sensor bound but asleep
  (the usual state of a parked bike), the head unit keeps a connect attempt
  pending — and NimBLE's default connect parameters scan for the peer at
  100 % duty, sharing the radio with the phone link the whole time. The
  arbiter now scans 30 ms in every 100 ms (a sensor advertising at ~1 Hz is
  still caught within seconds of waking), and the OTA parks the arbiter
  entirely while receiving.
- **Chunk size.** The firmware's flatten buffer capped DATA writes at 244 B
  although both sides negotiate MTU 512. The buffer now takes 509 B and READY
  advertises that cap in its `detail` field; the app uses the largest chunk
  the cap and the MTU allow — half the writes. Older firmware sends 0 and the
  app keeps 244.
- **Ack fallback.** On Android's BUSY (TX credits exhausted) the app retried
  after 15 and 30 ms and then switched that chunk to an *acknowledged* write
  — one chunk per connection event. Under load that path swallowed most of
  the image. Retries now wait roughly a connection interval and stay
  unacknowledged for six attempts before falling back.
- Also: progress IPC to the UI isolate is throttled (was one message per
  chunk).

Note: the update *to* 1.3.11 still runs on the old firmware's side of the
protocol (244-byte chunks, no peripheral-side interval request, scan still
running) — only the app-side improvements apply to it. Updates *from* 1.3.11
onward get the full effect. Not hardware-measured yet.

## v1.3.10 / app 0.3.10 — 2026-09-02

### Android Auto no longer drops on a big video frame

- Field log (1.3.9, five minutes into a session): `aa_tls: decrypt: out_buf
  full (1564)` → `recv_decrypted: ESP_ERR_NO_MEM` → `tcp: client closed` —
  the panel fell back to the dashboard and the phone had to reconnect. A
  fragmented AA message (a video I-frame) came in larger than the fixed
  96 KiB reassembly buffer. The same cap had already been raised once, from
  32 KiB, for the post-BT-handover key-frame burst.
- `recv_decrypted` now reassembles into per-channel buffers that grow on
  demand (doubling from 96 KiB, hard ceiling 2 MiB — an 800×480 I-frame is
  ~100–300 KiB) and hands the message to the dispatcher by pointer instead
  of copying it out, which also removes a ~100 KiB memcpy per video frame.
  Growth steps are logged once (`ch 3 reassembly buffer 98304 -> 196608
  bytes`) so real frame sizes show up in the log.

### Trip statistics switch removed again

- The opt-in switch introduced in 1.3.9 is gone; statistics are always on,
  as before 1.3.9 (the NVS key it left behind is ignored). What stays from
  1.3.9 is the actual fix: the trip log's sector runway is erased at boot,
  before the display comes up, instead of at every stop longer than ~30 s.
- On the log's own device the runway was already clean (`runway: 64 clean
  sectors ahead`) — so the stop-time erases were NOT what that unit was
  freezing on; the 10 s record writes remain the only flash activity during
  a ride.

## v1.3.9 / app 0.3.9 — 2026-09-02

### Trip statistics are now opt-in (Settings → Trip statistics, default OFF)

- New switch **Settings → Trip statistics**. Off (the default): the dashboard's
  STATISTICS entry point is hidden on every theme and the trip log does
  nothing at all — no boot scan, no 10 s record writes, no sector erases, no
  flash I/O. On: behaves as before; toggling takes effect without a reboot
  (off stops sampling and erases at once, on scans the ring and starts a new
  trip). The Settings "Reset" button returns it to off.
- Why: every flash write or erase on this board suspends the cache for both
  cores AND stalls the DSI DMA that feeds the panel from PSRAM (AUTO_SUSPEND is
  unavailable on this GD25Q256, see `sdkconfig.defaults`), so the trip log was
  the one subsystem that touched flash *during a ride* — a 64-byte record
  every 10 s, plus 4 KB sector erases (45–400 ms each = a frozen screen with a
  blue flash) whenever the runway of pre-erased sectors ahead of the log's
  head had to be rebuilt. It is the prime suspect for the "dashboard freezes
  for a moment, often" reports. With the switch off a ride performs zero flash
  writes; if the freezes persist with it off, the cause is elsewhere (CAN data
  dropouts, render path) — the switch doubles as the experiment.
- Consequence of "off": the dashboard TRIP / Ah / uptime totals no longer
  survive a head-unit reboot (the log was their only persistence) and start
  from zero at every power-on; the smart-battery tracker falls back to its
  10-minute NVS backup. The odometer is unaffected (it comes from the VESC, or
  from the BLE speed sensor's own NVS counter).
- Firmware side (`components/trip_log`): the old boot scan never *cleaned*
  anything — at start it only read one record per sector to find the head and
  erased 2 sectors ahead; old trips were left for the ring to overwrite, and
  the erases of that stale data ran from the writer's idle loop, i.e. at every
  stop longer than ~30 s (one 45–400 ms freeze every 3 s until 64 sectors were
  clean), or just-in-time mid-ride once the runway ran out. Now, when the
  feature is on, the runway is rebuilt synchronously in `trip_log_init()`
  BEFORE `display_init()` — the panel is still dark, so a dirty sector costs
  boot time (typically a few sectors ≈ 0.1–0.3 s; capped at 3 s, the rest is
  left to the idle trickle) instead of a mid-ride freeze. The idle trickle and
  the just-in-time erase remain as fallbacks only.
- Not hardware-verified: the freeze diagnosis itself. Compile-tested on
  Waveshare + the desktop simulator.

## v1.3.8 / app 0.3.8 — 2026-08-19

### BLE wheel-speed sensor (stock Coospo-class CSC sensors)

- The head unit can now take its speed from an off-the-shelf BLE wheel-speed
  sensor (Coospo, Magene, ... — anything speaking the standard Cycling Speed
  and Cadence profile, service `0x1816` / CSC Measurement `0x2A5B`). New
  files: `main/ble_speed_client.c` (central client + wrap/reset-safe CSC
  parser), `main/speed_sensor.c` (source setting, rev→km/h conversion, local
  trip/odometer integrators + their NVS persistence in namespace `spdsns`),
  `custom/speed_screen.c` (settings screen).
- **Settings → Speed sensor → Open**: pick the speed source (VESC vs BLE
  sensor — an explicit switch, no auto-fallback), pair/forget the sensor
  (same address-binding flow as the PAS cadence sensor), set the wheel
  diameter (the shared `wheel_mm` setting, now with a debounced NVS commit so
  +/- taps don't stall LVGL), and watch live speed / trip / odometer /
  sensor battery.
- With the BLE source selected, dashboard **speed, trip and odometer** come
  from wheel revolutions × circumference — they keep updating even when the
  VESC is silent (the "ESC NOT CONNECTED" banner still reflects the VESC).
  The AA video HUD speed and the trip-log speed samples follow the same
  source; the trip-log idle detector (which gates flash-erase trickling)
  now requires BOTH sources to look idle before erasing. The local odometer
  is stored in NVS (saved on ride stops + a 10 min fallback, on a dedicated
  writer task) and is never resettable; "Reset trip" zeroes the BLE trip
  alongside the VESC one.
- Both BLE sensors (cadence + speed) can be bound at once: a new
  `main/ble_central_arb.c` shares NimBLE's single connect-initiator between
  them (one bound sensor behaves exactly as before; with two, connect
  attempts rotate on 8 s windows). `CONFIG_BT_NIMBLE_MAX_CONNECTIONS` is now
  4 — delete the stale per-board `build_*/sdkconfig` before rebuilding.
- NOT yet hardware-verified: CSC parsing against a real Coospo, dual-sensor
  coexistence, and the BLE-source trip/odometer accuracy.

## v1.3.6 / app 0.3.6 — 2026-08-05

### One navigation strip across the on-device web UI

- The three built-in pages — `/ota` (firmware update), `/files` (file manager)
  and `/lisp` (LISP editor) — now share an identical tab strip, so they read as
  one interface instead of three unrelated pages. Markup and CSS live in
  `main/web_nav.h`; the current tab is marked by appending `on` to its class.
- No JavaScript: every tab is a plain link to a sibling page, which costs
  nothing and survives a reload.
- The LISP editor is a standalone gzipped `.html` and cannot include a C
  header, so it carries its own copy of the strip — the header says so, and the
  two have to be kept in step by hand.

### Line-by-line Russian walkthrough of the LISP script

- `lisp/main.ru.lisp`: the same code as `main.lisp` with an explanation above
  every line. Documentation only — **`main.lisp` is what gets flashed**, and
  this copy has to be updated by hand when the real script changes.

## v1.3.5 / app 0.3.5 — 2026-08-05

### The display no longer dies on the CAN bus (and neither does the helper)

- `comm_can.c` never handled **TWAI bus-off**. Past 255 transmit errors the
  controller stops, and it does not come back on its own: recovery has to be
  requested, and after 128 sequences of 11 recessive bits the driver waits in
  STOPPED for an explicit start. Without those two calls a node that hit bus-off
  was simply gone from the bus until its power was cycled — every send returned
  silently, with nothing in the log.
- Found while chasing dead pedal assist: the **C3 BLE helper** lost PAS, the
  throttle toggle and its state queries all at the same instant, and only a
  power cycle brought them back. Same bug, same file — the helper's fix ships as
  its `v1.0.8`.
- `can_health_check()` now runs every 500 ms from the receive task (which
  already wakes on a 10 ms timeout, so no new task): bus-off → recovery,
  STOPPED → restart, both logged.

### Why the bus gets there

A trace of the wire counted **35 615 bus errors** with our own
`tx_error_counter` pinned at 128, on a 1 Mbit/s bus with three nodes. Recovery
removes the permanent death, not the errors. If the recovery counter climbs in
normal use, the bus itself wants attention — 500 kbit/s is the VESC default and
has twice the timing margin.

### CAN polling hold-off

- New `CONFIG_VESC_CAN_POLL_START_DELAY_MS`, default **4000**: the display stays
  off the bus for the first four seconds, while the other nodes and the VESC's
  LISP script come up. Cheap insurance, not the fix — note that it delays only
  the *polling*; the TWAI controller itself joins the bus at `comm_can_start()`
  and participates in ACK and error signalling from that moment.

### Bundled helper firmware

- Now **1.0.8** (was 1.0.6 in the 0.3.4 APK, which predated the helper release).
  `stage_firmware_asset.sh` pulls it from the helper repo's latest GitHub
  release at build time, so cutting this release after publishing the helper's
  is what keeps the two in step.

## v1.3.4 / app 0.3.4 — 2026-08-03

No firmware or app code changed in this repository — this release exists to ship
the enclosure models and to cut a numbered build of the current tree.

### Enclosure models for the Guition board

- `3d-model/esp32-p4-wifi6-guiton-800x480/` — main body, insert, u-holder and a
  TPU cover, plus the STEP source. The Waveshare models move into
  `3d-model/esp32-p4-wifi6-waveshare/`, so the directory now names its board
  instead of implying there is only one.

### Related: the PAS chain fix landed in the helper, not here

- The pedal-assist regression traced to the **C3 BLE helper** (separate repo,
  firmware `v1.0.7`): its single boot-time `REQ_STATE` missed while the VESC was
  still loading `main.lisp`, and with periodic polling off by design the miss was
  permanent — hence "no data from vesc" in the app, and a throttle toggle that
  flipped `throttle-on` from a guess, which coasts the motor arbiter
  (`lisp/main.lisp:451`) with a healthy cadence sensor.
- Nothing on the P4 side was involved, so this app build carries the same P4
  firmware as 1.3.3. The helper image is fetched from the helper repo's GitHub
  releases at update time, not bundled here.

## v1.3.3 / app 0.3.3 — 2026-08-03

### New: three concept dashboard themes

- **Classic Max**, **Lamborghini** and **Supermoto**, plus a reference screen,
  generated by the new `tools/build_classic_max.py` and
  `tools/build_concept_dashboards.py`.
- The amber dashboard is gone, and with it the DSEG7 / DSEG14 segment fonts: no
  screen references them any more, so they are neither compiled from
  `generated/guider_fonts` nor re-subset into the build. The `.ttf` files stay
  in `import/font/` and the CMake comment says what to re-add to bring them
  back. Antonio gains a size-100 subset.
- The simulator's `dashboard_themes_auto.c` is generated from `gui_guider.h` by
  `scripts/gen_dashboard_themes.py`, so it is now gitignored as a build artifact.

### The AI assistant can no longer ship a dead panel control

- Asked to add on-screen profile switching, the assistant emitted
  `(pu8 1) (pu8 8) (pstr "Profile")` — `<id>` and `<type>` swapped, so the type
  byte read 8. An unknown type has no known tail length, so the P4 stops
  decoding and throws away the **rest of the frame**: the row silently never
  rendered, with nothing but an `ESP_LOGW` on the head unit's own console to
  show for it. The protocol lived only in `lisp/README.md`, which the model is
  never shown, and the linter knew nothing about it — so the invariant was
  enforced nowhere.
- The prompt now carries the panel as a fails-silently section (id-then-type,
  the four control types, the count byte in both senders, one id across all
  three functions, the 16-control cap, and that a panel change is three edits),
  the full byte layout, copy-paste templates for all four types, and the
  radio-group pattern — there is no 1-of-N control, so a profile selector is N
  toggles over one variable.
- The linter gates flashing on nine new checks: unknown control type, a count
  byte that disagrees with the rows listed, duplicate ids, ids that only
  `panel-send-state` or `panel-action` know about, over-long labels and unit
  suffixes (a label ≥ 40 bytes leaves the P4 decoder stopped mid-string and
  everything after it is misread), and a frame wider than its `bufcreate`.
- False positives were the design constraint — a gate that cries wolf gets
  switched off, taking the real checks with it. An unrecognised shape produces
  **no** diagnostic: broken parens skip the pass, every emit in a frame must
  share one parent (so rows emitted inside an `if` or a loop are left alone), a
  foreign call between emits abandons the frame (helper emitters), and
  non-literal arguments disable only the rule that needed the value.
- Also written down for the model: why the arbiter ticks at 100 Hz (at 20 Hz the
  ramp advanced in 12.5 %-of-max steps that FOC executes instantly — the rider
  felt jerks) and why `app-disable-output` is refreshed every tick rather than
  once (if the script dies the stock throttle returns ~1.5 s later and the bike
  stays rideable).

### Pedal assist: one sender at a time

- Two nodes may stream PAS setpoints, and the head unit's own PAS idles at 0 A,
  20 Hz, forever. Interleaved with a real assist current those zeros chopped the
  setpoint into 3 A → 0 → 3 A and the motor jerked. The script now locks onto
  whoever sent the last non-zero setpoint and ignores the others until that
  source goes silent or sends zero; a 0.4 s staleness check releases the lock
  too.
- The BLE helper can toggle the throttle master switch atomically (`msg 0x06`,
  it never needs to know the current state), and its buttons arrive as plain
  standard-id CAN frames on `0x123` mapped to throttle-toggle and
  profile-switch.

## v1.3.2 / app 0.3.2 — 2026-08-02

### New: LISP editor in the browser

- `http://android-auto.local/lisp` — a full LispBM editor on the head unit's
  own HTTP server, next to `/ota` and `/files`. Syntax highlighting, line
  numbers, matching and rainbow parens, find/replace, block indent, hotkeys.
- The linter encodes the failures that cost the most time here: paren balance,
  unterminated strings, `@const-start` / `@const-end` pairing, defuns left
  outside the const block, buffers created inside it, and a thread started
  before the function it runs is defined (the handler dies with
  `variable_not_bound` and the feature is simply gone).
- Read VESC / Upload / Upload + Run / Start / Stop over CAN with a progress
  bar. The transfer is asynchronous — an upload takes tens of seconds and the
  server serves everything from one task, so blocking it would freeze `/files`
  and `/ota` too.
- Live console of the script's `(print ...)` output. The firmware never parsed
  `COMM_LISP_PRINT` / `COMM_PRINT` before; the packets were arriving all along.
- A REPL line evaluates an expression without reflashing the script (capped at
  240 bytes — one CAN buffer transfer).
- The script library on the device (`/vescfs/lisp` and microSD) is browsable
  from the same page: open, save, rename, **move**, delete, mkdir, upload.
- `scripts/lisp_web_mock.py` serves the page against a fake device, so the
  editor can be worked on without a board.

### CAN identity

- The head unit answers `COMM_FW_VERSION` on the bus, so VESC Tool's CAN scan
  lists it as **Super VESC Display** instead of "Unknown". It replies to that
  one request only — it is not pretending to be a motor controller (the scan
  icon comes from `HW_TYPE_CUSTOM_MODULE`).

### Traction control in the LISP arbiter

- The native VESC traction-control algorithm, ported into the arbiter as a
  0..1 current scale. It cannot be left to the ADC app: that app's control
  block sits behind `app_is_output_disabled()`, which the arbiter keeps
  asserted, so the app's own TC never runs.
- On/off and the threshold are the real appconf fields `adc-tc` /
  `adc-tc-max-diff` — the panel toggle, the on-device VESC Tool menu and VESC
  Tool itself are one setting. `tc-peer` (the other motor's CAN id) must be set
  by hand; 255 disables the limiter, which is the safe default.
- Profiles are now also selectable directly from the on-screen panel
  (`panel-set-profile`), a radio group of three. The tune/melody controls are
  gone, and with them the ~2 kB quoted literal that cost cons heap at load.

### Reference and docs

- `conf-get`'s second argument is a "read the firmware defaults" **flag**, not a
  fallback value, and `conf-store` writes both mcconf and appconf from live RAM
  — in a script that scales limits at runtime that bakes temporary values over
  the rider's master config. Both are now documented in the assistant's LISP
  reference and in `lisp/README.md`.

## v1.3.1 / app 0.3.1 — 2026-07-31

30 commits, 130 files, +27 929 / −8 284 since v1.2.35.

### New: AI assistant for LISP

- An "Assistant" tab in the LISP editor. It reads the script off the VESC,
  edits it with anchored search/replace patches, runs the linter, flashes only
  on an explicit tap, and then **verifies on hardware** — doneCtx, heap trend,
  sentinel globals, print output. The protocol acks only prove the bytes
  reached flash; they succeed even when the script is dead, so they are not
  trusted.
- Every flash and every script start needs a tap; stopping a script never does.
  Provider is OpenRouter (or DeepSeek directly), the key is yours and lives in
  the system keystore.
- A VESC LispBM reference ships in the prompt — motor commands, config, CAN,
  buffers, events — taken from the upstream bldc docs, plus this project's own
  rules and worked examples.

### New: ESP32-C3 BLE helper configurator

- Status, parameters, pairing and firmware screens: cadence gauge, assist
  current and levels, binding of buttons and the cadence sensor (the helper
  itself does the scanning), per-button CAN commands, PAS tuning.
- Runs as a second GATT link from the background isolate, so the head-unit
  connection stays up while you configure the helper.
- The helper's firmware is pulled from its GitHub releases at build time and
  bundled into the APK, so it can be flashed with no network.

### VESC console and LISP linter

- Script `(print ...)` output is finally visible on the phone. The bytes always
  arrived — the head unit's bridge forwards every packet — but the link layer
  dropped anything that matched no pending request.
- The linter encodes the rules that fail SILENTLY on hardware: one `@const`
  block, mutable `def`s and `bufcreate` above it, and no forward references
  from top-level statements (not "spawns last" — `main.lisp` legitimately
  spawns from the middle of its const block).
- Syntax highlighting in the editor and in the assistant's code blocks.

### LISP and motor control

- LISP editor in the app over any NUS link: the head unit's bridge, a
  stand-alone VESC BLE adapter, or the helper.
- Upload format now matches VESC Tool's CodeLoader. WRITE offsets include the
  8-byte flash header the VESC validates at startup, so raw uploads flashed
  fine and then never loaded. Limit raised 16K → 120K, pollers pause during a
  transfer, faster BLE reads.
- The motor is owned by a single current-based arbiter in LISP (100 Hz):
  smooth ramps, brake slew, no step to zero. The stock ADC app stays configured
  but its output is suppressed by a rolling `app-disable-output` — if the
  script ever dies the motor stops on the command timeout and the stock
  throttle comes back in ~1.5 s.
- Tuning moved out of the quick panel: ramp times are read live from VESC Tool,
  cruise PI gains live in the script.

### Pedal assist (PAS)

- Entirely on the head unit: a BLE central link to the cadence sensor, the PAS
  loop, and a current setpoint handed to the arbiter on the VESC. The phone is
  not involved.
- Quiet on CAN unless actually assisting. PAS_SET frames used to stream at
  ~20 Hz forever, even with no sensor connected.

### Display and settings

- 180° screen rotation for upside-down mounting, done at render level:
  panel-level mirroring on the ST7701 produces stripes because the DPI scan
  order is fixed — the data has to be flipped, not the panel.
- Hold-to-repeat on the +/− step buttons in settings.
- "Display CAN ID" row — the head unit's own CAN node id. The backend existed;
  its UI had been left commented out.
- throttle-curve range corrected to −100..100%. An audit of 985 parameters
  across three firmware versions found this to be the only display-range
  divergence.

### BLE and stability

- Advertising continues while a peer is connected, so VESC Tool can find the
  head unit alongside the phone app. The third NimBLE slot stays reserved for
  the cadence sensor.
- The NUS→CAN bridge targets the VESC id from settings instead of a
  compile-time default. Anyone whose VESC sits on a different CAN id had a
  working dashboard but VESC Tool over BLE timing out against a node that does
  not exist.
- No more screen stalls while riding: the battery tracker's NVS commits moved
  off the LVGL thread and the trip log is trickled out over time.
- The app's whole BLE stack moved into the foreground-service isolate;
  reconnect hardened.

### Tooling

- `scripts/build_app.sh` builds the APK with **no embedded API key** by
  default; `--with-key` masks the key from `.env` into the build.
- `scripts/release.sh` aborts if a key ends up in the APK anyway. The check
  reads the decompressed zip entries — grepping the `.apk` itself always
  reports "clean" even when the key is sitting in `libapp.so`.
- `scripts/build_board.sh` uses each build directory's own Python venv, so both
  boards build in one run. Releases used to stop at the first board.

## Earlier releases

Shipped between v1.2.35 and v1.3.1; cut as release commits but not tagged.

| Version | Date | Summary |
|---|---|---|
| 1.2.44 / 0.2.44 | 2026-07-27 | BLE bridge targets the configured VESC id |
| 1.2.43 / 0.2.43 | 2026-07-27 | Display CAN ID in settings |
| 1.2.42 / 0.2.42 | 2026-07-21 | PAS merged to main; CAN-quiet PAS; VESC Tool alongside the app |
| 1.2.41 / 0.2.41 | 2026-07-04 | Deferred battery NVS, trickled trip log — no flash stalls while riding |
| 1.2.40 / 0.2.40 | 2026-07-06 | Hold-to-repeat on the +/− step buttons |
| 1.2.39 / 0.2.39 | 2026-07-05 | 180° flip fixed at render level (reboot to apply) |
| 1.2.38 / 0.2.38 | 2026-07-05 | 180° screen flip option; on-device pedal assist (2026-06-30) |
| 1.2.37 / 0.2.37 | 2026-06-29 | Companion BLE moved into the foreground-service isolate; reconnect hardening |
| 1.2.36 / 0.2.36 | 2026-06-23 | VESC config throttle-curve range −100..100% |

## v1.2.35 / app 0.2.35 — 2026-06-22

LISP quick-action panel with cruise control and profiles, LVGL partial-render
performance work, battery charge/swap detection by voltage.
