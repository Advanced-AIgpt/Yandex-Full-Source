#! /usr/bin/env python2
# encoding: utf-8
import sys
sys.path.append('..')
sys.path.append('../..')

from general.normbase import *
import substitution_cvt
import antimat_cvt

# Preprocessing: add " " at the start so that word() works uniformly
space_at_start = insert(" ") + ss(g.any_symb)
space_at_start = space_at_start.optimize()

remove_space_at_start = remove(" ") + ss(g.any_symb)

## Group of steps that involve substitution

simplify_url = word(substitution_cvt.simplify_url_cvt, permit_inner_space=True)
substitution = word(substitution_cvt.substitution_cvt, permit_inner_space=True)

# Simplify_url should be applied before substituton
simplify_url_step = ss(cost(simplify_url, 0.01) | cost(word(ss(g.any_symb)), 1))
simplify_url_step = simplify_url_step.optimize()

substitution_step = ss(cost(substitution, 0.01) | cost(word(ss(g.any_symb)), 1))
substitution_step = substitution_step.optimize()

substitution_group = simplify_url_step >> substitution_step

join_abbrev = g.letter + pp(remove(pp(" ")) + g.letter)
join_abbrev_cvt = convert_words(join_abbrev, permit_inner_space=True)

# Make profanity removal a separate stage, so it is easy to skip
profanity = ss(cost(antimat_cvt.antimat_cvt, 0.01) | cost(word(ss(g.any_symb)), 1))
profanity = profanity.optimize()
