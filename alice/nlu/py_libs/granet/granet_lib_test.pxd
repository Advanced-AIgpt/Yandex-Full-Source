from common cimport *
from granet_lib_compiler cimport *
from granet_lib_sample cimport *

cdef extern from "alice/nlu/granet/lib/test/dataset.h" namespace "NGranet":
    cdef cppclass TTsvSample:
        pass


    cdef cppclass TTsvSampleDataset:
        ctypedef TIntrusivePtr[TTsvSampleDataset] TRef
        ctypedef TIntrusiveConstPtr[TTsvSampleDataset] TConstRef

        TTsvSampleDataset()
        TTsvSampleDataset(const TFsPath& path)
        void Load(const TFsPath& path)
        TTsvSample ReadSample(size_t index)
        size_t Size() const


cdef extern from "alice/nlu/granet/lib/test/sample_creator.h" namespace "NGranet":
    cdef enum EEntitySourceType:
        EST_TSV
        EST_ONLINE
        EST_EMPTY


    cdef cppclass TSampleCreatorWithCache:
        ctypedef TIntrusivePtr[TSampleCreatorWithCache] TRef
        TSample.TRef CreateSample(const TTsvSample& tsvSample, EEntitySourceType entitySources)

        @staticmethod
        TSampleCreatorWithCache.TRef Create(const TGranetDomain& domain, size_t cacheLimit)


cdef extern from "alice/nlu/granet/lib/test/fetcher.h" namespace "NGranet":
    cdef struct TBegemotFetcherOptions:
        TBegemotFetcherOptions()
