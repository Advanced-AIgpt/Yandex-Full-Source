#! /usr/bin/env python2
# encoding: utf-8

import sys
sys.path.append('..')

from general.normbase import Fst, pp, qq, replace, remove, anyof, word, tagger, direct_tagger, feats, fst_render
import categories  # NOQA - importing for the side effects

print >> sys.stderr, 'Numerals'

number_renditions = {
    0: Fst('zero') | 'oh',
    1: 'one',
    2: 'two',
    3: 'three',
    4: 'four',
    5: 'five',
    6: 'six',
    7: 'seven',
    8: 'eight',
    9: 'nine',
    10: 'ten',
    11: 'eleven',
    12: 'twelve',
    100: 'hundred',
    1000: 'thousand',
    1000000: 'million',
    1000000000: 'billion',
}

# Cardinals

cardinal_numeral = feats('numeral', 'card')

direct_replacers = {}
card_direct_taggers = {}

for number, name in number_renditions.iteritems():
    replacer = direct_replacers[number] = replace(name, unicode(number))
    card_direct_taggers[number] = direct_tagger((replacer, cardinal_numeral))

teens = {
    13: 'thir',
    14: 'four',
    15: 'fif',
    16: 'six',
    17: 'seven',
    18: 'eigh',
    19: 'nine',
}
stems_13_19 = [replace(name, unicode(number)) for number, name in teens.iteritems()]
card_13_19 = anyof(stems_13_19) + remove('teen')
card_13_19_tagger = direct_tagger((card_13_19, cardinal_numeral))

tens = {
    20: 'twen',
    30: 'thir',
    40: 'for',
    50: 'fif',
    60: 'six',
    70: 'seven',
    80: 'eigh',
    90: 'nine',
}
stems_20_90 = [replace(name, unicode(number)) for number, name in tens.iteritems()]
card_20_90 = anyof(stems_20_90) + remove('ty')
card_20_90_tagger = direct_tagger((card_20_90, cardinal_numeral))

card_tagger = (
    anyof(card_direct_taggers.itervalues()) |
    card_13_19_tagger |
    card_20_90_tagger
)

# Ordinals

ordinal_numeral = feats('numeral', 'ord')

ord_direct_renditions = {
    0: 'zeroth',  # to avoid forming 'ohth' in the common case
    1: 'first',
    2: 'second',
    3: 'third',
}

ord_direct_taggers = {
    number: direct_tagger((replace(name, unicode(number)), ordinal_numeral))
    for number, name in ord_direct_renditions.iteritems()
}

ord_custom_stems_4_onwards = {
    5: 'fif',
    8: 'eigh',
    9: 'nin',
    12: 'twelf',
}

ord_stems_20_90 = anyof(stems_20_90) + remove('tie')
ord_stems_4_onwards = [
    (
        replace(ord_custom_stems_4_onwards[number], unicode(number))
        if number in ord_custom_stems_4_onwards
        else direct_replacer
    )
    for number, direct_replacer in direct_replacers.iteritems()
    if number not in ord_direct_renditions
] + [
    card_13_19,
    ord_stems_20_90,
]
ord_flex_4_onwards = [('th', ordinal_numeral)]
ord_4_onwards_and_0_tagger = tagger(ord_stems_4_onwards, ord_flex_4_onwards)

ord_tagger = anyof(ord_direct_taggers.itervalues()) | ord_4_onwards_and_0_tagger

# In the British English, 'and' is inserted before tens and units
conjunction = 'and'

tagger = card_tagger | ord_tagger
tagger = tagger.optimize()

producer = tagger.inverse()
producer = producer.optimize()

# We ignore single 'and's, should we also ignore single 'oh's?
word_seq_tagger = pp(word(tagger)) + qq(word(conjunction) + pp(word(tagger)))
word_seq_tagger = word_seq_tagger.optimize()

if __name__ == '__main__':
    fst_render(tagger, 'numerals-tagger')
