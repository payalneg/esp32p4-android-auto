#!/usr/bin/env bash
# Build the companion APK.
#
# NO API KEY IS EMBEDDED BY DEFAULT. The app asks the user for one and keeps it
# in the Android keystore, which is the only arrangement that does not hand
# your key to whoever ends up with the APK.
#
# --with-key bakes the LLM_API_KEY from .env into the build (XOR-masked, so it
# is not a plaintext `sk-...` string). Be clear about what that masking is
# worth: it defeats automated APK scanners and `strings | grep`, and nothing
# more — anyone willing to run jadx/frida gets the key out. If you use it:
#
#   * use a DEDICATED key with a spend limit,
#   * never share the resulting APK,
#   * rotate the key if that APK ever leaves your device.
#
# Usage:
#   scripts/build_app.sh                 # release APK, no key (default)
#   scripts/build_app.sh --install       # …and adb install -r onto the phone
#   scripts/build_app.sh --with-key      # bake in .env's LLM_API_KEY
#   scripts/build_app.sh --debug         # debug APK (faster, for iterating)
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
APP="$ROOT/flutter-application"
ENV_FILE="$ROOT/.env"

# Must match _mask in lib/agent/embedded_key.dart.
MASK='aa-bridge/vesc-display/2026'

MODE=release
INSTALL=0
USE_KEY=0
for arg in "$@"; do
    case "$arg" in
        --install)  INSTALL=1 ;;
        --with-key) USE_KEY=1 ;;
        --no-key)   USE_KEY=0 ;;   # accepted for muscle memory; already default
        --debug)    MODE=debug ;;
        *) echo "build_app: unknown option $arg" >&2; exit 2 ;;
    esac
done

DEFINES=()
if [[ $USE_KEY -eq 1 && -f "$ENV_FILE" ]]; then
    KEY="$(grep -E '^LLM_API_KEY=' "$ENV_FILE" | head -n1 | cut -d= -f2- | tr -d '"'"'"'[:space:]')"
    if [[ -n "$KEY" ]]; then
        OBF="$(KEY="$KEY" MASK="$MASK" python3 -c '
import base64, os
key = os.environ["KEY"].encode()
mask = os.environ["MASK"].encode()
print(base64.b64encode(bytes(b ^ mask[i % len(mask)] for i, b in enumerate(key))).decode())
')"
        DEFINES+=(--dart-define="LLM_KEY_OBF=$OBF")
        echo "build_app: embedding LLM key from .env (${#KEY} chars, masked)"
        echo "build_app: NOTE — recoverable from the APK; do not share this build"
    else
        echo "build_app: .env has no LLM_API_KEY — building without an embedded key"
    fi
else
    echo "build_app: no embedded key (the app asks the user for one)"
fi

cd "$APP"
if [[ "$MODE" == release ]]; then
    # --obfuscate renames Dart symbols; it does NOT hide string constants,
    # which is exactly why the key is masked above rather than passed raw.
    flutter build apk --release --obfuscate \
        --split-debug-info=build/symbols "${DEFINES[@]}"
    APK="$APP/build/app/outputs/flutter-apk/app-release.apk"
else
    flutter build apk --debug "${DEFINES[@]}"
    APK="$APP/build/app/outputs/flutter-apk/app-debug.apk"
fi
echo "build_app: $APK ($(du -h "$APK" | cut -f1))"

if [[ $INSTALL -eq 1 ]]; then
    ADB="$(command -v adb || true)"
    if [[ -z "$ADB" ]]; then
        for c in "$HOME/Library/Android/sdk/platform-tools/adb" \
                 /opt/homebrew/bin/adb /usr/local/bin/adb; do
            [[ -x "$c" ]] && ADB="$c" && break
        done
    fi
    [[ -n "$ADB" ]] || { echo "build_app: adb not found" >&2; exit 1; }
    "$ADB" install -r "$APK"
fi
