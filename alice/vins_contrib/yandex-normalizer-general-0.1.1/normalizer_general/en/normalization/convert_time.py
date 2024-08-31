#! /usr/bin/env python2
# encoding: utf-8
import sys
sys.path.append('..')
sys.path.append('../..')

from general.normbase import *

convert_time = (word(qq(remove("0")) + g.digit | rr(g.digit, 2),
                     permit_inner_space=True,
                     need_outer_space=False) +
                qq(" ") + remove(":") + qq(" ") +
                (replace("00", " o'clock ") | 
                 word(rr(g.digit, 2),
                      permit_inner_space=True,
                      need_outer_space=False)))
convert_time = convert_words(convert_time, permit_inner_space=True).optimize()
