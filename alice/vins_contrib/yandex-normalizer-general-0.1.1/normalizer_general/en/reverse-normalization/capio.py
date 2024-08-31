#! /usr/bin/env python2
# encoding: utf-8
import sys
sys.path.append('..')
sys.path.append('../..')

from general.normbase import insert, remove, ss, word, drop_output_nonterminals, cost, g, fst_syms_save, fst_save, convert_symbols

print >> sys.stderr, 'Capio'

intersperse_digits_with_spaces = g.digit + ss(insert(" ") + g.digit)
intersperse_digits_with_spaces_cvt = convert_symbols(intersperse_digits_with_spaces)
