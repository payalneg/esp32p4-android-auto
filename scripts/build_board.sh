#!/usr/bin/env bash
# Build (or flash / monitor / size / …) the P4 firmware for a head-unit board.
# Each board gets its own build directory and sdkconfig so the two configs never
# clobber each other, and the per-board sdkconfig.defaults.<board> overlay is
# layered on top of the common sdkconfig.defaults (later file wins).
#
# Usage:
#   scripts/build_board.sh                       # build EVERY board (firmware images)
#   scripts/build_board.sh all [idf.py args...]  # same, optionally with custom args
#   scripts/build_board.sh waveshare build
#   scripts/build_board.sh jc4880 -p /dev/cu.usbmodem* flash monitor
#   scripts/build_board.sh jc4880 size
#
# With no board (or "all"), runs the given idf.py command — defaulting to
# `build` — for each board in turn. A plain `idf.py build` (no wrapper) still
# builds only Waveshare from the base sdkconfig.defaults.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

# Boards with a sdkconfig.defaults.<board> overlay. Keep in sync with release.sh.
BOARDS=(waveshare jc4880)

# Make idf.py available if the caller forgot to source export.sh.
if ! command -v idf.py >/dev/null 2>&1; then
    if [[ -n "${IDF_PATH:-}" && -f "$IDF_PATH/export.sh" ]]; then
        # shellcheck disable=SC1091
        . "$IDF_PATH/export.sh" >/dev/null
    fi
fi
command -v idf.py >/dev/null 2>&1 || {
    echo "build_board: idf.py not found — run '. \$IDF_PATH/export.sh' first" >&2; exit 1; }

# run_board <board> <idf.py args...>
#
# Runs in a subshell because of the Python-environment dance below: each build
# directory remembers the interpreter it was CONFIGURED with, and idf.py flatly
# refuses to build under a different one ("configured with … Run 'idf.py
# fullclean'"). On this machine the two boards were configured with different
# venvs, so building them both from one shell used to fail on the second one —
# which meant scripts/release.sh never got past board #1. Point PATH at the
# interpreter each build dir expects, per board, and both just work.
run_board() (
    local board="$1"; shift
    local overlay="sdkconfig.defaults.${board}"
    [[ -f "$overlay" ]] || { echo "build_board: $overlay not found" >&2; exit 1; }

    local cache="build_${board}/CMakeCache.txt"
    if [[ -f "$cache" ]]; then
        local py
        py="$(sed -n 's/^PYTHON:[A-Za-z]*=//p' "$cache" | head -n1)"
        if [[ -n "$py" && -x "$py" ]]; then
            local bin_dir; bin_dir="$(dirname "$py")"
            if [[ "$(command -v python || true)" != "$py" ]]; then
                echo "build_board: ${board} was configured with $py — using it"
            fi
            export PATH="${bin_dir}:${PATH}"
            export IDF_PYTHON_ENV_PATH="$(dirname "$bin_dir")"
        fi
    fi

    idf.py -B "build_${board}" \
           -D SDKCONFIG="build_${board}/sdkconfig" \
           -D SDKCONFIG_DEFAULTS="sdkconfig.defaults;${overlay}" \
           "$@"
)

board="${1:-all}"

case "$board" in
    all)
        # No board (or explicit "all"): run for every board. Default cmd: build.
        shift || true
        args=("$@"); [[ ${#args[@]} -gt 0 ]] || args=(build)
        for b in "${BOARDS[@]}"; do
            echo "==> build_board: ${b} -> idf.py ${args[*]}"
            run_board "$b" "${args[@]}"
        done
        ;;
    waveshare|jc4880)
        shift
        run_board "$board" "$@"
        ;;
    *)
        echo "usage: $0 [all|waveshare|jc4880] [idf.py args...]" >&2
        exit 2
        ;;
esac
