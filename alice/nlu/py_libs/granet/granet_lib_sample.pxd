from common cimport *

cdef extern from "alice/nlu/granet/lib/sample/sample.h" namespace "NGranet":
    cdef cppclass TSample:
        ctypedef TIntrusivePtr[TSample] TRef
        const TString& GetText() const
        TString SaveToJsonString() const


cdef extern from "alice/nlu/granet/lib/sample/markup.h" namespace "NGranet":
    cdef cppclass TSlotMarkup:
        TString Name
        TInterval Interval
        TSlotMarkup()
        TSlotMarkup(const TSlotMarkup& other)
        TString PrintMarkup(bool needValue, bool needType) const


    cdef cppclass TSampleMarkup:
        TString Text
        TVector[TSlotMarkup] Slots
        TSampleMarkup() except +
        TString PrintMarkup(bool needValue, bool needType) const
