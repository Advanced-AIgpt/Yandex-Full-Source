#! /usr/bin/env python2
# encoding: utf-8
import sys
import os
sys.path.append(os.path.dirname(os.path.abspath(__file__)))
sys.path.append('..')
sys.path.append('../..')

from general.normbase import *
import categories
import number_conversions as n
import case_control

print "Times"

time_pattern = (((qq(anyof("01")) + g.digit) | ("2" + anyof("0123"))) + ":" +
                anyof("012345") + g.digit +
                qq(":" + anyof("012345") + g.digit))

maybe_dot = qq(remove(ss(" ") + "."))

def make_time(case):
    return ((time_pattern >> (n.unit_with_case("час", 'mas', case) +
                              replace(":", " ") +
                              (replace("00" + qq(":00"), "ровно") |
                               cost(n.unit_with_case("минут", 'fem', case) +
                                    qq(replace(":", " ") +
                                       n.unit_with_case("секунд", 'fem', case)), 0.001))) |
             (n.unit_with_case("час", 'mas', case) +
              remove(word("ч" + maybe_dot, need_outer_space=False, permit_inner_space=True)) +
              qq(word(n.unit_with_case("минут", 'fem', case)) +
                 remove(word("мин" + maybe_dot, need_outer_space=False, permit_inner_space=True)) +
                 qq(word(n.unit_with_case("секунд", 'fem', case)) +
                    remove(word("с" + qq("ек") + maybe_dot, need_outer_space=False, permit_inner_space=True)))))) +
            # Add request for genitive in case there is a date right after us
            insert(" #" + 'gen'))

# Never generate Locative, in particular, with 'в'
time = cost(case_control.use_mark(make_time, ['nom', 'gen', 'dat', 'acc', 'instr'], permit_inner_space=True), 0.0001)
time_range = insert(" с ") + make_time("gen") + replace(qq(" ") + "-" + qq(" "), " до ") + make_time("gen")
#((time_pattern >> make_time('nom')) |
#        anyof([(remove("#" + case + pp(" ")) + make_time(case))
#               for case in ['nom', 'gen', 'dat', 'acc', 'instr']]))
#time = time.optimize()

convert_time = convert_words(time_range | time, permit_inner_space=True).optimize()

