#! /usr/bin/env python2
# encoding: utf-8

import sys
sys.path.append('..')

from general.normbase import Fst, pp, qq, empty, failure, \
              replace, remove, anyof, word, tagger, direct_tagger, feats, fst_render, g, cost
import categories  # NOQA - importing for the side effects

print >> sys.stderr, "Numerals"

def utf(s):
    if isinstance(s,unicode):
        return s
    else:
        return unicode(s, 'utf-8')

# Accept either the argument or (with a small penalty)
# argument with diacritics removed.
def maybe_diacr(s):
    s = utf(s)
    r = empty()
    for c in s:
        if c in u"áàâä":
            cc = c | cost(u"a", 0.0001)
        elif c in u"éèêë":
            cc = c | cost(u"e", 0.0001)
        elif c in u"íìîï":
            cc = c | cost(u"i", 0.0001)
        elif c in u"óòôö":
            cc = c | cost(u"o", 0.0001)
        elif c in u"úùûü":
            cc = c | cost(u"u", 0.0001)
        else:
            cc = c
        r += cc
    return r


cardinal_feats = feats('numeral', 'card')

cardinal_renditions = {
      0: "zéro",
      2: "deux",
      3: "trois",
      4: "quatre",
      5: "cinq",
      6: "six",
      7: "sept",
      8: "huit",
      9: "neuf",
     10: "dix",
     11: "onze",
     12: "douze",
     13: "treize",
     14: "quatorze",
     15: "quinze",
     16: "seize",
     20: "vingt",
     30: "trente",
     40: "quarante",
     50: "cinquante",
     60: "soixante",
     # 70-90 are variants of norm
     70: "septante",
     80: "huitante",
     90: "nonante",
    100: "cent",
   1000: "mille",
   1000000: "million",
   1000000000: "milliard",
}

card_1_tagger = (replace("un", "1" + feats('numeral', 'card', 'mas', 'sg')) |
                 replace("une", "1" + feats('numeral', 'card', 'fem', 'sg')))

card_word_tagger = failure()
for number in cardinal_renditions:
    name = cardinal_renditions[number]
    replacer = replace(maybe_diacr(name), unicode(number) + cardinal_feats)
    card_word_tagger |= replacer

card_tagger = card_1_tagger | card_word_tagger


ordinal_feats = feats('numeral', 'ord')

ord_1_tagger = (
    replace("premier", "1" + feats('numeral', 'ord', 'mas', 'sg')) |
    replace(maybe_diacr("première"), "1" + feats('numeral', 'ord', 'fem', 'sg')) |
    replace("premiers", "1" + feats('numeral', 'ord', 'mas', 'pl')) |
    replace(maybe_diacr("premières"), "1" + feats('numeral', 'ord', 'fem', 'pl')) |
    # Mark the "1" after tens by a special mark.
    replace(maybe_diacr("unième"), "!1" + feats('numeral', 'ord', 'sg')) |
    replace(maybe_diacr("unièmes"), "!1" + feats('numeral', 'ord', 'pl'))
)

ordinal_renditions = {
#      0: "zeroème",
      2: "deuxième",
      3: "troisième",
      4: "quatrième",
      5: "cinquième",
      6: "sixième",
      7: "septième",
      8: "huitième",
      9: "neuvième",
     10: "dixième",
     11: "onzième",
     12: "douzième",
     13: "treizième",
     14: "quatorzième",
     15: "quinzième",
     16: "seizième",
     20: "vingtième",
     30: "trentième",
     40: "quarantième",
     50: "cinquantième",
     60: "soixantième",
     # 70-90 are variants of norm
     70: "septantième",
     80: "huitantième",
     90: "nonantième",
    100: "centième",
    1000: "millième",
    1000000: "millionième",
    1000000000: "milliardième",
}

ord_word_tagger = failure()
for number in ordinal_renditions:
    name = ordinal_renditions[number]
    replacer = (replace(maybe_diacr(name), unicode(number) + feats('numeral', 'ord', 'sg')) |
                replace(maybe_diacr(name + "s"), unicode(number) + feats('numeral', 'ord', 'pl')))
    ord_word_tagger |= replacer

ord_tagger = ord_1_tagger | ord_word_tagger

tagger = card_tagger | ord_tagger

producer = tagger.invert()

tagger = tagger.optimize()
producer = producer.optimize()
