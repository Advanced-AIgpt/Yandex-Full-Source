#! /usr/bin/env python2
# encoding: utf-8
import sys
sys.path.append('..')
sys.path.append('../..')

from general.normbase import *
from numerals import *
import numbers as n

print 'Date'

month_to_number = anyof([replace(mname, '%02d' % num)
                                 for mname, num in [("січня", 1),
                                                    ("лютого", 2),
                                                    ("березня", 3),
                                                    ("квітня", 4),
                                                    ("травня", 5),
                                                    ("червня", 6),
                                                    ("липня", 7),
                                                    ("серпня", 8),
                                                    ("вересня", 9),
                                                    ("жовтня", 10),
                                                    ("листопада", 11),
                                                    ("грудня", 12),
                                                    ]])

date_cvt = glue_words((g.digit | insert("0")) + g.digit + remove("-" + ss(g.letter) + feats('numeral', 'ord', 'neu')),
                      qq(remove("числа")),
                      insert("."),
                      month_to_number | ("1" | insert("0")) + g.digit + remove("-" + ss(g.letter) + feats('numeral', 'ord', 'neu', 'gen')),
                      qq(remove("місяця")),
                      insert("."),
                      rr(g.digit, 4) + remove("-" + ss(g.letter) + feats('numeral', 'ord', 'gen', 'mas')),
                      qq(remove("року")),
                      permit_inner_space=True)
date_cvt = date_cvt.optimize()

if __name__ == '__main__':
    qq_pre = (insert(" ") + ss(g.any_symb)) >> ss((pp(" " + tagger) >> n.ordinal) | cost(word(ss(g.any_symb))))
    qq_pre = qq_pre.optimize()

    date_full = qq_pre >> date_cvt

    print 'date_full', date_full

    date_full = date_full.optimize()

    print 'date_full, optimized', date_full

    fst_save(date_full, "date")
