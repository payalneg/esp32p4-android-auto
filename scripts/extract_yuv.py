#!/usr/bin/env python3
"""Extract a base64-dumped I420 frame from an ESP-IDF capture log.

The firmware (main/h264_pipe.c) emits the 10th decoded frame between
sentinel lines:

    === I420_DUMP_BEGIN w=W h=H size=N ===
    <base64 chunks, 64 chars per line>
    === I420_DUMP_END ===

This script finds the block, decodes the base64, writes raw I420 to disk,
and prints the ffplay command line.

Usage:
    python3 scripts/extract_yuv.py logs/<capture>.log [-o frame.yuv]
"""

from __future__ import annotations

import argparse
import base64
import re
import sys
from pathlib import Path

BEGIN_RE = re.compile(
    r"=== I420_DUMP_BEGIN w=(\d+) h=(\d+) size=(\d+) ===")
END = "=== I420_DUMP_END ==="


def main() -> int:
    p = argparse.ArgumentParser()
    p.add_argument("input", nargs="?", help="capture log (default stdin)")
    p.add_argument("-o", "--output", default="frame.yuv",
                   help="output raw I420 (default: frame.yuv)")
    args = p.parse_args()

    text = (Path(args.input).read_text(errors="replace")
            if args.input else sys.stdin.read())

    m = BEGIN_RE.search(text)
    if not m:
        print("error: no I420_DUMP_BEGIN sentinel in input", file=sys.stderr)
        return 1
    w, h, size = (int(m.group(i)) for i in (1, 2, 3))
    after_begin = text[m.end():]
    end_idx = after_begin.find(END)
    if end_idx < 0:
        print("error: no I420_DUMP_END sentinel after begin", file=sys.stderr)
        return 1
    body = after_begin[:end_idx]

    # Parse line-by-line: each dump line is exactly 64 base64 chars; the
    # very last line may be shorter and end with `=` padding. Anything
    # else (interleaved ESP_LOG lines, ANSI codes, timestamps) is dropped.
    line_re = re.compile(r"^[A-Za-z0-9+/]{2,64}={0,2}$")
    chunks = []
    for ln in body.splitlines():
        ln = ln.strip()
        if line_re.match(ln) and (len(ln) == 64 or "=" in ln):
            chunks.append(ln)
    cleaned = "".join(chunks)
    extra = len(cleaned) % 4
    if extra:
        cleaned = cleaned[:-extra]
    raw = base64.b64decode(cleaned, validate=False)

    if len(raw) != size:
        print(f"warning: decoded {len(raw)} bytes, header said {size}",
              file=sys.stderr)
    expected = w * h * 3 // 2
    if len(raw) != expected:
        print(f"warning: expected {expected} bytes for {w}x{h} I420, "
              f"got {len(raw)}", file=sys.stderr)

    Path(args.output).write_bytes(raw)
    print(f"wrote {len(raw)} bytes to {args.output} ({w}x{h} I420)")
    print(f"ffplay -f rawvideo -pixel_format yuv420p -video_size {w}x{h} "
          f"{args.output}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
