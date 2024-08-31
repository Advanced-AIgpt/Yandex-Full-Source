#! /usr/bin/env python2
# encoding: utf-8
import sys
sys.path.append('..')
sys.path.append('../..')

from general.normbase import *
import morphology

print "Case to features"

# Should I derive this from morphology.py?
CASE_ENDINGS = [
    (u"ın", 'gen'),
    (u"nın", 'gen'),
    (u"in", 'gen'),
    (u"nin", 'gen'),
    (u"un", 'gen'),
    (u"nun", 'gen'),
    (u"ün", 'gen'),
    (u"nün", 'gen'),

    (u"ı", 'acc'),
    (u"yı", 'acc'),
    (u"i", 'acc'),
    (u"yi", 'acc'),
    (u"u", 'acc'),
    (u"yu", 'acc'),
    (u"ü", 'acc'),
    (u"yü", 'acc'),

    (u"a", 'dat'),
    (u"ya", 'dat'),
    (u"e", 'dat'),
    (u"ye", 'dat'),

    (u"da", 'loc'),
    (u"ta", 'loc'),
    (u"de", 'loc'),
    (u"te", 'loc'),

    (u"dan", 'abl'),
    (u"tan", 'abl'),
    (u"den", 'abl'),
    (u"ten", 'abl'),

    (u"la", 'comit'),
    (u"yla", 'comit'),
    (u"le", 'comit'),
    (u"yle", 'comit'),
]

case_to_feats = (
    pp(g.digit | anyof(".,")) +
    (qq(remove("'")) + anyof(replace(ending, feats('numeral', 'card', case))
                             for ending, case in CASE_ENDINGS) |
     "." + qq(remove("'")) + anyof(replace(ending, feats('numeral', 'ord', case))
                                   for ending, case in CASE_ENDINGS))
).optimize()

cvt = convert_words(case_to_feats)

