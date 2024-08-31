#! /usr/bin/env python2
# encoding: utf-8
import sys
sys.path.append('..')
sys.path.append('../..')

from general.normbase import *
import categories

print 'Telephone numbers'

num_nom_feats = feats('numeral', 'card', 'nom', 'mas')

# This trasducer is called when spaces around punctuation marks are already inserted,
# so it has to take care of them (in particular, around parentheses).

# We need at least some telephone marker -- either "+" at tthe start, or at least one of "()" in the middle or preceeding context such as "телефон" or "номер".
telephone_context = anyof(["телефон", "номер"]) + qq(anyof(["а", "у"]))

telephone_pre_check = (
    "+" + pp(g.digit | anyof("()- ")) |
    ss(g.digit) + anyof("() ") + pp(g.digit | anyof("() ")) |
    telephone_context + pp(g.digit | anyof("+()- "))
).optimize()

telephone_main = (
    qq(telephone_context + qq(" ")) +
    qq(qq(qq("+") + rr(g.digit, 1, 3) + insert(num_nom_feats)) +
       qq(remove(ss(" ") + anyof(["(", "-"]) + ss(" "))) + insert(" ") +
       # Prefer three-digit area codes
       (rr(g.digit, 3) + insert(num_nom_feats) |
        cost(rr(g.digit, 1, 2) + insert(num_nom_feats), 0.01)) +
       qq(remove(ss(" ") + anyof([")", "-"]) + ss(" "))) + insert(" ")) +
    # Prefer 7-digit numbers; helps when there are no spacers
    (rr(g.digit, 3) + insert(num_nom_feats) |
     cost(rr(g.digit, 2) + insert(num_nom_feats), 0.01)) + replace(qq("-"), " ") +
    rr(g.digit, 2) + insert(num_nom_feats) + replace(qq("-"), " ") +
    rr(g.digit, 2) + insert(num_nom_feats)
).optimize()

telephone = (telephone_pre_check >> telephone_main).optimize()


convert_telephone = convert_words(telephone, permit_inner_space=True)

