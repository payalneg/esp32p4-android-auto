import gzip, sys
with open(sys.argv[1], 'rb') as f_in:
    with gzip.GzipFile(sys.argv[2], 'wb', mtime=0) as f_out:
        f_out.write(f_in.read())
