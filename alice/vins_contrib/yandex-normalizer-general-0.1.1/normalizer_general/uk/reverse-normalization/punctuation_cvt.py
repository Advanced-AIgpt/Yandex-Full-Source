#! /usr/bin/env python2
# encoding: utf-8
import sys
sys.path.append('..')
sys.path.append('../..')

from general.normbase import *
import categories

print 'Punctuation'

def make_upcase():
    r = failure()
    lowercase = set(u'abcdefghijklmnopqrstuvwxyzçıöşüабвгдеёжзийклмнопрстуфхцчшщъыьэюяґієї\'')
    for c in lowercase:
        r = r | replace(c, c.upper())
    for c in g.sym_set - lowercase:
        r = r | c
    # Don't ever convert "www". This variant, being longer, will always be preferred.
    r = r | "www"
    r = r.optimize()
    return r

upcase = make_upcase()

# Need to set the cost low so that it does not interfere with other convertors.
# Upcase the very first symbol and symbols after end-of-sentence markers.
capitalize = (
    "" |
    upcase + ss(anyof('.!?') + " " + upcase | cost(g.any_symb, 0.01))
)

# Glue punctuation to previous words
glue = ss(remove(" ") + anyof('.,!?') | cost(g.any_symb, 0.01))

punctuation_cvt = capitalize >> glue
punctuation_cvt = punctuation_cvt.optimize()
