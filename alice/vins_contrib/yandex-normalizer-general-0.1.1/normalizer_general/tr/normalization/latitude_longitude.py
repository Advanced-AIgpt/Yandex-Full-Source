#! /usr/bin/env python2
# encoding: utf-8
import sys
sys.path.append('..')
sys.path.append('../..')

from general.normbase import *
import categories

print 'Latitude, lognitude'

latlon = (
    qq(word(qq("-") + pp(g.digit) + qq("." + pp(g.digit)) +
            replace("°", " derece"))) +
    qq(word(qq("-") + pp(g.digit) + qq("." + pp(g.digit)) +
            replace("'", " dakika"))) +
    qq(word(qq("-") + pp(g.digit) + qq("." + pp(g.digit)) +
            replace("''", " saniye")))
)

cvt = convert_words(latlon, permit_inner_space=True, need_outer_space=False)



