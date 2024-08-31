#! /usr/bin/env python2
# encoding: utf-8
import sys
sys.path.append('..')
sys.path.append('../..')

from general.normbase import *
from numerals import *
import numbers as n

print 'Time'

time_cvt = glue_words(qq(anyof("12")) + g.digit + remove(ss("-а")) + remove(feats('numeral', 'card', 'nom', 'mas', 'ord', 'nom', 'fem')),
                      replace(qq("годин" + ss("а")), ":"),
                      glue_words(anyof("12345") + g.digit + remove(feats('numeral', 'card', 'nom', 'fem')),
                                  qq(remove("хвилин" + ss("а"))),
                                  permit_inner_space=True) |
                       # "ноль" explicitly given -- "минут" is optional
                      glue_words("0" + remove(feats('numeral', 'card', 'nom')),
                                  g.digit + remove(feats('numeral', 'card', 'nom', 'fem')),
                                  qq(remove("хвилин" + ss("a"))),
                                  permit_inner_space=True) |
                       # No explicit "ноль", less than 10 minutes -- obligatorily expect "минут"
                      glue_words(g.digit + remove(feats('numeral', 'card', 'nom', 'fem')),
                                  remove("хвилин" + qq("а")),
                                  permit_inner_space=True),
                      permit_inner_space=True)
time_cvt = time_cvt.optimize()

if __name__ == '__main__':
    qq_pre = (insert(" ") + ss(g.any_symb)) >> ss((pp(" " + tagger) >> n.cardinal) | cost(word(ss(g.any_symb))))
    qq_pre = qq_pre.optimize()

    time_full = qq_pre >> time_cvt

    print 'time_full', time_full

    time_full = time_full.optimize()

    print 'time_full, optimized', time_full

    fst_save(time_full, "time")
