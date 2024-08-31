#! /usr/bin/env python2
# encoding: utf-8
import sys
sys.path.append('..')

from general.normbase import category, pos, categories_defined

# We define categories, grammemes and parts of speech (grammemes will extend our alphabet)

print >>sys.stderr, 'Categories'

# Categories
category('numkind', 'ord', 'card')

pos('numeral', 'numkind')

# Now we register the extended alphabet
categories_defined()
