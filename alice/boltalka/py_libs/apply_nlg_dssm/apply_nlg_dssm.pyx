# coding: utf-8
# cython: wraparound=False

from util.generic.string cimport TString
from util.generic.vector cimport TVector


cdef extern from "alice/boltalka/libs/dssm_model/dssm_model.h" namespace "NNlg":
    cdef cppclass TDssmModel:
        TDssmModel(const TString& modelFilename)
        size_t GetDimension(const TString& dssmModelName) const
        TVector[TVector[float]] FpropBatch(const TVector[TVector[TString]]& context, const TVector[TString]& reply, const TVector[TString]& output) const


cdef class NlgDssmApplier:
    cdef TDssmModel *_thisptr

    def __cinit__(self, model_filename):
        """Wrapper constructor.
        """
        self._thisptr = new TDssmModel(bytes(model_filename, encoding="utf8"))

    cpdef TString _to_tstring(self, s):
        if isinstance(s, str):
            s = bytes(s, encoding="utf8")
        return <TString>s

    cpdef TVector[TString] _replies_to_tstrings(self, list context):
        cdef TVector[TString] res;
        for i in range(len(context)):
            res.push_back(self._to_tstring(context[i]))
        return res

    cpdef TVector[TVector[TString]] _contexts_to_tstrings(self, list context):
        cdef TVector[TVector[TString]] res;
        for i in range(len(context)):
            res.push_back([])
            for j in range(len(context[i])):
                res[i].push_back(self._to_tstring(context[i][j]))
        return res

    def __dealloc__(self):
        if self._thisptr != NULL:
            del self._thisptr

    cpdef list get_embeddings(self, list contexts, list replies):
        contexts = [list(reversed(el)) for el in contexts]
        cdef TVector[TVector[TString]] tvector_context = self._contexts_to_tstrings(contexts)
        cdef TVector[TString] tstring_reply = self._replies_to_tstrings(replies)
        cdef TVector[TVector[float]] results = self._thisptr.FpropBatch(tvector_context, tstring_reply, [b"query_embedding", b"doc_embedding"])

        cdef list res = []
        cdef TVector[float] vec
        cdef size_t i
        for vec in results:
            vec_list = [0.0 for i in range(vec.size())]
            for i in range(vec.size()):
                vec_list[i] = vec[i]
            res.append(vec)
        return res

    cpdef list get_scores(self, list contexts, list replies):
        contexts = [list(reversed(el)) for el in contexts]
        cdef TVector[TVector[TString]] tvector_context = self._contexts_to_tstrings(contexts)
        cdef TVector[TString] tstring_reply = self._replies_to_tstrings(replies)
        cdef TVector[float] vec = self._thisptr.FpropBatch(tvector_context, tstring_reply, [b"output"])[0]

        cdef list vec_list = [0.0 for i in range(vec.size())]
        for i in range(vec.size()):
            vec_list[i] = vec[i]
        return vec_list
