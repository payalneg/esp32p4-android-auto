#!/usr/bin/env bash
# Capture serial output from the D1 Mini ESP32 BT agent into logs/bt-<ts>.log.
#
# Usage:
#   scripts/capture_bt.sh                  # run until Ctrl-C
#   scripts/capture_bt.sh 60               # 60 s, then auto-stop
#   scripts/capture_bt.sh 60 --flash       # rebuild+flash bt_agent first
#   BT_PORT=/dev/cu.foo scripts/capture_bt.sh
#
# Resets the D1 Mini via DTR/RTS at start (so we always see boot lines)
# and leaves the lines un-asserted on exit (chip stays running).

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

PORT="${BT_PORT:-/dev/cu.usbserial-02BOJDQ2}"
BAUD="${BAUD:-115200}"

if [ ! -e "$PORT" ]; then
    echo "Serial port $PORT not found. Plug the D1 Mini in or set BT_PORT=..." >&2
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
    echo "Flashing $PORT (bt_agent)..." >&2
    if ! command -v idf.py >/dev/null 2>&1; then
        for cand in /Users/alexey/.espressif/v*/esp-idf/export.sh; do
            [ -f "$cand" ] && { . "$cand" >/dev/null 2>&1 || true; break; }
        done
    fi
    if ! command -v idf.py >/dev/null 2>&1; then
        echo "idf.py not found. Source ESP-IDF manually." >&2
        exit 1
    fi
    if ! (cd "$BT_DIR" && idf.py -p "$PORT" -b 460800 flash) >&2; then
        echo "Flash FAILED." >&2
        exit 1
    fi
fi

TS="$(date +%Y%m%d-%H%M%S)"
LOG="$REPO/logs/bt-$TS.log"

echo "Capturing ${DURATION:-∞} s from $PORT @ $BAUD → $LOG" >&2

"$PYTHON" - "$PORT" "$BAUD" "${DURATION:-0}" "$LOG" <<'PYEOF'
import sys, time, signal, serial

port, baud, dur, out = sys.argv[1], int(sys.argv[2]), float(sys.argv[3]), sys.argv[4]
s = serial.Serial(port, baud, timeout=0.5)
s.dtr = False
s.rts = True
time.sleep(0.1)
s.rts = False
s.reset_input_buffer()

running = True
def stop(*_):
    global running
    running = False
signal.signal(signal.SIGTERM, stop)
signal.signal(signal.SIGINT,  stop)

end = (time.time() + dur) if dur > 0 else float('inf')
with open(out, "wb") as f:
    while running and time.time() < end:
        try:
            data = s.read(4096)
        except Exception:
            break
        if data:
            f.write(data); f.flush()
            sys.stdout.buffer.write(data); sys.stdout.flush()
s.close()
PYEOF

echo                                          >&2
echo "Done. $(wc -c < "$LOG" | tr -d ' ') bytes → $LOG" >&2
echo "$LOG"
