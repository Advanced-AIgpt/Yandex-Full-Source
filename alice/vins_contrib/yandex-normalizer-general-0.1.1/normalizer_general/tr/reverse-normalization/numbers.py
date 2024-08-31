#! /usr/bin/env python2
# encoding: utf-8
import sys
sys.path.append('..')

from general.normbase import *
from morphology import *

d1_9 = anyof("123456789")
d2_9 = anyof("23456789")

def hundreds(ord=False):
    if ord:
        maybe_ord_marker = "."
    else:
        maybe_ord_marker = ""
    return (
        word("100" + maybe_ord_marker, need_outer_space=False) |
        glue_words(d2_9,
                   word(remove("1") + "00" + maybe_ord_marker),
                   need_outer_space=False)
    )

card_1_9 = word(d1_9, need_outer_space=False)

card_10_99 = (
    word(d1_9 + "0", need_outer_space=False) |
    glue_words(d1_9 + remove("0"),
               d1_9,
               need_outer_space=False)
)

card_100_999 = (
    hundreds() |
    glue_words(hundreds() >> word(g.digit + "0" + remove("0")),
               card_1_9,
               permit_inner_space=True,
               need_outer_space=False) |
    glue_words(hundreds() >> word(d1_9 + remove("00")),
               card_10_99,
               permit_inner_space=True,
               need_outer_space=False)
)

card_1_999 = card_1_9 | card_10_99 | card_100_999
card_1_999 = card_1_999.optimize()

fill_with_zeroes = word(insert("00") + g.digit |
                        insert("0") + rr(g.digit, 2) |
                        rr(g.digit, 3),
                        need_outer_space=False)

card_ones_block = card_1_999 >> fill_with_zeroes
card_ones_block = card_ones_block.optimize()

def block_with_unit(zeroes):
    return (
        word(insert("00") + "1" + zeroes, need_outer_space=False) |
        glue_words(card_ones_block,
                   remove("1") + zeroes,
                   permit_inner_space=True,
                   need_outer_space=False)
    ).optimize()

card_thousands_block = block_with_unit("000")
card_millions_block = block_with_unit("000000")
card_billions_block = block_with_unit("000000000")

card_glue_blocks = glue_words(qq(rr(g.digit, 3) + remove("000000000")),
                              rr(g.digit, 3) + remove("000000") | insert("000"),
                              rr(g.digit, 3) + remove("000") | insert("000"),
                              rr(g.digit, 3) | insert("000"),
                              need_outer_space=False)
card_glue_blocks = card_glue_blocks.optimize()

raw_cardinal = (
    card_billions_block + qq(card_millions_block) + qq(card_thousands_block) + qq(card_ones_block) |
                          card_millions_block     + qq(card_thousands_block) + qq(card_ones_block) |
                                                    card_thousands_block     + qq(card_ones_block) |
                                                                               card_ones_block
).optimize()

drop_leading_zeros = word(remove(ss("0")) + anyof("123456789") + ss(g.any_symb))
drop_leading_zeros = drop_leading_zeros.optimize()

zero = word("0")

cardinal = (((raw_cardinal >> card_glue_blocks >> drop_leading_zeros) | zero) +
            qq(remove(feats('numeral', 'card'))))
cardinal = cardinal.optimize()

# Ignor the word boundaries here, it still works.
ordinal = cardinal + "." + qq(remove(feats('numeral', 'ord')))
ordinal = ordinal.optimize()

number = cardinal | ordinal
number = number.optimize()

if __name__ == '__main__':
    qq_pre = (insert(" ") + ss(g.any_symb)) >> ss(" " + pp(tagger))
    qq_pre = qq_pre.optimize()

    qqz = qq_pre >> card1_999
    print qqz

    qq = qqz.optimize()
    print qqz

    fst_save(qqz, "numbers")
