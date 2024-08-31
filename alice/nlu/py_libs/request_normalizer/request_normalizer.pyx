from alice.nlu.py_libs.request_normalizer.lang import Lang
from util.generic.string cimport TStringBuf, TString


cdef extern from "library/cpp/langs/langs.h" nogil:
    ctypedef enum ELanguage:
        pass
    ELanguage LanguageByName(TStringBuf& name)


cdef extern from "alice/nlu/libs/request_normalizer/request_normalizer.h" namespace "NNlu" nogil:
    cpdef cppclass TRequestNormalizer:
        @staticmethod
        TString Normalize(ELanguage lang, TStringBuf text) except +
        @staticmethod
        void WarmUpSingleton() except +


class RequestNormalizer:

    @staticmethod
    def _check_lang(lang):
        if not lang:
            raise TypeError("'lang' must not be None")

    @staticmethod
    def _check_text(text):
        if text is None:
            raise TypeError('Text must a valid string object')

    @staticmethod
    def normalize(lang, text):
        RequestNormalizer._check_lang(lang)
        RequestNormalizer._check_text(text)
        encoded_lang = lang.value.encode()
        l = LanguageByName(TStringBuf(<const char *> encoded_lang, <size_t> len(encoded_lang)))
        encoded_text = text.encode('utf-8')
        return TRequestNormalizer.Normalize(l, TStringBuf(<const char *> encoded_text, <size_t> len(encoded_text)))\
            .data().decode('utf-8')

    @staticmethod
    def warm_up_singleton():
        TRequestNormalizer.WarmUpSingleton()
