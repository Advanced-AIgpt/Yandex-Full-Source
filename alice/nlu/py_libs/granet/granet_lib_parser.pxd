from common cimport *
from granet_lib_sample cimport *
from granet_lib_grammar cimport *


cdef extern from "alice/nlu/granet/lib/parser/result.h" namespace "NGranet":
    cdef cppclass TParserVariant:
        ctypedef TIntrusiveConstPtr[TParserVariant] TConstRef
        TVector[TSlotMarkup] ToMarkup() const
        TVector[TSlotMarkup] ToNonterminalMarkup(const TIntrusiveConstPtr[TGrammar]& grammar) const

    cdef cppclass TParserFormResult:
        ctypedef TIntrusiveConstPtr[TParserFormResult] TConstRef
        const TString& GetName() const
        bool IsPositive() const
        const TParserVariant.TConstRef& GetBestVariant() const
        const TVector[TParserVariant.TConstRef]& GetVariants() const
        void Dump(IOutputStream* log, const TString& indent) const
