#! /usr/bin/env python2
# encoding: utf-8
import sys
sys.path.append('..')
sys.path.append('../..')

from general.normbase import *
from morphology import *
import number_conversions as n

print 'Phone'

def make_phone_cvt():
    nfv = feats('numeral', 'card', 'nom', 'mas')
    three_digits = (
        word(rr(g.digit,3) + qq(remove(nfv))) |
        glue_words(rr(g.digit, 2) + qq(remove(nfv)),
                   g.digit + qq(remove(nfv))) |
        glue_words(g.digit + qq(remove(nfv)),
                   rr(g.digit, 2) + qq(remove(nfv))) |
        glue_words(g.digit + qq(remove(nfv)),
                   g.digit + qq(remove(nfv)),
                   g.digit + qq(remove(nfv)))
        )
    two_digits = (
        word(rr(g.digit, 2) + qq(remove(nfv))) |
        glue_words(g.digit + qq(remove(nfv)),
                   g.digit + qq(remove(nfv)))
        )

    return glue_words(qq(replace_stressed("пл+юс", "+") | "+"),
                      qq(word(g.digit + qq(remove(nfv))) | two_digits),
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
