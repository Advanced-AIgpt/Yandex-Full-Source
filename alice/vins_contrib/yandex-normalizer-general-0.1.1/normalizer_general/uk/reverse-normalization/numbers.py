#! /usr/bin/env python2
# encoding: utf-8
import sys
sys.path.append('..')
sys.path.append('../..')

from general.normbase import *
from numerals import *

# Cardinals
print 'Cardinals'

card_1_19 = word(qq("1") + g.digit + feats('numeral', 'card'))

def card_20_99_of_case(case, **kwargs):
    d1_9 = anyof("123456789")
    d2_9 = anyof("23456789")
    return glue_words(d2_9 + remove("0" + feats('numeral', 'card', case, 'nom')),	\
                      d1_9 + feats('numeral', 'card', case),					\
                      **kwargs) |			      	      				\
           word(d2_9 + "0" + feats('numeral', 'card', case))

card_20_99 = anyof([card_20_99_of_case(case) for case in cat_values('case')])

def card_100_999_of_case(case):
    d1_9 = anyof("123456789")
    fv = feats('numeral', 'card', case)
    fv_lax = feats('numeral', 'card', case, 'nom')
    x20_99 = glue_words(d1_9 + remove("00" + fv_lax),
                        card_20_99_of_case(case, need_outer_space=False),
                        permit_inner_space=True)
    x10_19 = glue_words(d1_9 + remove("00" + fv_lax),
                        "1" + g.digit + fv)
    x1_9 = glue_words(d1_9 + "0" + remove("0" + fv_lax),
                      d1_9 + fv)
    x0 = word(d1_9 + "00" + fv)
    return x0 | x1_9 | x10_19 | x20_99

card_100_999 = anyof([card_100_999_of_case(case) for case in cat_values('case')])

card_1_999 = card_1_19 | card_20_99 | card_100_999

fill_with_zeroes = word((insert("00") + g.digit | \
                        insert("0") + g.digit + g.digit | \
                        g.digit + g.digit + g.digit) + \
                       feats('numeral', 'card'))

card_ones_block = card_1_999 >> fill_with_zeroes

def block_with_unit(stem, zeroes, gender, case):
    stem = stem
    zeroes = zeroes

    d_but1 = anyof("023456789")
    d234 = anyof("234")
    d056789 = anyof("056789")

    # no number => 001
    x001 = word(replace(stem, "001" + zeroes) +
                replace(feats('noun', 'sg', case), feats('numeral', 'card', gender, case)))
    x_ends_in_1 = glue_words(g.digit + d_but1 + "1" +
                             remove(feats('numeral', 'card', gender, case)),
                            replace(stem, zeroes) +
                             replace(feats('noun', 'sg', case), feats('numeral', 'card', case)))

    if case in ['nom', 'acc']:
        x_ends_in_234 = glue_words(g.digit + d_but1 + d234 +
                                     remove(feats('numeral', 'card', case)),
                                   replace(stem, zeroes) +
                                     replace(feats('noun', 'sg', 'gen'), feats('numeral', 'card', case)))
        x_ends_in_056789 = glue_words(g.digit + ("1" + g.digit |
                                                 d_but1 + d056789) +
                                      remove(feats('numeral', 'card', case)),
                                      replace(stem, zeroes) +
                                      replace(feats('noun', 'pl', 'gen'), feats('numeral', 'card', case)))
        x_rest = x_ends_in_234 | x_ends_in_056789
    else:
        x_rest = glue_words((g.digit + ("11" |
                                        (g.digit + d_but1)) +
                             remove(feats('numeral', 'card', case))),
                            replace(stem, zeroes) +
                            replace(feats('noun', 'pl', case), feats('numeral', 'card', case)))
    res = x001 | x_ends_in_1 | x_rest
    return res


make_card_thousands_block = anyof([block_with_unit("тисяч", "000", 'fem', case) for case in cat_values('case')])
make_card_millions_block = anyof([block_with_unit("мільйон", "000000", 'mas', case) for case in cat_values('case')])
make_card_billions_block = anyof([block_with_unit("мільярд", "000000000", 'mas', case) for case in cat_values('case')])

