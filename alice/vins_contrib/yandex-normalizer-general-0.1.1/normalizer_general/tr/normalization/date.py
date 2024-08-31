#! /usr/bin/env python2
# encoding: utf-8
import sys
sys.path.append('..')
sys.path.append('../..')

from general.normbase import *
import categories

print 'Dates'

MONTHS = [(1, "ocak"),
          (2, "şubat"),
          (3, "mart"),
          (4, "nisan"),
          (5, "mayıs"),
          (6, "haziran"),
          (7, "temmuz"),
          (8, "ağustos"),
          (9, "eylül"),
          (10, "ekim"),
          (11, "kasım"),
          (12, "aralık"),
          ]

def month_by_number(n, name):
    in_pat = (Fst(unicode(n)) |
              Fst("%02d" % n))
    return replace(in_pat, name)

month_to_month_name = anyof(month_by_number(n, name)
                            for n, name in MONTHS)

date = (
    qq(remove("0") | anyof("123")) + g.digit +
    replace(anyof("./"), " ") +
    month_to_month_name +
    replace(anyof("./"), " ") +
    (rr(g.digit, 2) | rr(g.digit, 4))
).optimize()

cvt = convert_words(date)
