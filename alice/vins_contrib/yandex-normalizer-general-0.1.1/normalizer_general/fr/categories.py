#! /usr/bin/env python2
# encoding: utf-8
import sys
sys.path.append('..')

from general.normbase import *

# We define categories, grammemes and parts of speech (grammemes will extend our alphabet)

print 'Categories'

# Categories
category('numkind', 'ord', 'card')
# For numerals and adjectives, plural behaves as just another gender
category('gender', 'mas', 'fem')
# For nouns, number is a true category
category('number', 'sg', 'pl')

pos('numeral', 'numkind', 'gender', 'number') # number only for ordinals
pos('noun', 'number')
pos('adjective', 'number', 'gender')

# Now we register the extended alphabet
categories_defined()