card_thousands_block = (qq(card_ones_block) + word(ss(g.any_symb))) >> make_card_thousands_block
card_millions_block = (qq(card_ones_block) + word(ss(g.any_symb))) >> make_card_millions_block
card_billions_block = (qq(card_ones_block) + word(ss(g.any_symb))) >> make_card_billions_block

card_block = card_ones_block | card_thousands_block | card_millions_block | card_billions_block

raw_cardinal = (
    card_billions_block + qq(card_millions_block) + qq(card_thousands_block) + qq(card_ones_block) |
                          card_millions_block     + qq(card_thousands_block) + qq(card_ones_block) |
                                                    card_thousands_block     + qq(card_ones_block) |
                                                                               card_ones_block
)

def make_card_clue_blocks(case, gender):
    fv = feats('numeral', 'card', case, gender)
    # Middle blocks can have either nominal or the target case
    fv_lax = feats('numeral', 'card', case, 'nom', gender)
    return (
        glue_words(qq(g.digit + g.digit + g.digit + remove("000000000" + fv_lax)),
                   (g.digit + g.digit + g.digit + remove("000000" + fv_lax)) | insert("000"),
                   (g.digit + g.digit + g.digit + remove("000" + fv_lax)) | insert("000"),
                   (g.digit + g.digit + g.digit + remove(fv)) | insert("000")) +
        insert(fv)
    )

card_glue_blocks = anyof([make_card_clue_blocks(case, gender) for case in cat_values('case') for gender in cat_values('gender')])
card_glue_blocks = card_glue_blocks.optimize()

drop_leading_zeros = word(remove(ss("0")) + anyof("123456789") + ss(g.any_symb))
drop_leading_zeros = drop_leading_zeros.optimize()

def make_zero(case):
    return word("0" + replace("@" + feats('noun', 'sg', case), feats('numeral', 'card', 'case')))

zero = anyof([make_zero(case) for case in cat_values('case')])

cardinal = (raw_cardinal >> card_glue_blocks >> drop_leading_zeros) | zero
cardinal = cardinal.optimize()


# Ordinals
print 'Ordinals'

ord_ending = "-" + ss(g.letter) + feats('numeral','ord')
ord1_9 = word(anyof("123456789") + ord_ending)

d_but1 = anyof("023456789")
d2_9 = anyof("23456789")

ord_2digits = (
    glue_words(insert("0"), ord1_9) |
    word("1" + g.digit + ord_ending) |
    word(d2_9 + "0" + ord_ending)    |
    glue_words(d2_9 + remove("0" + feats('numeral', 'card', 'nom')),
               ord1_9)
)

ord_3digits = (
    # 1 - 99
    glue_words(insert("0"), ord_2digits, permit_inner_space=True) |
    # 100
    word("100-" + ss(g.letter) + feats('numeral', 'ord')) |
    # 200 - 900
    glue_words(d2_9 + remove(feats('numeral', 'card', 'gen')),
               remove("1") + "00-" + ss(g.letter) + feats('numeral', 'ord'),
               need_outer_space=False)  |
    # 201 - 999
    glue_words(g.digit + remove("00" + feats('numeral', 'card', 'nom')),
               ord_2digits,
               permit_inner_space=True)
)

ord_ones_block = word(g.digit + g.digit + g.digit + ord_ending)
ord_ones_block = ord_ones_block.optimize()

def ord_big_block(zeros):
    return glue_words(
        ((card_ones_block >> word(g.digit + g.digit + g.digit + remove(feats('numeral', 'card', 'gen')))) |
         # Exactly 100 is a special case
         ("100" + remove(feats('numeral', 'card', 'nom'))) |
         # Just тысячный and such
         insert("001") |
         # 21000 and the like -- analysis is linguistically unsound, but convenient
         glue_words(card_ones_block >> word(g.digit + d_but1 + remove("0" + feats('numeral', 'card', 'nom', 'gen'))),
                    # одно
                    "1" + remove(feats('numeral', 'card', 'nom', 'neu')),
                    need_outer_space=False,
                    permit_inner_space=True)
        ),
        remove("1") + zeros + ord_ending,
        permit_inner_space=True,
        need_outer_space=False
    ).optimize()

