# cython: wraparound=False
from libcpp cimport bool as bool_t
from util.generic.string cimport TString, TStringBuf
from util.generic.vector cimport TVector
from util.system.types cimport ui32

import numpy as np


cdef extern from "catboost/libs/cat_feature/cat_feature.h":
    cpdef ui32 CalcCatFeatureHash(TStringBuf feature)
    cpdef float ConvertCatFeatureHashToFloat(ui32 hashVal)

cpdef float convert_cat_to_float(s):
    cdef TString st = TString(<char *>s)
    return ConvertCatFeatureHashToFloat(CalcCatFeatureHash(st))

cdef extern from "alice/nlu/libs/catboost_model_interface/model_calcer_wrapper.h":
    cdef void* ModelCalcerCreate()
    cdef void ModelCalcerDelete(void* calcer)
    cdef bool_t LoadFullModelFromFile(void* calcer, const char* filename)
    cdef bool_t CalcModelPredictionFlat(void* calcer, size_t docCount, const float** floatFeatures, size_t floatFeaturesSize, double* result, size_t resultSize)

cdef bool_t _CalcModelPredictionFlat(void* calcer, size_t docCount, const float* floatFeatures,     size_t floatFeaturesSize, double* result, size_t resultSize):
    cdef TVector[const float*] features
    features.reserve(docCount)
    cdef size_t i
    for i in xrange(docCount):
        features.push_back(floatFeatures + i * floatFeaturesSize)
    return CalcModelPredictionFlat(calcer, docCount, features.data(), floatFeaturesSize, result, resultSize)

cdef class CatBoostApplier:
    cdef void* Calcer
    cdef size_t ApproxDim

    def __cinit__(self, filename, size_t approxDim):
        self.Calcer = ModelCalcerCreate()
        success = LoadFullModelFromFile(self.Calcer, filename)
        assert success and 'Failed init CatBoostApplier from ' + filename
        self.ApproxDim = approxDim

    def __dealloc__(self):
        ModelCalcerDelete(self.Calcer)

    def predict(self, features):
        num_samples = features.shape[0]
        num_features = features.shape[1]
        result = np.zeros(num_samples * self.ApproxDim, dtype=np.float64)
        cdef double[::1] result_view = result
        cdef const float[::1] features_view = features.ravel()
        success = _CalcModelPredictionFlat(
            self.Calcer, num_samples, &features_view[0], num_features, &result_view[0], len(result))
        assert success and 'Failed apply'
        return result.reshape((num_samples, self.ApproxDim))

    def predict_proba(self, features):
        logits = self.predict(features)
        logits -= np.tile(logits.max(axis=1)[:, None], self.ApproxDim)
        probs = np.exp(logits)
        probs /= np.tile(probs.sum(axis=1)[:, None], self.ApproxDim)
        return probs
