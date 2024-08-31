#! /usr/bin/env python2
# encoding: utf-8
import sys
sys.path.append('..')

from general.normbase import *

print 'Categories'

category('numkind', 'ord', 'card') # distributive?
category('case', 'nom', 'gen', 'acc', 'dat', 'loc', 'abl', 'comit')

pos('numeral', 'numkind', 'case')

categories_defined()

# "'" is not a letter in Turkish (got into the list because of Ukrainian)
g.letter = g.letter - "'"
