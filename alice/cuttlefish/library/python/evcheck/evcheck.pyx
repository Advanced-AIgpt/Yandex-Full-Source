from util.generic.ptr cimport THolder
from util.generic.string cimport TStringBuf


cdef extern from "alice/cuttlefish/library/evcheck/evcheck.h" namespace "NVoice":
    cdef cppclass TParser:
        bint ParseJson(TStringBuf rawJson) except +

    THolder[TParser] ConstructSynchronizeStateParserInHeap()


cdef class EventCheck:
    cdef THolder[TParser] __parser

    def __init__(self):
        self.__parser = ConstructSynchronizeStateParserInHeap()

    def check(self, const char* data):
        return self.__parser.Get().ParseJson(TStringBuf(data))
