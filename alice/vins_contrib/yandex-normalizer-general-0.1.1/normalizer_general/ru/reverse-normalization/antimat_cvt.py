#! /usr/bin/env python2
# encoding: utf-8
import sys
sys.path.append('..')
sys.path.append('../..')

from general.normbase import *
from morphology import *

print 'Antimat'

antimat_cvt = word(replace(word_list_from_file(os.path.join(os.path.dirname(os.path.abspath(__file__)), "antimat.txt")), "<censored>"))
antimat_cvt = antimat_cvt.optimize()

if __name__ == '__main__':
    qq_pre = (insert(" ") + ss(g.any_symb))
    qq_pre.remove_epsilon()
    qq_pre.arc_sort_output()

    antimat_full = qq_pre >> ss(antimat_cvt | cost(word(ss(g.any_symb)), 1))

    print antimat_full

    fst_save(antimat_full, "antimat")
