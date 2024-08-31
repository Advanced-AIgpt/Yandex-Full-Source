#! /usr/bin/env python2
# encoding: utf-8
import codecs
import sys
sys.path.append('..')
sys.path.append('../..')

from general.normbase import *
import morphology

print 'Substitution'

def make_subctitution_cvt(fname):
    def line_to_pair(ln):
        ln = ln.decode('utf-8')
        ln = ln.strip()
        split_ln = ln.split('\t')
        if len(split_ln) == 2:
            # the normal case
            [s_from, s_to] = split_ln
        elif len(split_ln) == 1:
            # deletion
            [s_from] = split_ln
            s_to = ""
        elif len(split_ln) == 0:
            return (u"", u"")
        else:
            print 'Bad line', ln
            return (u"", u"")
        for c in ln:
            if c != '\t' and c not in g.sym_set:
                print 'Bad symbol', c
                continue
        s_from = s_from.strip()
        s_to = s_to.strip()
        return (s_from, s_to)

    dd = dict()
    with codecs.open(fname) as f:
        for ln in f:
            s_from, s_to = line_to_pair(ln)
            if s_from != u"" and s_from not in dd:
                dd[s_from] = s_to

    r = Fst.union_seq([replace(s_from, dd[s_from]) for s_from in dd])
    r = r.optimize()
    return r

current_directory = os.path.dirname(os.path.abspath(__file__))

simplify_url_cvt = make_subctitution_cvt(os.path.join(current_directory, "simplify_url.txt"))

substitution_cvt = make_subctitution_cvt(os.path.join(current_directory, "substitution.txt"))

url_substitution_cvt = make_subctitution_cvt(os.path.join(current_directory, "url_substitution.txt"))
