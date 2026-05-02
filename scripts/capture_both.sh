#!/usr/bin/env bash
# Capture ESP32 serial AND Android phone logcat in parallel into logs/.
#
# Usage:
#   scripts/capture_both.sh                 # run until Ctrl-C
#   scripts/capture_both.sh 60              # run for 60 s, then auto-stop
#   scripts/capture_both.sh 60 --flash      # flash ESP first, then capture
#   scripts/capture_both.sh --flash         # flash, then run until Ctrl-C
#
# Both files are printed up-front so you always know where they live.
# Ctrl-C cleanly stops both captures and prints a summary.

set -euo pipefail

REPO="$(cd "$(dirname "$0")/.." && pwd)"
mkdir -p "$REPO/logs"

# Args parsing — first positional may be a number (duration) OR --flash.
DURATION=""
FLASH=""
for arg in "$@"; do
    case "$arg" in
        --flash|-f) FLASH=1 ;;
        ''|*[!0-9]*) ;;          # ignore non-numeric
        *) DURATION="$arg" ;;
    esac
done

PORT="${PORT:-/dev/cu.usbmodem5B5E0700881}"
BAUD="${BAUD:-115200}"

if [ ! -e "$PORT" ]; then
    echo "Serial port $PORT not found." >&2
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

if [ -n "$FLASH" ]; then
    echo "Flashing $PORT..." >&2
    # idf.py only works after sourcing export.sh. Outside an exported IDF
    # shell the previous version of this script silently no-op'd, leaving
    # the device on the old firmware — caused hours of confusion. Source
    # the env explicitly here and bail loudly if anything goes wrong.
    if ! command -v idf.py >/dev/null 2>&1; then
        for cand in /Users/alexey/.espressif/v*/esp-idf/export.sh; do
            [ -f "$cand" ] && { . "$cand" >/dev/null 2>&1 || true; break; }
        done
    fi
    if ! command -v idf.py >/dev/null 2>&1; then
        echo "idf.py not found even after trying ~/.espressif/v*/esp-idf/export.sh." >&2
        echo "Source ESP-IDF manually and re-run." >&2
        exit 1
    fi
    if ! (cd "$REPO" && idf.py -p "$PORT" -b 460800 flash) >&2; then
        echo "Flash FAILED — keeping old firmware on the chip." >&2
        exit 1
    fi
fi

TS="$(date +%Y%m%d-%H%M%S)"
ESP_LOG="$REPO/logs/$TS.log"
PHONE_LOG="$REPO/logs/phone-$TS.log"

echo "ESP32 serial → $ESP_LOG"   >&2
echo "Android adb  → $PHONE_LOG" >&2
echo "(press Ctrl-C to stop, or wait ${DURATION:-forever} s)" >&2
echo                              >&2

# --- Start ESP serial capture in background ---
"$PYTHON" - "$PORT" "$BAUD" "$ESP_LOG" <<'PYEOF' &
import sys, signal, serial, time
port, baud, out = sys.argv[1], int(sys.argv[2]), sys.argv[3]
s = serial.Serial(port, baud, timeout=0.5)
s.dtr = False
s.rts = True             # EN low — reset asserted
time.sleep(0.1)
s.rts = False            # EN high — boot normally
s.reset_input_buffer()
running = True
def stop(*_):
    global running
    running = False
signal.signal(signal.SIGTERM, stop)
signal.signal(signal.SIGINT,  stop)
with open(out, "wb") as f:
    while running:
        try:
            data = s.read(4096)
        except Exception:
            break
        if data:
            f.write(data); f.flush()
s.close()
PYEOF
ESP_PID=$!

# --- Start adb logcat in background ---
adb logcat -c >/dev/null 2>&1 || true
adb logcat -v time > "$PHONE_LOG" &
ADB_PID=$!

cleanup() {
    echo                         >&2
    echo "Stopping captures..."  >&2
    kill -TERM "$ESP_PID" 2>/dev/null || true
    kill -TERM "$ADB_PID" 2>/dev/null || true
    wait "$ESP_PID" 2>/dev/null || true
    wait "$ADB_PID" 2>/dev/null || true
    ESP_BYTES=$(wc -c < "$ESP_LOG"   2>/dev/null | tr -d ' ' || echo 0)
    PH_BYTES=$(wc -c < "$PHONE_LOG"  2>/dev/null | tr -d ' ' || echo 0)
    echo "  ESP32 : $ESP_BYTES bytes → $ESP_LOG"   >&2
    echo "  Phone : $PH_BYTES bytes → $PHONE_LOG"  >&2
    echo "$ESP_LOG"
    echo "$PHONE_LOG"
}
trap cleanup EXIT INT TERM

if [ -n "$DURATION" ]; then
    sleep "$DURATION"
else
    # Wait for either child to exit (e.g. user kills adb manually).
    wait -n "$ESP_PID" "$ADB_PID" 2>/dev/null || true
fi
