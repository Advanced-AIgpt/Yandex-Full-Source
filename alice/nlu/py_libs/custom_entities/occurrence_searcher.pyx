from alice.nlu.proto.entities import custom_pb2 as custom_entities_pb2
from util.generic.string cimport *
from util.generic.vector cimport *
from util.memory.blob cimport *
from util.system.types cimport ui64
import logging
import os.path


logger = logging.getLogger(__name__)


cdef extern from "alice/nlu/proto/entities/custom.pb.h" namespace "NAlice::NNlu":
    cdef:
        cppclass TCustomEntityValues:
            TString SerializeAsString() const


cdef extern from "alice/nlu/libs/occurrence_searcher/occurrence_searcher.h" namespace "NAlice::NNlu":
    cdef:
        cppclass TOccurrence[TCustomEntityValues]:
            TCustomEntityValues Value
            ui64 Begin
            ui64 End

    cdef:
        cppclass TTokenizedOccurrenceSearcher[TCustomEntityValues]:
            TTokenizedOccurrenceSearcher(const TBlob)
            TVector[TOccurrence[TCustomEntityValues]] Search(const TVector[TString]& tokens)


def _to_string(str):
    return TString(<const char*>str, len(str))


def _to_string_vector(lst):
    cdef TVector[TString] v
    for item in lst:
        v.push_back(_to_string(item))
    return v


class CustomEntity:
    def __init__(self, type, value, begin, end):
        self.type = type
        self.value = value
        self.begin = begin
        self.end = end


cdef class CustomEntitySearcher:
    cdef TTokenizedOccurrenceSearcher[TCustomEntityValues]* searcher

    def __cinit__(self, searcher_data_path):
        self.searcher = NULL
        if not os.path.exists(searcher_data_path):
            logger.warning('File not found: {0}'.format(searcher_data_path))
            return

        self.searcher = new TTokenizedOccurrenceSearcher[TCustomEntityValues](TBlob.FromFile(searcher_data_path))
        
    def __dealloc__(self):
        if self.searcher:
            del self.searcher

    def search(self, tokens):
        result = []
        if not self.searcher:
            return result
        occurrences_cpp = self.searcher.Search(_to_string_vector(tokens))
        for occurrence_cpp in occurrences_cpp:
            custom_entity_values = custom_entities_pb2.TCustomEntityValues()
            custom_entity_values.ParseFromString(occurrence_cpp.Value.SerializeAsString())
            for value in custom_entity_values.CustomEntityValues:
                result.append(CustomEntity(
                    type=value.Type,
                    value=value.Value,
                    begin=int(occurrence_cpp.Begin),
                    end=int(occurrence_cpp.End)
                ))
        return result
