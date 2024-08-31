#! /usr/bin/env python2
# encoding: utf-8
import sys
sys.path.append('..')
sys.path.append('../..')

from general.normbase import *
import categories

import by_character
import chem

print 'Special codes in general'

ignore_unknown = (remove("#[" + pp(g.letter) + "|") +
                  ss(g.any_symb - "]") +
                  remove("]#"))

special_codes = convert_words((by_character.handle |
                               chem.handle |
                               cost(ignore_unknown, 0.1)),
                               f_cost=0.0,
                               permit_inner_space=True,
                               need_outer_space=False)
