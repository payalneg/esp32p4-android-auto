/* global React */
// VESC cyberpunk dashboard — shared primitives + 3 layout variants.
// Exports layout components to window for the canvas app script.

const DATA = {
  speed: 42, mode: "SPORT",
  batPct: 73, batAh: 6.4, range: 38,
  powerKw: 2.8, current: 24.5, maxKw: 4.5,
  voltage: 58.4, trip: 12.6, odo: 6436,
  mTemp: 47, cTemp: 39,
  clock: "14:32", tripTime: "0:48:12",
};

const ghostOf = (s) => String(s).replace(/[0-9]/g, "8");

/* 7-seg readout with dim "off-segment" ghost behind */
function Seg({ value, size, weight = 700, hot, warn, ghost = true, style, className = "" }) {
  return (
    <span className={`seg ${hot ? "hot" : ""} ${warn ? "warn" : ""} ${className}`}
      style={{ fontSize: size, fontWeight: weight, ...style }}>
      {ghost && <span className="ghost" aria-hidden="true">{ghostOf(value)}</span>}
      <span className="val">{value}</span>
    </span>
  );
}

/* vertical segmented bar */
function SegBar({ count = 16, pct, w = 46, h = 300, warnAbove = null, vertical = true }) {
  const on = Math.round((count * pct) / 100);
  return (
    <div className="segbar" style={{ width: w, height: h }}>
      {Array.from({ length: count }).map((_, i) => {
        const lit = i < on;
        const warn = warnAbove != null && (i / count) * 100 >= warnAbove;
        return <div key={i} className={`cell ${lit ? "on" : ""} ${warn ? "warn" : ""}`} style={{ flex: 1 }} />;
      })}
    </div>
  );
}

/* horizontal segmented bar (variant 3) */
function SegRow({ count = 28, pct, w = 320, h = 22, warnAbove = null }) {
  const on = Math.round((count * pct) / 100);
  return (
    <div style={{ display: "flex", gap: 4, width: w, height: h }}>
      {Array.from({ length: count }).map((_, i) => {
        const lit = i < on;
        const warn = warnAbove != null && (i / count) * 100 >= warnAbove;
        return <div key={i} className={`cell ${lit ? "on" : ""} ${warn ? "warn" : ""}`}
          style={{ flex: 1, borderRadius: 2 }} />;
      })}
    </div>
  );
}

/* smile / sweep arc gauge (SVG progress arc) */
const pol = (cx, cy, r, deg) => {
  const a = (deg * Math.PI) / 180;
  return [cx + r * Math.cos(a), cy + r * Math.sin(a)];
};
function arcPath(cx, cy, r, a0, a1) {
  const [x0, y0] = pol(cx, cy, r, a0);
  const [x1, y1] = pol(cx, cy, r, a1);
  const large = Math.abs(a1 - a0) > 180 ? 1 : 0;
  const sweep = a1 > a0 ? 1 : 0;
  return `M ${x0} ${y0} A ${r} ${r} 0 ${large} ${sweep} ${x1} ${y1}`;
}
/* smile: angles 70..110 deg (bottom of a high circle) */
function SmileArc({ pct = 60, width = 300, height = 46, stroke = 10, theta = 18 }) {
  const pad = stroke / 2 + 3;
  const chord = width - 2 * pad;
  const th = (theta * Math.PI) / 180;
  const r = chord / (2 * Math.sin(th));
  const cx = width / 2;
  const bottomY = height - pad;
  const cy = bottomY - r;
  const at = (d) => { const a = (d * Math.PI) / 180; return [cx + r * Math.cos(a), cy + r * Math.sin(a)]; };
  const aL = 90 + theta, aR = 90 - theta;
  const aFill = aL - 2 * theta * (pct / 100);
  const seg = (a0, a1) => {
    const [x0, y0] = at(a0), [x1, y1] = at(a1);
    return `M ${x0.toFixed(2)} ${y0.toFixed(2)} A ${r.toFixed(2)} ${r.toFixed(2)} 0 0 0 ${x1.toFixed(2)} ${y1.toFixed(2)}`;
  };
  return (
    <svg width={width} height={height} viewBox={`0 0 ${width} ${height}`} style={{ display: "block" }}>
      <path d={seg(aL, aR)} fill="none" stroke="var(--amber-dim2)" strokeWidth={stroke} strokeLinecap="round" />
      <path d={seg(aL, aFill)} fill="none" stroke="var(--amber)" strokeWidth={stroke} strokeLinecap="round"
        style={{ filter: "drop-shadow(0 0 5px rgba(255,122,26,.85)) drop-shadow(0 0 12px rgba(255,90,0,.5))" }} />
    </svg>
  );
}

function Topbar({ compact }) {
  return (
    <div className="topbar" style={{
      height: compact ? 38 : 50, padding: "0 20px", gap: 0,
      borderBottom: "1px solid var(--grid-line)", justifyContent: "space-between",
      fontSize: compact ? 14 : 17,
    }}>
      <span className="navitem active">VESC MODE</span>
      <Seg value={DATA.tripTime} size={compact ? 15 : 18} style={{ opacity: .8 }} />
      <span className="navitem active">STATISTICS</span>
      <Seg value={DATA.clock} size={compact ? 15 : 18} style={{ opacity: .8 }} />
      <span className="navitem active">SETTINGS</span>
      <span style={{ display: "flex", alignItems: "center", gap: 7 }}>
        <span className="dot bt on" />
        <span className="navitem" style={{ color: "#2aa9ff", textShadow: "0 0 8px rgba(42,169,255,.6)" }}>BT</span>
      </span>
    </div>
  );
}

