#!/usr/bin/env python3
"""gzip a file reproducibly: compress_gz.py <in> <out.gz>

Stands in for `gzip -9 -n` in the build, which is not available on Windows.
Like `-n`, this writes NO original filename and NO mtime into the header, so
the same input always produces byte-identical output (the firmware embeds the
result in .rodata, and a header that changes per build would churn the image).
"""
import gzip
import sys


def main(src, dst):
    with open(src, "rb") as f_in, open(dst, "wb") as raw_out:
        # filename="" + mtime=0 is what suppresses the FNAME/MTIME fields;
        # GzipFile derives them from the output file otherwise.
        with gzip.GzipFile(filename="", mode="wb", compresslevel=9,
                           fileobj=raw_out, mtime=0) as f_out:
            f_out.write(f_in.read())


if __name__ == "__main__":
    if len(sys.argv) != 3:
        sys.exit("usage: compress_gz.py <in> <out.gz>")
    main(sys.argv[1], sys.argv[2])
