#!/usr/bin/python
# coding=utf-8
import codecs
import sys
import string
import re
sys.stdin = codecs.getreader('utf-8')(sys.stdin)
sys.stdout = codecs.getwriter('utf-8')(sys.stdout)
sys.stderr = codecs.getwriter('utf-8')(sys.stderr)

PUNCT_REGEX = re.compile(ur'''([''' + string.punctuation + ur'''\\])''')

for i, line in enumerate(sys.stdin):
    s = line.strip().lower()
    s = re.sub(PUNCT_REGEX, ur' \1 ', s).strip()
    s = re.sub(ur'\s+', ' ', s).strip()
    if not s:
        print >> sys.stderr, 'WARNING: empty reply! #', i, 'original: "', line.strip() + '"'
        s = u'что ?'
    print s
