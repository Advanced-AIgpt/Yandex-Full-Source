#! /usr/bin/env python2
# encoding: utf-8
import sys
sys.path.append('..')
sys.path.append('../..')

from general.normbase import *
import morphology as m
import numbers as n

import os
import shutil

print 'Fullchain'

# Preprocessing: add " " at start so that word() works uniformly
space_at_start = insert(" ") + ss(g.any_symb)
space_at_start = space_at_start.optimize()

# Convert numbers
numbers_step = ss(cost(pp(word(m.tagger, need_outer_space=False)) >> n.number, 0.01) | cost(word(ss(g.any_symb)), 1))
numbers_step = numbers_step.optimize()

remove_space_at_start = remove(" ") + ss(g.any_symb)

shutil.rmtree("revnorm", ignore_errors=True)
os.mkdir("revnorm")

fst_syms_save("revnorm/symbols")

with open("revnorm/sequence.txt", "w") as of:
    fst_save(space_at_start, "revnorm/space_at_start")
    print >>of, "space_at_start"

    fst_save(numbers_step, "revnorm/numbers")
    print >>of, "numbers"

    fst_save(remove_space_at_start, "revnorm/remove_space_at_start")
    print >>of, "remove_space_at_start"

with open("revnorm/flags.txt", "w") as ff:
    print >>ff, "report-intermediate false"
