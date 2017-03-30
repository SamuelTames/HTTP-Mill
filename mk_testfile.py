#!/usr/bin/env python
# this code is mostly pasted python snippets and quite slow.

import sys
import random
import string

stringspace = string.ascii_letters + " "
max_value_len = 20

def random_string():
    return ''.join(random.choice(stringspace) for i in range(random.randint(0,max_value_len)))

lines = []
with open('headers.dat') as f:
    lines = f.read().splitlines()

lines = map(lambda s: s.strip() + ":" + random_string() + "\r\n", lines)

for i in xrange(1000000):
    sys.stdout.write(random.choice(lines))
    sys.stdout.flush()
