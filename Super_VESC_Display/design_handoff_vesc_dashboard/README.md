# Handoff: VESC E-Bike Dashboard (Cyberpunk Amber)

## Overview
A full-screen instrument cluster for an electric bike running a **VESC** motor controller.
Target hardware display: **800 × 480 px** (landscape, fixed resolution).
The design shows live ride telemetry — speed, battery, power, temperatures — in a
"cyberpunk" amber-on-black aesthetic with a 7-segment LCD numeric typeface and warm glow.

The chosen direction is **Variant A — "Classic Reskin"**: it keeps the exact information
layout of the original VESC stock display (battery left, big speed center, power right,
secondary readouts along the bottom) and reskins it.

## About the Design Files
The files in this bundle are **design references created in HTML/React (via Babel in the
browser)** — a prototype showing the intended look and behavior. They are **not** meant to
be shipped as-is to the device.

The task is to **recreate this design in the target environment** for the VESC display —
e.g. an embedded UI framework (LVGL/C, Qt/QML), a React/web kiosk, or whatever renders on
the actual screen — using that environment's patterns. If no environment exists yet, pick
the most appropriate one for an 800×480 embedded display. Treat the HTML as the visual
spec, not the implementation.

## Fidelity
**High-fidelity (hifi).** Final colors, typography, spacing, layout and glow treatment are
all intended as-shown. Recreate pixel-accurately at 800×480. The numbers shown are sample
telemetry values (see Design Tokens → Sample data) — wire them to real VESC data.

## Canvas / Coordinate system
- Fixed art-board: **800 × 480 px**, origin top-left. All measurements below are in this space.
- The screen is divided into 3 horizontal bands:
  - **Top status bar** — height **50px**
  - **Main row** — height **340px** (battery | speed | power)
  - **Bottom readout row** — height **90px**
  - (50 + 340 + 90 = 480)

## Screens / Views

### Screen: Ride Dashboard (single screen)
**Purpose:** at-a-glance riding telemetry.

**Layout (top → bottom):**

**1. Top status bar** (`height 50px`, padding `0 20px`, bottom border `1px solid rgba(255,122,26,0.14)`)
A single flex row, `justify-content: space-between`, font-size 17px:
- `VESC MODE` (nav label, amber-bright, glow-s)
- trip timer `0:48:12` (7-seg, 18px, opacity .8)
- `STATISTICS` (nav label)
- clock `14:32` (7-seg, 18px, opacity .8)
- `SETTINGS` (nav label)
- Bluetooth group: a 8px blue dot (`#2aa9ff`, glow) + `BT` label in blue
  Nav labels: font Chakra Petch 700, letter-spacing .12em, uppercase, `white-space: nowrap`.

**2. Main row** (`height 340px`, flex row):

  **a) BATTERY column** — fixed width **210px**, padding `16px 18px 10px`, right border `1px solid grid-line`, flex column:
  - `BATTERY` label (Chakra Petch 600, 22px, uppercase, letter-spacing .18em, color `--label`)
  - Value row (margin-top 14px, align-items baseline, gap 8px): big `73` (7-seg, **46px, "hot" style**) + `%` unit (22px)
  - Sub row (margin-top 6px, gap 6px): `6.4` (7-seg, 24px, **no ghost**) + `Ah` unit (16px)
  - **Vertical segmented bar** — fills remaining height (`flex:1`, margin `12px 0 8px`), 16 cells, fill from bottom, `73%` lit
  - Footer row (space-between, baseline): `RANGE` label (16px) + `38` (7-seg 24px, no ghost) + `KM` unit (15px)

  **b) SPEED column** — `flex: 1` (center), flex column centered:
  - `SPEED · KM/H` label (Chakra Petch 600, 17px, letter-spacing .32em, nowrap, **margin-bottom 20px**)
  - `42` — the hero number (7-seg, **166px, "hot" style**, line-height .9). Value is zero-padded to 2 digits.
  - `SPORT` — ride mode label (Chakra Petch 600, 15px, letter-spacing .3em, color `--amber`, margin-top 18px)
  - *(Note: an earlier version had a curved "smile" arc gauge under the speed — it was intentionally removed. Do not add it.)*

  **c) POWER column** — fixed width **210px**, padding `16px 18px 10px`, left border `1px solid grid-line`, flex column, **right-aligned** (`align-items: flex-end`):
  - `POWER` label (22px)
  - Value row (baseline, gap 8px): `2.8` (7-seg, **46px, hot**) + `kW` unit (22px)
  - Sub row (gap 6px): `24.5` (7-seg, 24px, no ghost) + `A` unit (16px)
  - **Vertical segmented bar** — fills remaining height, 16 cells, lit to `power/maxKw` = `2.8/4.5 ≈ 62%`, **warn (red) above 80%**
  - Footer row (space-between, full width, baseline): `MAX` label (16px) + `4.5` (7-seg 24px, no ghost) + `KW` unit (15px)

