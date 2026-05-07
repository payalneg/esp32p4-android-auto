#!/usr/bin/env bash
# Re-pack the embedded firmware blobs (BT agent + C6 slave) used by
# bt_agent_fw / c6_ota_partition. Run this whenever you rebuild the BT
# agent or refresh the C6 slave fw.
#
# Outputs:
#   components/bt_agent_fw/bt_agent_fw.bin.gz
#   components/c6_ota_partition/slave_fw_bin/network_adapter.bin.gz
#
# Source files:
#   tools/bt_agent/build/{bt_agent.bin,bootloader/bootloader.bin,partition_table/partition-table.bin}
#       — produced by `idf.py -C tools/bt_agent build` (requires IDF + esptool in PATH)
#   components/c6_ota_partition/slave_fw_bin/network_adapter.bin
#       — produced by `idf.py -C tools/c6_slave_fw build` (gitignored intermediate)
#
# Pre-compressed `.gz` is what the host build embeds and decompresses into
# PSRAM at OTA time. We keep only the `.gz` in git — saves ~1 MiB host
# flash + repo bloat — so this script is the only way to regenerate them.

set -euo pipefail

repo="$(cd "$(dirname "$0")/.." && pwd)"

bt_app="$repo/tools/bt_agent/build/bt_agent.bin"
bt_bl="$repo/tools/bt_agent/build/bootloader/bootloader.bin"
bt_pt="$repo/tools/bt_agent/build/partition_table/partition-table.bin"
bt_out="$repo/components/bt_agent_fw/bt_agent_fw.bin.gz"

c6_out="$repo/components/c6_ota_partition/slave_fw_bin/network_adapter.bin.gz"
# Try the local rebuild output first (tools/c6_slave_fw/build/...), then
# the legacy components/.../network_adapter.bin location for users who
# dropped the bin in directly. We commit only the .gz, so the .bin under
# components/ would have to be put there manually.
if [[ -f "$repo/tools/c6_slave_fw/build/network_adapter.bin" ]]; then
    c6_src="$repo/tools/c6_slave_fw/build/network_adapter.bin"
else
    c6_src="$repo/components/c6_ota_partition/slave_fw_bin/network_adapter.bin"
fi

filesize() { stat -f %z "$1" 2>/dev/null || stat -c %s "$1"; }

pct() {
    awk -v r="$1" -v g="$2" 'BEGIN { printf "%.0f%%", g*100/r }'
}

# ---------- BT agent ----------
if [[ -f "$bt_app" && -f "$bt_bl" && -f "$bt_pt" ]]; then
    if ! command -v esptool.py >/dev/null; then
        echo "BT agent: esptool.py not in PATH — source IDF's export.sh first" >&2
        exit 1
    fi
    tmp="$(mktemp -t bt_agent_merge.XXXXXX.bin)"
    trap 'rm -f "$tmp"' EXIT
    # --target-offset 0x1000 strips the leading 4 KiB pad: ESP32 ROM rejects
    # writes to 0x0..0x1000 anyway, and bt_agent_ota writes from 0x1000.
    esptool.py --chip esp32 merge_bin --target-offset 0x1000 -o "$tmp" \
        --flash_mode dio --flash_freq 40m --flash_size 2MB \
        0x1000  "$bt_bl" \
        0x8000  "$bt_pt" \
        0x10000 "$bt_app" >/dev/null
    raw=$(filesize "$tmp")
    gzip -9 -c "$tmp" > "$bt_out"
    gz=$(filesize "$bt_out")
    echo "BT agent:   $raw → $gz B ($(pct "$raw" "$gz")) — $bt_out"
else
    echo "BT agent:   build outputs missing under $repo/tools/bt_agent/build — skipping"
fi

# ---------- C6 slave ----------
if [[ -f "$c6_src" ]]; then
    raw=$(filesize "$c6_src")
    gzip -9 -c "$c6_src" > "$c6_out"
    gz=$(filesize "$c6_out")
    echo "C6 slave:   $raw → $gz B ($(pct "$raw" "$gz")) — $c6_out"
else
    echo "C6 slave:   $c6_src missing — skipping"
fi
