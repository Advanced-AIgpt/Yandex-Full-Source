#!/usr/bin/env python
# -*- coding: utf-8 -*-

from _crfsuitex import *
import os

def convert_to_std(x):
    x_c = std_vector_vector_string()
    w_c = std_vector_vector_double()
    for values, weights in x:
        values_c = std_vector_string()
        weights_c = std_vector_double()
        values_c.extend(values)
        weights_c.extend(weights)
        x_c.append(values_c)
        w_c.append(weights_c)
    return x_c, w_c

def convert_from_std(x):
    return [list(xi) for xi in x]


class Tagger():

    def __init__(self, npaths=1, normalized_scores=False):
        self.m = CRFSuiteXTagger()
        self.m.npaths = npaths
        self.m.normalized_scores = normalized_scores
        pass

    def open(self, model_file):
        self.m.load_from_file(model_file)

    def load(self, model_buff):
        self.m.model = model_buff

    def tag(self, X):
        Xitems = []
        for x in X.items():
            values = []
            weights = []
            for xk, weight in x.items():
                values.append(xk.encode('utf-8'))
                weights.append(weight)
            Xitems.append((values, weights))

        Xc, Wc = convert_to_std(Xitems)
        if self.m.npaths > 1:
            self.m.decode_nbest(Xc, Wc)
        else:
            self.m.decode(Xc, Wc)
        Y = convert_from_std(self.m.nbest)
        p = list(self.m.scores)
        return Y, p


if __name__=="__main__":
    print 'Test Tagger()...'
    t = Tagger()
    print 'Ok!'


