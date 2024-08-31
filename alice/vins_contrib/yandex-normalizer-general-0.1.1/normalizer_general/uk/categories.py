#! /usr/bin/env python2
# encoding: utf-8
import sys
sys.path.append('..')

from general.normbase import *

# We define categories, grammemes and parts of speech (grammemes will extend our alphabet)

print 'Categories'

# Categories
category('numkind', 'ord', 'card')
category('case', 'nom', 'gen', 'dat', 'acc', 'instr', 'loc')
# For numerals, plural behaves as just another gender
category('gender', 'mas', 'neu', 'fem', 'pl')
# For nouns, number is a true category
category('number', 'sg', 'pl')

pos('numeral', 'numkind', 'case', 'gender')
pos('noun', 'case', 'number')

# Now we register the extended alphabet
categories_defined()
