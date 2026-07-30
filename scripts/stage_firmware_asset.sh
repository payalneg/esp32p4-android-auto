#!/usr/bin/env bash
# Stage the freshly-built per-board P4 firmware images into the Flutter app's
# assets so they get bundled into the APK for on-device (over-WiFi) OTA. The app
# picks the matching image at flash time from the board model the head unit
# reports over BLE / GET /info (see lib/firmware/firmware_updater.dart).
#
# Copies, for each board:
#   build_<board>/esp32p4_android_auto.bin
#       -> flutter-application/assets/firmware/esp32p4_android_auto-<board>.bin
# and once:
#   version.txt -> flutter-application/assets/firmware/version.txt
#
# Run after building every board (scripts/build_board.sh <board> build) and
# before `flutter build apk`. Keeps the bundled binaries and the version string
# the app shows in lockstep with what was built.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
VER="${ROOT}/version.txt"
DEST="${ROOT}/flutter-application/assets/firmware"
BOARDS=(waveshare jc4880)

mkdir -p "$DEST"

for board in "${BOARDS[@]}"; do
    BIN="${ROOT}/build_${board}/esp32p4_android_auto.bin"
    if [[ ! -f "$BIN" ]]; then
        echo "stage_firmware_asset: $BIN not found — run 'scripts/build_board.sh ${board} build' first" >&2
        exit 1
    fi
    cp "$BIN" "$DEST/esp32p4_android_auto-${board}.bin"
    echo "stage_firmware_asset: bundled ${board}: $(wc -c < "$BIN" | tr -d ' ') bytes"
done

# First line of version.txt only (strip any trailing blank lines / comments).
head -n1 "$VER" | tr -d '[:space:]' > "$DEST/version.txt"
echo "stage_firmware_asset: version $(cat "$DEST/version.txt")"

# --- VESC BLE Helper (ESP32-C3) ----------------------------------------------
#
# The helper's firmware is bundled too, so the app can flash it with no network
# — a garage is exactly where you need it and exactly where there is no Wi-Fi.
# It is fetched from that project's GitHub releases rather than copied from a
# sibling checkout: the helper releases on its own schedule, and this way the
# binary is never committed here (assets/firmware/ is git-ignored).
#
# Offline or rate-limited, we keep whatever is already staged instead of
# failing: the P4 firmware is what this script exists for.
HELPER_REPO="${HELPER_REPO:-payalneg/esp32c3-ble-helper}"

fetch_helper_fw() {
    local api="https://api.github.com/repos/${HELPER_REPO}/releases/latest"
    local json
    json="$(curl -fsSL -H 'Accept: application/vnd.github+json' "$api" 2>/dev/null)" || return 1

    # The release also ships <name>-merged.bin — a full esptool image
    # (bootloader + partition table + app). Flashing THAT over OTA bricks the
    # helper, and it is listed first, so match digits-and-dots only.
    local pick
    pick="$(JSON="$json" python3 -c '
import json, os, re, sys
rel = json.loads(os.environ["JSON"])
rx = re.compile(r"^esp32c3_ble_helper-([0-9][0-9.]*)\.bin$")
for a in rel.get("assets", []):
    m = rx.match(a.get("name", ""))
    if m:
        print(m.group(1), a["browser_download_url"])
        sys.exit(0)
sys.exit(1)
')" || return 1

    local ver url
    ver="${pick%% *}"; url="${pick#* }"
    curl -fsSL -o "$DEST/esp32c3_ble_helper.bin.tmp" "$url" || return 1
    mv "$DEST/esp32c3_ble_helper.bin.tmp" "$DEST/esp32c3_ble_helper.bin"
    printf '%s\n' "$ver" > "$DEST/esp32c3_ble_helper_version.txt"
    echo "stage_firmware_asset: helper $ver" \
         "($(wc -c < "$DEST/esp32c3_ble_helper.bin" | tr -d ' ') bytes, from $HELPER_REPO)"
}

if ! fetch_helper_fw; then
    rm -f "$DEST/esp32c3_ble_helper.bin.tmp"
    if [[ -f "$DEST/esp32c3_ble_helper.bin" ]]; then
        echo "stage_firmware_asset: helper fetch failed — keeping staged" \
             "$(cat "$DEST/esp32c3_ble_helper_version.txt" 2>/dev/null || echo '?')" >&2
    else
        echo "stage_firmware_asset: helper fetch failed and nothing staged —" \
             "the app will offer a download instead" >&2
        : > "$DEST/esp32c3_ble_helper.bin"
        : > "$DEST/esp32c3_ble_helper_version.txt"
    fi
fi
