#! /usr/bin/env python
# encoding: utf-8

import sys
sys.path.append('..')
sys.path.append('../..')

from general.normbase import *
from morphology import *
import categories
import time_cvt
import case_control

print "Sport scores"

make_score = pp(g.digit) + ss(" ") + replace(":", " ") + ss(" ") + pp(g.digit)

score_mark = word(check_form("счёт", feats("noun")) | check_form("счет", feats("noun")), need_outer_space=False)

expand_score = convert_words((score_mark + pp(" ") + make_score) | cost(time_cvt.time_pattern, 0.001) | cost(make_score, 0.01), permit_inner_space=True).optimize()
