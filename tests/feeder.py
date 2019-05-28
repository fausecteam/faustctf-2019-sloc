#!/bin/python3

import sys

f = sys.argv[1]
c = open(f).read()
print("1 %d\n%s" % (len(c), c), end="")
