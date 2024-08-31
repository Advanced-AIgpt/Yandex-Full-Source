#! /usr/bin/env python2
# encoding: utf-8
import sys
sys.path.append('..')
sys.path.append('../..')

from general.normbase import *
import categories

print 'Units'

UNIT_SPELLOUT = [
    ("g", "gram"),
    ("l", "litre"),
    ("m", "metre"),
    ("w", "vat"),
]

spell_unit = (
    qq(anyof("+-")) +
    pp(g.digit) +
    qq(anyof(".,") + pp(g.digit)) +
    ss(" ") +
    anyof(replace(abbr + qq(ss(" ") + "."),
                  " " + full + " ")
          for abbr, full in UNIT_SPELLOUT)
).optimize()

cvt = convert_words(spell_unit, permit_inner_space=True)

