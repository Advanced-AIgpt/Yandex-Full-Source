#! /usr/bin/env python2
# encoding: utf-8
import sys
sys.path.append('..')
sys.path.append('../..')

from general.normbase import *
from numerals import *
import numbers as n

print 'Phone'

def make_phone_cvt():
    nfv = feats('numeral', 'card', 'nom', 'mas')
    three_digits = (
        word(g.digit + g.digit + g.digit + remove(nfv)) |
        glue_words(g.digit + g.digit + remove(nfv),
                   g.digit + remove(nfv)) |
        glue_words(g.digit + remove(nfv),
                   g.digit + g.digit + remove(nfv)) |
        glue_words(g.digit + remove(nfv),
                   g.digit + remove(nfv),
                   g.digit + remove(nfv))
        )
    two_digits = (
        word(g.digit + g.digit + remove(nfv)) |
        glue_words(g.digit + remove(nfv),
                   g.digit + remove(nfv))
        )

    return glue_words(qq(replace("плюс", "+") | "+"),
                      qq(word(g.digit + remove(nfv)) | two_digits),
                      qq(glue_words(insert("("),
                                    three_digits,
                                    insert(")"),
                                    permit_inner_space=True)),
                      three_digits,
                      insert("-"),
                      two_digits,
                      insert("-"),
                      two_digits,
                      permit_inner_space=True)

phone_cvt = make_phone_cvt()

if __name__ == '__main__':
    qq_pre = (insert(" ") + ss(g.any_symb)) >> ss(cost((pp(" " + tagger) >> n.cardinal), 0.01) | cost(word(ss(g.any_symb)), 1))
    qq_pre.remove_epsilon()
    qq_pre.arc_sort_output()

    phone_full = qq_pre >> phone_cvt

    print phone_full

    fst_save(phone_full, "phone")
