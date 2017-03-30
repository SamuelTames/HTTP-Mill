#!/usr/bin/env python

f = open("headers.dat", "r")
lines = [line for line in f if line.strip()]
f.close()
lines.sort()

for line in lines:
    if line.startswith('#'):
        continue
    print '  {.header = "%s", .count = 0},' % line.strip()
