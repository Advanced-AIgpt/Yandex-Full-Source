#! /usr/bin/env python2
# encoding: utf-8
import sys
sys.path.append('..')
sys.path.append('../..')

from general.normbase import *
import numerals as num
import numbers as n
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

## A group of steps that involve number conversion

# Convert numbers
numbers_step = ss(cost((pp(pp(" ") + num.tagger) >> n.number), 0.01) | cost(word(ss(g.any_symb)), 1))
numbers_step = numbers_step.optimize()

# Phones take priority over dates and times
phone_step = ss(cost(p.phone_cvt, 0.01) | cost(word(ss(g.any_symb)), 1))
phone_step = phone_step.optimize()

# Search for dates, times.
disj_step = ss(cost(d.date_cvt | t.time_cvt, 0.01) | cost(word(ss(g.any_symb)), 1))
disj_step = disj_step.optimize()

# Postprocess: remove grammar markers
postprocess = drop_output_nonterminals(ss(g.any_symb))
postprocess = postprocess.optimize()
numbers_dates_times_phones = ( numbers_step >> phone_step >> disj_step >> postprocess )
print 'numbers_dates_times_phones', numbers_dates_times_phones
numbers_dates_times_phones = numbers_dates_times_phones.optimize()
print 'numbers_dates_times_phones, optimized', numbers_dates_times_phones


## Group of steps that involve substitution

simplify_url = word(s.simplify_url_cvt, permit_inner_space=True)
substitution = word(s.substitution_cvt, permit_inner_space=True)

# Simplify_url should be applied before substituton
simplify_url_step = ss(cost(simplify_url, 0.01) | cost(word(ss(g.any_symb)), 1))
simplify_url_step = simplify_url_step.optimize()

substitution_step = ss(cost(substitution, 0.01) | cost(word(ss(g.any_symb)), 1))
substitution_step = substitution_step.optimize()

substitution_group = simplify_url_step >> substitution_step

# Make profanity removal a separate stage, so it is easy to skip
profanity = ss(cost(a.antimat_cvt, 0.01) | cost(word(ss(g.any_symb)), 1))
profanity = profanity.optimize()

remove_space_at_start = remove(pp(" ")) + ss(g.any_symb)

shutil.rmtree("revnorm", ignore_errors=True)
os.mkdir("revnorm")
fst_syms_save("revnorm/symbols")
with open("revnorm/sequence.txt", "w") as of:
    fst_save(space_at_start, "revnorm/space_at_start")
    print >>of, "space_at_start"

    fst_save(substitution_group, "revnorm/substitution")
    print >>of, "substitution"

    fst_save(numbers_dates_times_phones, "revnorm/numbers_dates_times_phones")
    print >>of, "numbers_dates_times_phones"

    fst_save(profanity, "revnorm/profanity")
    print >>of, "profanity"

    fst_save(remove_space_at_start, "revnorm/remove_space_at_start")
    print >>of, "remove_space_at_start"

    fst_save(punct.punctuation_cvt, "revnorm/punctuation")
    print >>of, "punctuation"
