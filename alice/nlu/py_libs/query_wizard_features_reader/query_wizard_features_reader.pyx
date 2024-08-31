# coding: utf-8
# cython: wraparound=False

from libcpp cimport bool as bool_t
from util.generic.string cimport TString
from util.generic.vector cimport TVector


cdef extern from "alice/nlu/libs/query_wizard_features_reader_wrapper/query_wizard_features_reader_wrapper.h":
    struct TFragmentFeatures:
        size_t Start
        size_t Length
        TString Fragment
        TString NormalizedFragment
        TVector[double] Features

    cdef void* ReaderCreate();
    cdef void ReaderDelete(void* reader)
    cdef bool_t LoadTrie(void* reader, const char* triePath, const char* dataPath)
    cdef bool_t GetFeaturesForTextFragments(void* reader, const char* text, TVector[TFragmentFeatures]* result)

cdef _get_python_fragment_features(const TFragmentFeatures& fragmentFeatures):
    return {
        u'start': fragmentFeatures.Start,
        u'length': fragmentFeatures.Length,
        u'fragment': fragmentFeatures.Fragment.decode('UTF-8'),
        u'normalized_fragment': fragmentFeatures.NormalizedFragment.decode('UTF-8'),
        u'features': fragmentFeatures.Features
    }

cdef class QueryWizardFeaturesReader:
    cdef void* Reader

    def __cinit__(self, triePath, dataPath):
        self.Reader = ReaderCreate()
        success = LoadTrie(self.Reader, triePath, dataPath)
        assert success and 'Failed init QueryWizardFeaturesReader from ' + triePath

    def __dealloc__(self):
        ReaderDelete(self.Reader)

    def get_features(self, text):
        cdef TVector[TFragmentFeatures] fragments_vec;
        success = GetFeaturesForTextFragments(self.Reader, text.encode('UTF-8'), &fragments_vec)
        assert success and 'Failed to get features'

        result = []

        if fragments_vec.size() != 0:
            fragments = &fragments_vec.front()
            for idx in xrange(fragments_vec.size()):
                result.append(_get_python_fragment_features(fragments[idx]))

        return result
