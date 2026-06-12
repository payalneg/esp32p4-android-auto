#!/usr/bin/env bash
# Convenience wrapper around the mid2melody host tool: builds it on demand
# (when the binary is missing or scripts/mid2melody.c is newer) and forwards
# all arguments to it. The submodule scripts/midi-parser must be checked out.
#
#   scripts/mid2melody.sh "song.mid"                      # stats + lisp to stdout
#   scripts/mid2melody.sh "song.mid" --track 1 --transpose 12 > /tmp/song.lisp
#
# See scripts/mid2melody.c for the full option list.
set -euo pipefail

here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
src="$here/mid2melody.c"
bin="$here/mid2melody"
parser_src="$here/midi-parser/src/midi-parser.c"
parser_inc="$here/midi-parser/include"

if [ ! -f "$parser_src" ]; then
  echo "error: submodule missing — run: git submodule update --init scripts/midi-parser" >&2
  exit 1
fi

# (re)build when binary is absent or any source is newer than it
if [ ! -x "$bin" ] || [ "$src" -nt "$bin" ] || [ "$parser_src" -nt "$bin" ]; then
  echo "mid2melody: building…" >&2
  cc -std=c11 -O2 -Wall -I "$parser_inc" "$parser_src" "$src" -lm -o "$bin"
fi

exec "$bin" "$@"
