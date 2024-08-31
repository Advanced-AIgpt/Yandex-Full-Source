#! /usr/bin/env python2
# encoding: utf-8
import sys
sys.path.append('..')
sys.path.append('../..')

from general.normbase import *
import morphology

print "Numbers"

maybe_tail = qq(".") + qq(feats('numeral'))

digit_but_0 = anyof("123456789")
digit_but_01 = anyof("23456789")

num_0 = ("0" + maybe_tail >> morphology.producer).optimize()

num_1_9 = (digit_but_0 + maybe_tail >> morphology.producer).optimize()

num_10_99 = (
    (digit_but_0 + "0" + maybe_tail >> morphology.producer) |
    ((digit_but_0 + insert("0") >> morphology.producer) +
     insert(" ") + num_1_9)
).optimize()

num_2digits = (
    remove("0") + num_1_9 |
    num_10_99
).optimize()

num_100_999 = (
    ("100" + maybe_tail >> morphology.producer) |
    (("1" + insert("00") >> morphology.producer) +
     insert(" ") +
     num_2digits) |
    ((digit_but_01 >> morphology.producer) +
     insert(" ") +
     (insert("1") + "00" + maybe_tail >> morphology.producer)) |
    ((digit_but_01 >> morphology.producer) +
     insert(" ") +
     (insert("100") >> morphology.producer) +
     insert(" ") +
     num_2digits)
).optimize()

num_3digits = (
    remove("0") + num_2digits |
    num_100_999
).optimize()

def higher_group(zeros):
    return (
        remove(ss("0")) +
        (("1" + insert(zeros) + maybe_tail >> morphology.producer) |
         (((digit_but_01 >> morphology.producer) |
           (rr(g.digit, 2) >> num_10_99) |
           (rr(g.digit, 3) >> num_100_999)) +
          insert(" ") +
          (insert("1" + zeros) + maybe_tail >> morphology.producer)))
    ).optimize()

thousands_group = higher_group("000")
millions_group = higher_group("000000")
billions_group = higher_group("000000000")

num_small = num_1_9 | num_10_99 | num_100_999

group_separator = replace(qq("." | cost(",", 0.1)), " ")

num_upto_thousands = (
    num_small |
    (rr(g.digit, 1, 3) + remove(group_separator + "000") + maybe_tail >> thousands_group) |
    ((rr(g.digit, 1, 3) >> thousands_group) +
     group_separator +
     (rr(g.digit, 3) + maybe_tail >> remove(ss("0")) + num_small))
).optimize()

num_upto_millions = (
    num_upto_thousands |
    ((rr(g.digit, 1, 3) +
      remove(group_separator +"000" + group_separator + "000") +
      maybe_tail) >>
     millions_group) |
    ((rr(g.digit, 1, 3) >> millions_group) +
     group_separator +
     (rr(g.digit, 3) + qq(".") +
      rr(g.digit, 3) + maybe_tail >> remove(ss(anyof("0 "))) + num_upto_thousands))
).optimize()

num_upto_billions = (
    num_upto_millions |
    ((rr(g.digit, 1, 3) +
      remove(group_separator + "000" + group_separator + "000" + group_separator + "000") +
      maybe_tail) >>
     billions_group) |
    ((rr(g.digit, 1, 3) >> billions_group) +
     group_separator +
     (rr(g.digit, 3) + qq(".") +
      rr(g.digit, 3) + qq(".") +
      rr(g.digit, 3) + maybe_tail >> remove(ss(anyof("0 "))) + num_upto_millions))
).optimize()

three_digits = (
    rr(g.digit, 3) >>
    (num_100_999 |
     ("0" >> morphology.producer) + insert(" ") + num_10_99 |
     rr("0" >> morphology.producer + insert(" "), 2) + num_1_9 |
     rr("0" >> morphology.producer + insert(" "), 3))
).optimize()

two_digits = (
    rr(g.digit, 2) >>
    (num_10_99 |
     ("0" >> morphology.producer) + insert(" ") + num_1_9 |
     rr("0" >> morphology.producer + insert(" "), 2))
).optimize()

# three_digits for the first group when the total sequence is of odd length
long_digit_sequence = (
    rr(g.digit, 13) + ss(g.digit) >>
    qq(three_digits) + pp(insert(" ") + two_digits)
).optimize()

integer = (
    remove(ss("0")) +
    (num_0 |
     num_upto_billions |
     long_digit_sequence)
).optimize()

fraction = (
    (pp(g.digit | ".") >> integer) +
    # Put a penalty on "." so that its preferred interpretation is group separator
    (replace(",", " virgül ") | cost(replace(".", " nokta "), 0.1)) +
    # The maybe_tail part can only occur after the last digit, of course.
    pp(insert(" ") + (g.digit + maybe_tail >> morphology.producer))
).optimize()

plus_minus = (
    replace("+", " artı ") |
    replace("-", " eksi ")
)

num = (
    qq(plus_minus) +
    (integer | fraction) +
    qq(qq(remove("'")) + pp(g.letter))
).optimize()

cvt = convert_words(num)