/* ============ VARIANT 1 — CLASSIC RESKIN ============ */
function VClassic() {
  return (
    <div className="scr fx">
      <Topbar />
      <div style={{ display: "flex", height: 340 }}>
        {/* battery */}
        <div style={{ width: 210, padding: "16px 18px 10px", display: "flex", flexDirection: "column", borderRight: "1px solid var(--grid-line)" }}>
          <div className="lbl" style={{ fontSize: 22 }}>BATTERY</div>
          <div style={{ display: "flex", alignItems: "baseline", gap: 8, marginTop: 14 }}>
            <Seg value={DATA.batPct} size={46} hot /><span className="unit" style={{ fontSize: 22 }}>%</span>
          </div>
          <div style={{ display: "flex", alignItems: "baseline", gap: 6, marginTop: 6 }}>
            <Seg value={DATA.batAh.toFixed(1)} size={24} ghost={false} /><span className="unit" style={{ fontSize: 16 }}>Ah</span>
          </div>
          <div style={{ flex: 1, display: "flex", alignItems: "stretch", margin: "12px 0 8px" }}>
            <SegBar count={16} pct={DATA.batPct} w={"100%"} h={"100%"} warnAbove={null} />
          </div>
          <div style={{ display: "flex", justifyContent: "space-between", alignItems: "baseline" }}>
            <span className="lbl" style={{ fontSize: 16 }}>RANGE</span>
            <span style={{ display: "flex", alignItems: "baseline", gap: 5 }}>
              <Seg value={DATA.range} size={24} ghost={false} /><span className="unit" style={{ fontSize: 15 }}>KM</span>
            </span>
          </div>
        </div>
        {/* speed */}
        <div style={{ flex: 1, display: "flex", flexDirection: "column", alignItems: "center", justifyContent: "center", position: "relative" }}>
          <div className="lbl" style={{ fontSize: 17, letterSpacing: ".32em", whiteSpace: "nowrap", marginBottom: 20 }}>SPEED · KM/H</div>
          <Seg value={String(DATA.speed).padStart(2, "0")} size={166} hot style={{ lineHeight: .9 }} />
          <div className="lbl" style={{ fontSize: 15, letterSpacing: ".3em", marginTop: 18, color: "var(--amber)" }}>{DATA.mode}</div>
        </div>
        {/* power */}
        <div style={{ width: 210, padding: "16px 18px 10px", display: "flex", flexDirection: "column", alignItems: "flex-end", borderLeft: "1px solid var(--grid-line)" }}>
          <div className="lbl" style={{ fontSize: 22 }}>POWER</div>
          <div style={{ display: "flex", alignItems: "baseline", gap: 8, marginTop: 14 }}>
            <Seg value={DATA.powerKw.toFixed(1)} size={46} hot /><span className="unit" style={{ fontSize: 22 }}>kW</span>
          </div>
          <div style={{ display: "flex", alignItems: "baseline", gap: 6, marginTop: 6 }}>
            <Seg value={DATA.current.toFixed(1)} size={24} ghost={false} /><span className="unit" style={{ fontSize: 16 }}>A</span>
          </div>
          <div style={{ flex: 1, display: "flex", alignItems: "stretch", margin: "12px 0 8px", width: "100%", justifyContent: "flex-end" }}>
            <SegBar count={16} pct={(DATA.powerKw / DATA.maxKw) * 100} w={"100%"} h={"100%"} warnAbove={80} />
          </div>
          <div style={{ display: "flex", justifyContent: "space-between", alignItems: "baseline", width: "100%" }}>
            <span className="lbl" style={{ fontSize: 16 }}>MAX</span>
            <span style={{ display: "flex", alignItems: "baseline", gap: 5 }}>
              <Seg value={DATA.maxKw.toFixed(1)} size={24} ghost={false} /><span className="unit" style={{ fontSize: 15 }}>KW</span>
            </span>
          </div>
        </div>
      </div>
      {/* bottom readouts */}
      <BottomRow />
    </div>
  );
}

function BottomRow() {
  const cells = [
    ["VOLTAGE", DATA.voltage.toFixed(1), "V", false],
    ["TRIP", DATA.trip.toFixed(1), "KM", false],
    ["ODO", DATA.odo, "KM", false],
    ["M·TEMP", DATA.mTemp, "°C", DATA.mTemp > 70],
    ["C·TEMP", DATA.cTemp, "°C", DATA.cTemp > 70],
  ];
  return (
    <div style={{ height: 90, display: "flex", borderTop: "1px solid var(--grid-line)" }}>
      {cells.map((c, i) => (
        <div key={i} style={{ flex: 1, display: "flex", flexDirection: "column", alignItems: "center", justifyContent: "center", gap: 7, borderLeft: i ? "1px solid var(--grid-line)" : "none" }}>
          <span className="lbl" style={{ fontSize: 14 }}>{c[0]}</span>
          <span style={{ display: "flex", alignItems: "baseline", gap: 4 }}>
            <Seg value={c[1]} size={26} warn={c[3]} ghost={false} /><span className="unit" style={{ fontSize: 14 }}>{c[2]}</span>
          </span>
        </div>
      ))}
    </div>
  );
}

window.VClassic = VClassic;
window.DASH = { Seg, SegBar, SegRow, SmileArc, Topbar, BottomRow, DATA };
