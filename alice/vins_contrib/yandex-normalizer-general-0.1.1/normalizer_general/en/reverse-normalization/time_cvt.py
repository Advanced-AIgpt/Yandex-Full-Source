#! /usr/bin/env python2
# encoding: utf-8
import sys
sys.path.append('..')
sys.path.append('../..')

from general.normbase import *
from numerals import *
import numbers as n

print 'Time'

time_cvt = rr(g.digit, 1, 2) + replace(pp(" ") + "o'clock", ":00")
time_cvt = pp(time_cvt | cost(g.any_symb, 1))
time_cvt = time_cvt.optimize()

if __name__ == '__main__':
    qq_pre = (insert(" ") + ss(g.any_symb)) >> ss((pp(" " + tagger) >> n.cardinal) | cost(word(ss(g.any_symb))))
    qq_pre = qq_pre.optimize()

    time_full = qq_pre >> time_cvt

    print 'time_full', time_full

    time_full = time_full.optimize()

    print 'time_full, optimized', time_full

    fst_save(time_full, "time")