**3. Bottom readout row** (`height 90px`, top border `1px solid grid-line`, flex row of 5 equal cells):
Each cell: flex column, centered, gap 7px, left border `1px solid grid-line` (except first). Contains a small label (Chakra Petch 600, 14px) above a value (7-seg, 26px, no ghost) + unit (14px). The five cells:
1. `VOLTAGE` — `58.4` `V`
2. `TRIP` — `12.6` `KM`
3. `ODO` — `6436` `KM`
4. `M·TEMP` — `47` `°C`  (motor temp; turns **red/warn** if > 70)
5. `C·TEMP` — `39` `°C`  (controller temp; turns **red/warn** if > 70)

## Components (reusable primitives)

### `Seg` — 7-segment numeric readout
The signature element. A number rendered in the **DSEG7 Classic** font with a warm glow.
Behind the lit value sits an optional dim "ghost" layer showing all segments as `8`s
(simulating unlit LCD segments) at `--amber-dim` with no glow.
- Props/states:
  - **default**: value color `--amber-bright`, text-shadow `--glow-m`
  - **hot** (`.hot`): value color `--amber-hot` (`#ffe6c2`), text-shadow `--glow-l` — used for the hero numbers (speed, battery %, power kW, current/maxKw big values)
  - **warn** (`.warn`): value color `--red` (`#ff3b2f`), red glow — used for over-temp
  - **ghost** on/off: ghost backdrop is ON for large hero numbers only; OFF for all small/secondary numbers (Ah, range, current, max, and the whole bottom row), because the leading-digit ghost looked like an artifact on short values.
- Ghost text = value with every digit replaced by `8`.

### `SegBar` — vertical segmented bar gauge
- Column of N equal cells (default 16), `flex-direction: column-reverse` so it fills from the bottom, `gap 4px`.
- Cell off: background `--amber-dim`, radius 2px, subtle inset border.
- Cell on: background `linear-gradient(90deg, --amber-deep, --amber)`, glow-s + inner highlight.
- Cell warn+on: red gradient (`#9c1500 → --red`) + red glow. `warnAbove` prop = % threshold above which lit cells go red.
- Lit count = `round(count * pct / 100)`.

### Labels
- `.lbl` — section/field labels: Chakra Petch 600, uppercase, letter-spacing .18em, color `--label`.
- `.unit` — unit suffixes: Chakra Petch 600, letter-spacing .08em, color `--label`.
- `.navitem` — top-bar items: Chakra Petch 700, letter-spacing .12em, nowrap; `.active` adds amber-bright + glow-s.

