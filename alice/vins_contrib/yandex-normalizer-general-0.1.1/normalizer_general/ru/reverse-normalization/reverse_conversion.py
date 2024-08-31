#! /usr/bin/env python2
# encoding: utf-8
import sys
sys.path.append('..')
sys.path.append('../..')

from general.normbase import *
import morphology as num
import number_conversions as n
import time_cvt as t
import date_cvt as d
import phone_cvt as p
import antimat_cvt as a
import substitution_cvt as s
import punctuation_cvt as punct

import os
import shutil

print 'Fullchain'

# Preprocessing: add " " at start so that word() works uniformly
space_at_start = insert(" ") + ss(g.any_symb)
space_at_start = space_at_start.optimize()

remove_unk = remove("<UNK>")
remove_unk = convert_words(remove_unk)

## A group of steps that involve number conversion

# Convert numbers
numbers_step = ss(cost((pp(word(num.tagger)) >> n.number), 0.01) | cost(word(ss(g.any_symb)), 1))
numbers_step = numbers_step.optimize()

# Phones take priority over dates and times
phone_step = ss(cost(p.phone_cvt, 0.01) | cost(word(ss(g.any_symb)), 1))
phones = phone_step.optimize()

date_step = ss(cost(d.date_cvt, 0.01) | cost(word(ss(g.any_symb)), 1))
dates = date_step.optimize()

time_step = ss(cost(t.time_cvt, 0.01) | cost(word(ss(g.any_symb)), 1))
times = time_step.optimize()

# Search for dates, times.
#disj_step = ss(cost(d.date_cvt | t.time_cvt, 0.01) | cost(word(ss(g.any_symb)), 1))
#disj_step = disj_step.optimize()

# Postprocess: remove grammar markers
postprocess = drop_output_nonterminals(ss(g.any_symb))
postprocess = postprocess.optimize()

numbers = (
    numbers_step >>
#    phone_step >>
#    disj_step >>
    postprocess
)
#print 'numbers', numbers
numbers = numbers.optimize()
#print 'numbers, optimized', numbers

# A special case: миллионная улица, to be wrapped up together with number_sequence
# (Too small to set up a special step for that)
endings = (
    replace("-я", "ая") |
    replace("-ой", "ой") |
    replace("-ю", "ую")
)

million_replace = (
    replace("1000000", "миллионн") + endings
)

million_tested = (
    "улиц" + pp(g.letter) + word(million_replace) |
    million_replace + word("улиц" + pp(g.letter))
)

number_sequence = (pp(g.digit) +
                   (replace(word("точка"), ".") |
                    replace(word("запятая"), ",")) +
                   pp(remove(ss(" ")) + pp(g.digit)))
number_sequence = convert_words(number_sequence | million_tested, permit_inner_space=True)

## Group of steps that involve substitution

simplify_url = word(s.simplify_url_cvt, permit_inner_space=True)
substitution = word(s.substitution_cvt, permit_inner_space=True)
url_substitution = word(s.url_substitution_cvt, permit_inner_space=True)

# Simplify_url should be applied before substituton
simplify_url_step = ss(simplify_url | cost(word(ss(g.any_symb)), 0.001))
simplify_url_step = simplify_url_step.optimize()

substitution_step = ss(cost(substitution, 0.1) | cost(word(ss(g.any_symb)), 1))
substitution_step = substitution_step.optimize()

url_substitution_step = ss(url_substitution | cost(word(ss(g.any_symb)), 1)) + url_substitution + ss(url_substitution | cost(word(ss(g.any_symb)), 1))
url_substitution_step = url_substitution_step.optimize()

substitution_group = (substitution_step | (simplify_url_step >> url_substitution_step)).optimize()

# Make profanity removal a separate stage, so it is easy to skip
profanity = ss(cost(a.antimat_cvt, 0.01) | cost(word(ss(g.any_symb)), 1))
profanity = profanity.optimize()

remove_space_at_start = pp(remove(" ")) + ss(g.any_symb)
