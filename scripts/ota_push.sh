#!/usr/bin/env bash
# Push build/esp32p4_android_auto.bin to the running head unit's
# OTA endpoint. Defaults match the SoftAP gateway IP (192.168.4.1) so
# this works when run from a laptop that joined the head unit's AP.
#
# Usage:
#   scripts/ota_push.sh [host]            # use default binary path
#   BIN=path/to/app.bin scripts/ota_push.sh [host]
#   PORT=8080 scripts/ota_push.sh 10.0.0.5
#
# Env:
#   BIN   path to app binary             (default: build/esp32p4_android_auto.bin)
#   PORT  HTTP port on the device        (default: 80)
#
# Exits non-zero on any failure. On success the device replies
# "OK rebooting" and restarts; the script prints the response and exits.

set -euo pipefail

HOST="${1:-192.168.4.1}"
PORT="${PORT:-80}"
BIN="${BIN:-build/esp32p4_android_auto.bin}"

if [[ ! -f "$BIN" ]]; then
    echo "ota_push: $BIN not found — run 'idf.py build' first" >&2
    exit 1
fi

SIZE=$(wc -c < "$BIN" | tr -d ' ')
URL="http://${HOST}:${PORT}/ota"

echo "ota_push: uploading $BIN (${SIZE} bytes) -> ${URL}"

# --progress-bar goes to stderr, response body comes back in $RESP.
RESP=$(curl --fail --show-error --progress-bar --no-buffer \
            --max-time 600 \
            -H 'Content-Type: application/octet-stream' \
            --data-binary "@${BIN}" \
            "${URL}")
echo "${RESP}"
echo "ota_push: device is rebooting"
