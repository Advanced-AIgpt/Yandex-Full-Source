from libcpp.utility cimport pair

from common cimport *
from granet_lib_compiler cimport *
from granet_lib_grammar cimport *
from granet_lib_parser cimport *
from granet_lib_sample cimport *
from granet_lib_test cimport *


# -- granet lib functions -- 

cdef extern from "alice/nlu/granet/lib/granet.h" namespace "NGranet":
    cdef TGrammar.TRef CompileGrammarFromString(TStringBuf str, const TGranetDomain& domain) except +
    cdef TGrammar.TRef CompileGrammarFromPath(const TFsPath& path, const TGranetDomain& domain, IDataLoader* loader) except +
    cdef TSample.TRef CreateSample(TStringBuf line, ELanguage language) except +
    cdef TVector[TParserFormResult.TConstRef] ParseSample(const TGrammar.TConstRef& grammar, const TSample.TRef& sample) except +
    cdef void FetchEntities(const TSample.TRef& sample, const TGranetDomain& domain, const TBegemotFetcherOptions& options) except +


cdef extern from "alice/nlu/py_libs/granet/util/lib/local_executor_wrapper.h" namespace "NParallelGranet":
    cdef TVector[TString] ParseInParallel(const TGrammar.TRef& grammar, TVector[TSample.TRef]& samples, 
        int nThreads, int blockSize, bool needValues, bool needTypes) except +

    cdef TVector[TVector[pair[TInterval, bool]]] ParseToAlmostMatchedPartsInParallel(const TGrammar.TRef& grammar, 
        TVector[TSample.TRef]& samples, int nThreads, int blockSize) except +

    cdef TVector[TParserFormResult.TConstRef] ParseToResultsInParallel(const TGrammar.TRef& grammar, 
        TVector[TSample.TRef]& samples, int nThreads, int blockSize) except +
