#!/usr/bin/env python3
"""Preview the VESC LISP buzzer melodies on a computer.

Parses the `(def NAME '((freq dur) ...))` melody lists straight out of a LISP
file (default: lisp/main.lisp) and renders them as a square wave — the same
harsh timbre the motor/FOC tone generator produces — so you can audition a
melody without uploading to the VESC every time.

No third-party deps: writes a 16-bit mono WAV with the stdlib `wave` module and
plays it with `afplay` (macOS). On other platforms it falls back to `aplay` /
`ffplay`, or just leave the .wav with --save.

Examples:
  scripts/play_melody.py                 # play melody2 from lisp/main.lisp
  scripts/play_melody.py melody          # play song 1 (Polish Cow)
  scripts/play_melody.py --list          # list melodies found in the file
  scripts/play_melody.py melody2 --save /tmp/fb.wav --no-play
  scripts/play_melody.py melody2 --wave sine --vol 0.5
"""
import argparse
import math
import os
import re
import shutil
import struct
import subprocess
import sys
import tempfile
import wave

RATE = 44100


def find_repo_default():
    here = os.path.dirname(os.path.abspath(__file__))
    cand = os.path.join(os.path.dirname(here), "lisp", "main.lisp")
    return cand if os.path.exists(cand) else "lisp/main.lisp"


def parse_melodies(path):
    """Return {name: [(freq, dur), ...]} for every (def NAME '( ... )) in file."""
    src = open(path, encoding="utf-8").read()
    melodies = {}
    for m in re.finditer(r"\(def\s+([A-Za-z0-9_-]+)\s+'\(", src):
        name = m.group(1)
        # walk parens from the opening of the quoted list to its match
        i = m.end() - 1  # at the '(' of the list
        depth = 0
        j = i
        while j < len(src):
            c = src[j]
            if c == "(":
                depth += 1
            elif c == ")":
                depth -= 1
                if depth == 0:
                    break
            j += 1
        body = src[i : j + 1]
        notes = [(int(f), float(d)) for f, d in re.findall(r"\((\d+)\s+([0-9.]+)\)", body)]
        if notes:
            melodies[name] = notes
    return melodies


def render(notes, vol=0.35, wave_kind="square"):
    """Render notes -> list of int16 samples.

    Mirrors the firmware play-list: a short silence is carved out of the end of
    each tone (gap = d/3 for d<90 ms, else 30 ms) so notes are articulated.
    """
    amp = int(max(0.0, min(1.0, vol)) * 32767)
    out = []
    for freq, dur in notes:
        if freq <= 0:  # rest
            out.extend([0] * int(dur * RATE))
            continue
        gap = (dur / 3.0) if dur < 0.09 else 0.03
        n = int((dur - gap) * RATE)
        period = RATE / freq
        fade = min(n // 2, int(0.004 * RATE))  # 4 ms click-suppression ramp
        buf = [0] * n
        for k in range(n):
            if wave_kind == "sine":
                s = math.sin(2 * math.pi * freq * k / RATE)
            else:  # square (buzzer-like)
                s = 1.0 if (k % period) < (period / 2) else -1.0
            env = 1.0
            if k < fade:
                env = k / fade
            elif k > n - fade:
                env = (n - k) / fade
            buf[k] = int(amp * s * env)
        out.extend(buf)
        out.extend([0] * int(gap * RATE))  # articulation gap
    return out


def write_wav(path, samples):
    with wave.open(path, "wb") as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(RATE)
        w.writeframes(struct.pack("<%dh" % len(samples), *samples))


def play(path):
    for player in ("afplay", "ffplay", "aplay"):
        exe = shutil.which(player)
        if not exe:
            continue
        args = [exe, path]
        if player == "ffplay":
            args = [exe, "-autoexit", "-nodisp", "-loglevel", "quiet", path]
        subprocess.run(args)
        return True
    return False


def main():
    ap = argparse.ArgumentParser(description="Preview VESC LISP buzzer melodies.")
    ap.add_argument("melody", nargs="?", default="melody2", help="melody name (default: melody2)")
    ap.add_argument("--file", default=find_repo_default(), help="LISP file to read")
    ap.add_argument("--list", action="store_true", help="list melodies and exit")
    ap.add_argument("--vol", type=float, default=0.35, help="volume 0..1 (default 0.35)")
    ap.add_argument("--wave", choices=("square", "sine"), default="square")
    ap.add_argument("--save", metavar="OUT.wav", help="also write the WAV here")
    ap.add_argument("--no-play", action="store_true", help="don't play, just render/save")
    args = ap.parse_args()

    melodies = parse_melodies(args.file)
    if not melodies:
        sys.exit(f"no melodies found in {args.file}")

    if args.list:
        for name, notes in melodies.items():
            secs = sum(d for _, d in notes)
            snd = [f for f, _ in notes if f > 0]
            rng = f"{min(snd)}-{max(snd)} Hz" if snd else "—"
            print(f"  {name:<10} {len(notes):>4} notes  {secs:6.1f}s  {rng}")
        return

    if args.melody not in melodies:
        sys.exit(f"melody '{args.melody}' not found. available: {', '.join(melodies)}")

    notes = melodies[args.melody]
    secs = sum(d for _, d in notes)
    print(f"{args.melody}: {len(notes)} notes, {secs:.1f}s, {args.wave} wave, vol {args.vol}")
    samples = render(notes, vol=args.vol, wave_kind=args.wave)

    out = args.save or os.path.join(tempfile.gettempdir(), f"{args.melody}.wav")
    write_wav(out, samples)
    if args.save:
        print(f"wrote {out}")

    if not args.no_play:
        if not play(out):
            print(f"no audio player found; WAV at {out}")


if __name__ == "__main__":
    main()
