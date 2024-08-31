#!/usr/bin/python
import sys
import codecs
sys.stdin = codecs.getreader('utf-8')(sys.stdin)
sys.stdout = codecs.getwriter('utf-8')(sys.stdout)

import string
mapping = {}
for i, c in enumerate(string.punctuation):
    mapping[c] = chr(ord('A') + i % 26)
    if i >= 26:
        mapping[c] += '0'

for line in sys.stdin:
    res = []
    for c in line.rstrip():
        to = mapping.get(c, c)
        res.append(to)
    print ''.join(res)

