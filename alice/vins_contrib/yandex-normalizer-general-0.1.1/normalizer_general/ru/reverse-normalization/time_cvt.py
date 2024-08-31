#! /usr/bin/env python2
# encoding: utf-8
import sys
sys.path.append('..')
sys.path.append('../..')

from general.normbase import *
from morphology import *
import number_conversions as n

print 'Time'

time_cvt = glue_words(qq(anyof("12")) + g.digit + qq(remove(feats('numeral', 'card', 'nom', 'mas'))),
                      replace("час" + qq(anyof(["а", "ов"])),
                              ":"),
                       # More than 10 minutes -- "минут" is optional
                       glue_words(anyof("12345") + g.digit + qq(remove(feats('numeral', 'card', 'nom', 'fem'))),
                                  qq(remove(maybe_stressed("мин+ут") + qq(anyof(["а", "ы"])))),
                                  permit_inner_space=True) |
                       # "ноль" explicitly given -- "минут" is optional
                       glue_words("0" + qq(remove(feats('numeral', 'card', 'nom'))),
                                  g.digit + qq(remove(feats('numeral', 'card', 'nom', 'fem'))),
                                  qq(remove(maybe_stressed("мин+ут") + qq(anyof(["а", "ы"])))),
                                  permit_inner_space=True) |
                       # No explicit "ноль", less than 10 minutes -- obligatorily expect "минут"
                       glue_words(g.digit + qq(remove(feats('numeral', 'card', 'nom', 'fem'))),
                                  remove(maybe_stressed("мин+ут") + qq(anyof(["а", "ы"]))),
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
