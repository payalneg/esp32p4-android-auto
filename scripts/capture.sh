#!/usr/bin/env bash
# Capture serial output from the ESP32-P4 for N seconds into logs/<timestamp>.log.
#
# Usage:
#   scripts/capture.sh                # 60 s, default port, no flash
#   scripts/capture.sh 120             # 120 s
#   scripts/capture.sh 60 --flash      # flash first, then capture
#   PORT=/dev/cu.foo scripts/capture.sh
#
# Notes:
#   - Stops early on Ctrl-C and still writes the partial log.
#   - Strips most non-printable chars but keeps ANSI colour codes intact;
#     editors/grep see clean text.
#   - Resets ESP32 via DTR/RTS at start so we always catch boot logs.

set -euo pipefail

DURATION="${1:-60}"
FLASH=""
for arg in "${@:2}"; do
    case "$arg" in
        --flash|-f) FLASH=1 ;;
    esac
done

REPO="$(cd "$(dirname "$0")/.." && pwd)"
PORT="${PORT:-/dev/cu.usbmodem5B5E0700881}"
BAUD="${BAUD:-115200}"

if [ ! -e "$PORT" ]; then
    echo "Serial port $PORT not found. Plug the board in or set PORT=..." >&2
    exit 1
fi

mkdir -p "$REPO/logs"
TS="$(date +%Y%m%d-%H%M%S)"
LOG="$REPO/logs/$TS.log"

if [ -n "$FLASH" ]; then
    echo "Flashing $PORT before capture..." >&2
    (cd "$REPO" && idf.py -p "$PORT" -b 460800 flash) >&2
fi

echo "Capturing $DURATION s from $PORT @ $BAUD → $LOG" >&2

# Use pyserial via the ESP-IDF Python env (always present once IDF is installed).
# IDF_PYTHON_ENV_PATH is only set inside an exported IDF shell, so try a few
# known locations and fall back to plain python3 (which probably won't have
# pyserial — that case prints a clear error).
PYTHON="${IDF_PYTHON_ENV_PATH:+$IDF_PYTHON_ENV_PATH/bin/python}"
if [ -z "$PYTHON" ] || [ ! -x "$PYTHON" ]; then
    for cand in /Users/alexey/.espressif/python_env/idf*/bin/python; do
        [ -x "$cand" ] && { PYTHON="$cand"; break; }
    done
fi
PYTHON="${PYTHON:-python3}"

"$PYTHON" - "$PORT" "$BAUD" "$DURATION" "$LOG" <<'PYEOF'
import sys, time, serial

port, baud, dur, out = sys.argv[1], int(sys.argv[2]), float(sys.argv[3]), sys.argv[4]

s = serial.Serial(port, baud, timeout=0.5)
# Standard ESP32 normal-boot reset:
#   DTR controls IO0 (boot mode), RTS controls EN (reset). Both active-low.
#   For a normal boot we want IO0=high (DTR=False) the whole time, and pulse
#   EN low→high (RTS=True→False).
# Keeping RTS asserted after the pulse leaves the chip in reset — that's the
# bug the previous version had.
s.dtr = False
s.rts = True            # EN low → chip held in reset
time.sleep(0.1)
s.rts = False           # EN high → chip boots normally
s.reset_input_buffer()

end = time.time() + dur
with open(out, "wb") as f:
    try:
        while time.time() < end:
            data = s.read(4096)
            if data:
                f.write(data); f.flush()
                # Mirror to stdout so you can watch live too.
                sys.stdout.buffer.write(data); sys.stdout.flush()
    except KeyboardInterrupt:
        pass
s.close()
PYEOF

echo >&2
echo "Done. $(wc -c < "$LOG" | tr -d ' ') bytes → $LOG" >&2
echo "$LOG"
