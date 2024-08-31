# coding: utf-8
# cython: wraparound=False

from util.generic.string cimport TString


cdef extern from "alice/boltalka/generative/training/data/tokenizer/tokenizer.h":
    cdef cppclass TTokenizer:
        TTokenizer(const TString& bpeVocPath)
        TString Tokenize(const TString& string)

cdef class Tokenizer:
    cdef TTokenizer* tokenizer

    def __cinit__(self, bpe_voc_path):
        self.tokenizer = new TTokenizer(bpe_voc_path)

    def __dealloc__(self):
        del self.tokenizer

    def tokenize(self, string):
        return self.tokenizer.Tokenize(string)
