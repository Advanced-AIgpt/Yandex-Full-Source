#! /usr/bin/env python2
# encoding: utf-8
import sys
sys.path.append('..')
sys.path.append('../..')

from general.normbase import *
from morphology import *
import number_conversions as n

print 'Date'

month_to_number = anyof([replace_stressed(mname, '%02d' % num)
                                          for mname, num in [("январ+я", 1),
                                                            ("феврал+я", 2),
                                                            ("м+арта", 3),
                                                            ("апр+еля", 4),
                                                            ("м+ая", 5),
                                                            ("и+юня", 6),
                                                            ("и+юля", 7),
                                                            ("+августа", 8),
                                                            ("сентябр+я", 9),
                                                            ("октябр+я", 10),
                                                            ("ноябр+я", 11),
                                                            ("декабр+я", 12),
                                                            ]])

date_cvt = glue_words((g.digit | insert("0")) + g.digit + remove("-" + ss(g.letter) + qq(feats('numeral', 'ord', 'neu'))),
                      qq(remove(maybe_stressed("числ+а"))),
                      insert("."),
                      month_to_number | ("1" | insert("0")) + g.digit + remove("-" + ss(g.letter) + qq(feats('numeral', 'ord', 'neu', 'gen'))),
                      qq(remove(maybe_stressed("м+есяца"))),
                      insert("."),
                      rr(g.digit, 4) + remove("-" + ss(g.letter) + qq(feats('numeral', 'ord', 'gen', 'mas'))),
                      qq(remove(maybe_stressed("г+ода"))),
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
