#! /usr/bin/env python2
# encoding: utf-8
import sys
sys.path.append('..')
sys.path.append('../..')

from general.normbase import *
from morphology import *

print 'Case control marking'

# Words that control the next NP's case leave a special 'word' with that case after themselves,
# which the next NP can pick up in its own rules.
# These special 'words' are destroyed at the end of normalization.

CONTEXT_CASE_CONTROL = {
  "на": 'acc',
  "в": 'loc',
  "во": 'loc',
  "с": 'gen',
  "со": 'gen',
  "от": 'gen',
  "до": 'gen',
  "из": 'gen',
  "раньше": 'gen',
  "позже": 'gen',
  "после": 'gen',
  "больше": 'gen',
  "меньше": 'gen',
  "более": 'gen',
  "менее": 'gen',
  "размере": 'gen',
  "течение": 'gen',
  "течении": 'gen', # Frequent error
  "протяжении": 'gen',
  "возрасте": 'gen',
  "к": 'dat',
  "ко": 'dat',
  "по": 'acc',
  "за": 'acc',
  "через": 'acc',
  "включая": 'acc',
  "исключая": 'acc',
  "около": 'gen',
  "равен": 'dat',
  "равна": 'dat',
  "равно": 'dat',
  "равны": 'dat',
  "равняется": 'dat',
  "равняются": 'dat',
  "равнялся": 'dat',
  "равнялась": 'dat',
  "равнялось": 'dat',
  "равнялись": 'dat',
  "свыше": 'gen',
  "выше": 'gen',
  "составляет": 'acc',
  "составляют": 'acc',
  "составил": 'acc',
  "составило": 'acc',
  "составила": 'acc',
  "составили": 'acc',
  "составлял": 'acc',
  "составляло": 'acc',
  "составляла": 'acc',
  "составляли": 'acc',
  "превышает": 'acc',
  "превышают": 'acc',
  "достигает": 'gen',
  "достигают": 'gen',
  # For dates
  "родился": 'gen',
  "родилась": 'gen',
  "родились": 'gen',
  "умер": 'gen',
  "умерла": 'gen',
  "умерли": 'gen',
  "скончался": 'gen',
  "скончалась": 'gen',
  "скончались": 'gen',
  "состоялся": 'gen',
  "состоялась": 'gen',
  "состоялось": 'gen',
  "состоялись": 'gen',
  "состоится": 'gen',
  "состоятся": 'gen',
  "выпущена": 'gen',
  "выпущен": 'gen',
  "старше": 'gen',
  "младше": 'gen',
  "ниже": 'gen',
}

insert_marks = anyof([(ww + insert(" " + "#" + CONTEXT_CASE_CONTROL[ww]))
                      for ww in CONTEXT_CASE_CONTROL.keys()])
insert_control_marks = convert_words(insert_marks)

def use_mark(converter, case_list=None, default_case='nom', **kwargs):
    """Apply mark to the next pattern; use converter('nom') when no mark is present"""
    if case_list == None:
        case_list = cat_values('case')
    r = (converter(default_case) |
         anyof((remove("#" + case) + word(converter(case), **kwargs))
               for case in case_list) |
         # Slightly penalize oblique cases without explicit marks
         cost(anyof(converter(case)
                    for case in case_list),
              0.01))
    return r.optimize()

remove_marks = remove("#" + anyof(cat_values('case')))
remove_control_marks = convert_words(remove_marks, need_outer_space=False)
