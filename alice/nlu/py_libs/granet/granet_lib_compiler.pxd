from common cimport *
from granet_lib_grammar cimport *

cdef extern from "alice/nlu/granet/lib/compiler/data_loader.h" namespace "NGranet::NCompiler":
    cdef cppclass IDataLoader:
        pass


cdef extern from "alice/nlu/granet/lib/compiler/source_text_collection.h" namespace "NGranet::NCompiler":
    cdef cppclass TSourceTextCollection:
        TGranetDomain Domain
        TString MainTextPath
        THashMap[TString, TString] Texts
        TString ToCompressedBase64() const
        void FromCompressedBase64(TStringBuf str)

    cdef cppclass TReaderFromSourceTextCollection:
        TReaderFromSourceTextCollection(const TSourceTextCollection& collection)


cdef extern from "alice/nlu/granet/lib/compiler/compiler.h" namespace "NGranet::NCompiler":
    cdef cppclass TCompiler:
        TCompiler()
        TSourceTextCollection CollectSourceTexts(const TFsPath& path, const TGranetDomain& domain, IDataLoader* loader)
