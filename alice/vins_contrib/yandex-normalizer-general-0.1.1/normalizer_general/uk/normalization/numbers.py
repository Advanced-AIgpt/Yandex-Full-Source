#! /usr/bin/env python2
# encoding: utf-8
import sys
sys.path.append('..')
sys.path.append('../..')

import general.normbase
from general.normbase import g, anyof, pp, ss, convert_words
import categories
import by_character

# For now, read all the numbers digit-wise

all_numbers = (g.digit + ss(g.digit | anyof('.,'))) >> ss(by_character.replacer)

numbers_cvt = convert_words(all_numbers)
numbers_cvt = numbers_cvt.optimize()


if __name__ == '__main__':
    from general.normbase import fst_print_random_sample
    fst_print_random_sample(all_numbers)