ord_thousands_block = ord_big_block("000")
ord_millions_block = ord_big_block("000000")
ord_billions_block = ord_big_block("000000000")

def make_ord_glue_blocks():
    cfv = feats('numeral', 'card', 'nom')
    return (
        # billions
        word(g.digit + g.digit + g.digit + "000000000" + ord_ending) |
        glue_words(qq(g.digit + g.digit + g.digit + remove("000000000" + cfv)),
                   g.digit + g.digit + g.digit + "000000" + ord_ending |
                   glue_words(g.digit + g.digit + g.digit + remove("000000" + cfv) | insert("000"),
                              g.digit + g.digit + g.digit + "000" + ord_ending |
                              glue_words(g.digit + g.digit + g.digit + remove("000" + cfv) | insert("000"),
                                         ord_ones_block),
                              permit_inner_space=True),
                   permit_inner_space=True)
    ).optimize()

ord_glue_blocks = make_ord_glue_blocks()

raw_ordinal = (
    qq(card_billions_block) + qq(card_millions_block) + qq(card_thousands_block) + ord_3digits |
    qq(card_billions_block) + qq(card_millions_block) + ord_thousands_block |
    qq(card_billions_block) + ord_millions_block |
    ord_billions_block
)

ordinal = raw_ordinal >> ord_glue_blocks >> drop_leading_zeros
ordinal = ordinal.optimize()

# Digit sequences
print 'Digit sequences'

def make_digit_seq(d):
    # 1, 2 repetitions need gender
    if d == 0:
        gender = 'mas'
    else:
        gender = 'fem'

    r = failure()
    df = unicode(d)

    r |= glue_words(remove("1" + feats('numeral', 'card', 'nom', gender)),
                    df + remove("@" + feats('noun', 'nom', 'sg')))
    r |= glue_words(remove("2" + feats('numeral', 'card', 'nom', gender)),
                    replace(df + "@" + feats('noun', 'gen', 'sg'), df + df))
    for n in [3, 4]:
        r |= glue_words(remove(unicode(n) + feats('numeral', 'card', 'nom')),
                        replace(df + "@" + feats('noun', 'gen', 'sg'), rr(df, n)))
    for n in [5, 6, 7, 8, 9]:
        r |= glue_words(remove(unicode(n) + feats('numeral', 'card', 'nom')),
                        replace(df + "@" + feats('noun', 'gen', 'pl'), rr(df, n)))

    r = r.determinize()
    r.minimize()
    r.remove_epsilon()

    return r

digit_seq = anyof([make_digit_seq(i) for i in xrange(0, 10)])
digit_seq = digit_seq.optimize()

# Number in general
print 'Number in general'
number = cardinal | ordinal | digit_seq
number = number.optimize()

if __name__ == '__main__':
    qq_pre = (insert(" ") + ss(g.any_symb)) >> ss(" " + pp(tagger))
    qq_pre = qq_pre.optimize()


    qqz = qq_pre >> cardinal
    #qqz = qq_pre >> ord_3digits
    #qqz = qq_pre >> ordinal
    #qqz = qq_pre >> digit_seq
    #print 'Composing wth tagger'
    #qqz = qq_pre >> number
    #print 'Done composing'

    #xx = (card_ones_block >> (g.digit + g.digit + g.digit + remove(feats('numeral', 'card', 'gen'))))
    #qqz = qq_pre >> card_ones_block >> (g.digit + g.digit + g.digit + remove(feats('numeral', 'card', 'gen')))

    # Make sure the FST is sane
    print qqz

    qqz = qqz.optimize()
    print qqz

    fst_save(qqz, "numbers")
