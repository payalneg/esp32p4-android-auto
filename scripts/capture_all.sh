#!/usr/bin/env bash
# Capture three log streams in parallel:
#   - ESP32-P4 serial      → logs/<ts>.log
#   - D1 Mini BT agent     → logs/bt-<ts>.log
#   - Android phone adb    → logs/phone-<ts>.log
#
# Usage:
#   scripts/capture_all.sh                 # until Ctrl-C
#   scripts/capture_all.sh 90              # 90 s
#   scripts/capture_all.sh 90 --flash      # flash P4+BT first, then capture
#
# All three logs share the same timestamp suffix so they line up. Ctrl-C or
# the duration timeout cleanly stops every stream.

set -euo pipefail

REPO="$(cd "$(dirname "$0")/.." && pwd)"
BT_DIR="$REPO/tools/bt_agent"
mkdir -p "$REPO/logs"

DURATION=""
FLASH=""
for arg in "$@"; do
    case "$arg" in
        --flash|-f) FLASH=1 ;;
        ''|*[!0-9]*) ;;
        *) DURATION="$arg" ;;
    esac
done

P4_PORT="${PORT:-/dev/cu.usbmodem5B5E0700881}"
BT_PORT="${BT_PORT:-/dev/cu.usbserial-02BOJDQ2}"
BAUD="${BAUD:-115200}"

# --- Pre-flight checks ---
if [ ! -e "$P4_PORT" ]; then
    echo "P4 port $P4_PORT not found." >&2
    exit 1
fi
if [ ! -e "$BT_PORT" ]; then
    echo "BT port $BT_PORT not found (D1 Mini)." >&2
    exit 1
fi
if ! command -v adb >/dev/null 2>&1; then
    echo "adb not found in PATH." >&2
    exit 1
fi
if [ "$(adb devices | awk 'NR>1 && $2=="device" {print $1}' | wc -l | tr -d ' ')" = "0" ]; then
    echo "No adb device. Plug phone via USB and authorise." >&2
    exit 1
fi

PYTHON="${IDF_PYTHON_ENV_PATH:+$IDF_PYTHON_ENV_PATH/bin/python}"
if [ -z "$PYTHON" ] || [ ! -x "$PYTHON" ]; then
    for cand in /Users/alexey/.espressif/python_env/idf*/bin/python; do
        [ -x "$cand" ] && { PYTHON="$cand"; break; }
    done
fi
PYTHON="${PYTHON:-python3}"

# --- Flash both targets if asked ---
if [ -n "$FLASH" ]; then
    if ! command -v idf.py >/dev/null 2>&1; then
        for cand in /Users/alexey/.espressif/v*/esp-idf/export.sh; do
            [ -f "$cand" ] && { . "$cand" >/dev/null 2>&1 || true; break; }
        done
    fi
    if ! command -v idf.py >/dev/null 2>&1; then
        echo "idf.py not found. Source ESP-IDF manually." >&2
        exit 1
    fi
    echo "Flashing P4 ($P4_PORT)..." >&2
    if ! (cd "$REPO" && idf.py -p "$P4_PORT" -b 460800 flash) >&2; then
        echo "P4 flash FAILED." >&2
        exit 1
    fi
    echo "Flashing BT agent ($BT_PORT)..." >&2
    if ! (cd "$BT_DIR" && idf.py -p "$BT_PORT" -b 460800 flash) >&2; then
        echo "BT flash FAILED." >&2
        exit 1
    fi
fi

TS="$(date +%Y%m%d-%H%M%S)"
P4_LOG="$REPO/logs/$TS.log"
BT_LOG="$REPO/logs/bt-$TS.log"
PH_LOG="$REPO/logs/phone-$TS.log"

echo "P4    serial → $P4_LOG"  >&2
echo "BT    serial → $BT_LOG"  >&2
echo "Phone adb    → $PH_LOG"  >&2
echo "(Ctrl-C to stop, or wait ${DURATION:-forever} s)" >&2
echo                            >&2

# --- Spawn all three children ---

# P4 serial
"$PYTHON" - "$P4_PORT" "$BAUD" "$P4_LOG" <<'PYEOF' &
import sys, signal, serial, time
port, baud, out = sys.argv[1], int(sys.argv[2]), sys.argv[3]
s = serial.Serial(port, baud, timeout=0.5)
s.dtr = False; s.rts = True; time.sleep(0.1); s.rts = False
s.reset_input_buffer()
running = True
def stop(*_):
    global running; running = False
signal.signal(signal.SIGTERM, stop)
signal.signal(signal.SIGINT,  stop)
with open(out, "wb") as f:
    while running:
        try: data = s.read(4096)
        except Exception: break
        if data: f.write(data); f.flush()
s.close()
PYEOF
P4_PID=$!

# BT serial (D1 Mini)
"$PYTHON" - "$BT_PORT" "$BAUD" "$BT_LOG" <<'PYEOF' &
import sys, signal, serial, time
port, baud, out = sys.argv[1], int(sys.argv[2]), sys.argv[3]
s = serial.Serial(port, baud, timeout=0.5)
s.dtr = False; s.rts = True; time.sleep(0.1); s.rts = False
s.reset_input_buffer()
running = True
def stop(*_):
    global running; running = False
signal.signal(signal.SIGTERM, stop)
signal.signal(signal.SIGINT,  stop)
with open(out, "wb") as f:
    while running:
        try: data = s.read(4096)
        except Exception: break
        if data: f.write(data); f.flush()
s.close()
PYEOF
BT_PID=$!

# Phone adb
adb logcat -c >/dev/null 2>&1 || true
adb logcat -v time > "$PH_LOG" &
ADB_PID=$!

cleanup() {
    echo                                                   >&2
    echo "Stopping captures..."                            >&2
    kill -TERM "$P4_PID" "$BT_PID" "$ADB_PID" 2>/dev/null  || true
    wait "$P4_PID" 2>/dev/null || true
    wait "$BT_PID" 2>/dev/null || true
    wait "$ADB_PID" 2>/dev/null || true
    P4_BYTES=$(wc -c < "$P4_LOG"  2>/dev/null | tr -d ' ' || echo 0)
    BT_BYTES=$(wc -c < "$BT_LOG"  2>/dev/null | tr -d ' ' || echo 0)
    PH_BYTES=$(wc -c < "$PH_LOG"  2>/dev/null | tr -d ' ' || echo 0)
    echo "  P4    : $P4_BYTES bytes → $P4_LOG"   >&2
    echo "  BT    : $BT_BYTES bytes → $BT_LOG"   >&2
    echo "  Phone : $PH_BYTES bytes → $PH_LOG"   >&2
    echo "$P4_LOG"
    echo "$BT_LOG"
    echo "$PH_LOG"
}
trap cleanup EXIT INT TERM

if [ -n "$DURATION" ]; then
    sleep "$DURATION"
else
    wait -n "$P4_PID" "$BT_PID" "$ADB_PID" 2>/dev/null || true
fi
