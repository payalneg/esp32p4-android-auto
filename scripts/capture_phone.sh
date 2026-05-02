#!/usr/bin/env bash
# Capture Android phone logcat into logs/phone-<timestamp>.log.
#
# Usage:
#   scripts/capture_phone.sh                  # save all, filter live to screen
#   scripts/capture_phone.sh --all-live       # also print every line live
#   scripts/capture_phone.sh --filter         # save filtered only (smaller file)
#
# Stop with Ctrl-C. File path is printed up-front so it's never a mystery.

set -euo pipefail

REPO="$(cd "$(dirname "$0")/.." && pwd)"
mkdir -p "$REPO/logs"
TS="$(date +%Y%m%d-%H%M%S)"
LOG="$REPO/logs/phone-$TS.log"

# Sanity: is adb on PATH and is a device connected?
if ! command -v adb >/dev/null 2>&1; then
    echo "adb not found in PATH. Install Android Platform Tools:" >&2
    echo "  brew install --cask android-platform-tools" >&2
    exit 1
fi

DEVICES="$(adb devices | awk 'NR>1 && $2=="device" {print $1}' | wc -l | tr -d ' ')"
if [ "$DEVICES" = "0" ]; then
    echo "No adb device. Plug phone via USB, allow USB debugging, then re-run." >&2
    echo "  adb devices    # should show 'device' (not 'unauthorized')" >&2
    exit 1
fi

# Filter: AAP-related tags. Add more as needed.
FILTER='aap|aagh|wireless.?helper|hurev|gearhead|gal|carproject|galserv|projection|androidauto'

MODE="${1:-}"
case "$MODE" in
    --filter)
        echo "Saving FILTERED logcat → $LOG" >&2
        adb logcat -c
        adb logcat -v time \
          | grep --line-buffered -iE "$FILTER" \
          | tee "$LOG"
        ;;
    --all-live)
        echo "Saving ALL logcat (live to screen too) → $LOG" >&2
        adb logcat -c
        adb logcat -v time | tee "$LOG"
        ;;
    *)
        echo "Saving ALL logcat → $LOG (live screen shows filtered only)" >&2
        adb logcat -c
        # All goes to file; only matches print to screen.
        adb logcat -v time \
          | tee "$LOG" \
          | grep --line-buffered -iE "$FILTER"
        ;;
esac