## Screen treatment (the "fx" overlay)
The whole panel (`.scr`) has:
- Background: near-black `--bg` (#0a0806) with two warm radial gradients (top-center orange wash + bottom warm glow).
- A non-interactive overlay (`.fx::after`, z-index 50) combining:
  - **Scanlines**: `repeating-linear-gradient(to bottom, transparent 0 2px, rgba(0,0,0,0.16) 2px 3px)`
  - **Vignette**: `radial-gradient(130% 100% at 50% 50%, transparent 55%, rgba(0,0,0,0.55) 100%)`
- `-webkit-font-smoothing: antialiased`, `user-select: none`.

## Interactions & Behavior
This is currently a **static mockup** with sample values. For the real device, drive these from VESC telemetry:
- **speed** → hero number (km/h, zero-padded to 2 digits)
- **battery %** + **Ah used** + **range (km)** → battery column + left bar (bar = battery %)
- **power kW** + **current A** + **max kW** → power column + right bar (bar = power/maxKw %, red above 80%)
- **voltage, trip, odo, motor temp, controller temp** → bottom row
- **temps** flip to red/warn when > 70 °C (threshold is illustrative — set to the real safe limit).
- **clock** + **trip timer** → top bar.
- Top-bar items (`VESC MODE / STATISTICS / SETTINGS`) are placeholders for navigation to other screens (not yet designed).

Suggested polish for implementation (not in the mock): animate numeric changes, ease bar fills, smooth glow. Keep updates ≤ a few Hz to avoid flicker on the embedded panel.

## State Management
Minimal — a single telemetry object updated from the controller. Shape (see `DATA` in `variant-classic.jsx`):
```
{ speed, mode, batPct, batAh, range, powerKw, current, maxKw,
  voltage, trip, odo, mTemp, cTemp, clock, tripTime }
```

## Design Tokens

### Colors
| Token | Value | Use |
|---|---|---|
| `--bg` | `#0a0806` | screen background |
| `--bg-panel` | `#100b07` | panel background |
| `--amber` | `#ff7a1a` | primary amber (bars, accents) |
| `--amber-bright` | `#ffb35c` | default lit digit |
| `--amber-hot` | `#ffe6c2` | hero digit (hottest) |
| `--amber-deep` | `#c8470a` | gradient base for lit bar cells |
| `--amber-dim` | `rgba(255,122,26,0.09)` | unlit segment ghost / off bar cell |
| `--amber-dim2` | `rgba(255,122,26,0.16)` | faint fills |
| `--grid-line` | `rgba(255,122,26,0.14)` | dividers / borders |
| `--red` | `#ff3b2f` | warn / over-temp |
| `--label` | `rgba(255,176,110,0.55)` | labels & units |
| `--label-dim` | `rgba(255,176,110,0.35)` | idle labels |
| Bluetooth blue | `#2aa9ff` | BT dot + label |

### Glows (text-shadow / drop-shadow)
| Token | Value |
|---|---|
| `--glow-s` | `0 0 6px rgba(255,122,26,.55), 0 0 16px rgba(255,90,0,.35)` |
| `--glow-m` | `0 0 10px rgba(255,138,30,.6), 0 0 30px rgba(255,90,0,.4)` |
| `--glow-l` | `0 0 16px rgba(255,150,40,.7), 0 0 50px rgba(255,90,0,.45), 0 0 90px rgba(255,70,0,.25)` |
| warn glow | `0 0 10px rgba(255,59,47,.7), 0 0 28px rgba(255,40,0,.4)` |

### Typography
- **Numeric / readouts:** `DSEG7 Classic` (7-segment LCD font). Source used in mock: `@fontsource/dseg7-classic` (weights 400 & 700). On device, embed an equivalent 7-segment font. Fallback `monospace`.
- **Labels / UI:** `Chakra Petch` (Google Fonts), weights 400/500/600/700.
- Sizes (px): hero speed **166**; section big values **46**; secondary values **24–26**; section labels **22**; bottom labels **14**; unit suffixes **14–22**; top-bar **17**.
- Letter-spacing: labels .18em, SPEED label .32em, mode .3em, nav .12em.

### Spacing / layout
- Bands: 50 / 340 / 90 px. Side columns 210px each, center flexes.
- Column padding `16px 18px 10px`. Bar cell gap 4px, cell radius 2px.
- Borders/dividers all `1px solid --grid-line`.

### Sample data (placeholder telemetry)
`speed 42 (SPORT) · battery 73% · 6.4 Ah · range 38 km · power 2.8 kW · current 24.5 A · max 4.5 kW · voltage 58.4 V · trip 12.6 km · odo 6436 km · motor 47 °C · controller 39 °C · timer 0:48:12 · clock 14:32`

## Assets
- `reference_original_display.png` — photo of the stock VESC display this layout mirrors.
- `reference_cyberpunk_style.png` — the cyberpunk amber style reference the look is based on.
- Fonts are loaded from CDN in the mock (Google Fonts: Chakra Petch; jsDelivr: DSEG7). For the device, **bundle** both font files locally — do not rely on the CDN.

## Files in this bundle
- `VESC Dashboard (Variant A).html` — standalone, self-scaling reference page. Open in a browser to see the target. (It letterboxes the fixed 800×480 panel to fit the window.)
- `variant-classic.jsx` — the React/Babel source for Variant A and its primitives (`Seg`, `SegBar`, `BottomRow`, `Topbar`). This is the authoritative structural reference.
- `styles.css` — all design tokens, the 7-seg/ghost treatment, bar styles, and the scanline/vignette overlay.

> Note: `variant-classic.jsx` also exports two unused primitives (`SegRow`, `SmileArc`) and a `DASH` bundle that other (non-included) variants consumed. `SmileArc` is the removed speed arc — ignore both unless you want them.
