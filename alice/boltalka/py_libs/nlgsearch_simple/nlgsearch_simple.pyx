# coding: utf-8
# cython: wraparound=False

from util.generic.string cimport TString
from util.generic.vector cimport TVector
from cpython cimport array
import array
include "alice/boltalka/py_libs/apply_nlg_dssm/apply_nlg_dssm.pyx"

cdef extern from "alice/boltalka/libs/nlgsearch_simple/nlgsearch_simple.h" namespace "NNlg":
    struct TNlgSearchSimpleParams:
        TString IndexDir
        TString DssmModelNames
        TString KnnIndexNames
        TString FactorDssmModelNames
        TString RankerModelName
        TString RankerModelNamesToLoad
        TString BaseDssmModelName
        TString BaseKnnIndexName
        TString Seq2SeqExternalUri
        TString MemoryMode;
        size_t MaxResults
        size_t NumThreads

    cdef cppclass TNlgSearchSimple:
        TNlgSearchSimple(const TNlgSearchSimpleParams& params)

        size_t GetDimension(const TString& dssmModelName) const
        TVector[float] EmbedContext(const TString& dssmModelName, const TVector[TString]& context) nogil const
        TVector[TString] GetReplyTexts(const TVector[TString]& context) nogil const
        TVector[const float*] GetReplyEmbeddings(const TString& dssmModelName, const TVector[TString]& context) nogil const
        void GetCandidates(const TString& dssmModelName, const TVector[TString]& context, TVector[const float*]* replyEmbeddings, TVector[TString]* replies, TVector[float]* contextEmbbeding, TVector[float]* scores) nogil const


cdef class NlgSearch:
    """TNlgSearch python wrapper.

    Parameters
    ----------
    index_dir : str
        Path to search index. Files "index[arc/dir/inv/key]" are stored in this folder.
        E.g. "/place/home/persiyanov/extsearch_models/tw35fps70l_20180314".
    max_results : int, optional
        Number of top results to return during queries. Default is 1.
    dssm_model_names : str, optional
        Name of dssm model to load. Default is None, which means "insight_c3_rus_lister" is loaded.
    factor_dssm_model_names : str, optional
        Name of dssm factor models. Only used in case of reranker. Default is None,
        which means no models are loaded & reranker is not used. E.g. "factor_dssm_0,factor_dssm_1".
    ranker_model_name : str, optional
        Name of ranker model to use. Default is None, which means no reranker is used. E.g. "catboost" or "matrixnet_relev".

    """
    cdef TNlgSearchSimple *_thisptr

    def __cinit__(self, index_dir, size_t max_results=1, dssm_model_names=None, factor_dssm_model_names=None, knn_index_names=None, ranker_model_name=None, size_t num_threads=20, memory_mode='Precharged', seq2seq_uri=''):
        """Wrapper constructor.
        """
        cdef TNlgSearchSimpleParams params
        params.IndexDir = self._to_tstring(index_dir)
        params.MaxResults = max_results
        params.NumThreads = num_threads

        if dssm_model_names:
            params.DssmModelNames = self._to_tstring(dssm_model_names)
            params.BaseDssmModelName = self._to_tstring(dssm_model_names.split(',')[0])
        if knn_index_names:
            params.KnnIndexNames = self._to_tstring(knn_index_names)
            params.BaseKnnIndexName = self._to_tstring(knn_index_names.split(',')[0].split(':')[0])
        if factor_dssm_model_names:
            params.FactorDssmModelNames = self._to_tstring(factor_dssm_model_names)
        if ranker_model_name:
            params.RankerModelNamesToLoad = self._to_tstring(ranker_model_name)
        params.Seq2SeqExternalUri = self._to_tstring(seq2seq_uri)
        params.MemoryMode = self._to_tstring(memory_mode)

        self._thisptr = new TNlgSearchSimple(params)

    def __dealloc__(self):
        if self._thisptr != NULL:
            del self._thisptr

    cpdef TString _to_tstring(self, s):
        return <TString>bytes(s, encoding="utf8")

    cpdef TVector[TString] _context_to_tstrings(self, list context):
        cdef TVector[TString] res;
        for i in range(len(context)):
            res.push_back(self._to_tstring(context[i]))
        return res

    cpdef TVector[float] embed_context(self, str dssm_model_name, list context):
        """Get context embedding.

        Parameters
        ----------
        context : list of str
            Previous utterances in format [oldest_turn, ...., most_recent_turn].
        dssm_model_name: str

        Returns
        ------
        list of floats with length == self.dimension

        """
        context = self._context_to_tstrings(context)
        return self._thisptr.EmbedContext(dssm_model_name, context)

    cpdef TVector[TString] get_reply_texts(self, list context):
        """Get top replies given context.

        Parameters
        ----------
        context : list of str
            Previous utterances in format [oldest_turn, ...., most_recent_turn].

        Returns
        -------
        list of str
            List of replies. The number of replies equals to `max_results` passed in the contructor.

        """
        context = self._context_to_tstrings(context)
        return self._thisptr.GetReplyTexts(context)

    cpdef list get_reply_embeddings(self, str dssm_model_name, list context):
        """Get embeddings of top replies given context.

        Parameters
        ----------
        context : list of str
            Previous utterances in format [oldest_turn, ...., most_recent_turn]..
        dssm_model_name: str

        Returns
        -------
        list of list of floats
            List of replies vectors. Length of list equals to `max_results` passed in the constructor.

        """
        cdef size_t dimension = self._thisptr.GetDimension(bytes(dssm_model_name, encoding='utf8'))
        context = self._context_to_tstrings(context)
        cdef TVector[const float*] results = self._thisptr.GetReplyEmbeddings(bytes(dssm_model_name, encoding='utf8'), context)

        cdef list res = []
        cdef const float* vec_ptr
        cdef size_t i
        for vec_ptr in results:
            vec = [0.0 for i in range(dimension)]
            for i in range(dimension):
                vec[i] = vec_ptr[i]
            res.append(vec)
        return res

    cpdef list get_candidates(self, str dssm_model_name, list context):
        cdef size_t dimension = self._thisptr.GetDimension(bytes(dssm_model_name, encoding='utf8'))
        cdef TVector[TString] context_vec = self._context_to_tstrings(context)
        cdef TVector[const float*] replyEmbeddings
        cdef TVector[TString] replies
        cdef TVector[float] contextEmbbeding
        cdef TVector[float] scores
        cdef TString dssm_model = self._to_tstring(dssm_model_name)
        with nogil:
            self._thisptr.GetCandidates(dssm_model, context_vec, &replyEmbeddings, &replies, &contextEmbbeding, &scores)

        cdef const float* vec_ptr
        cdef size_t i
        cdef array.array res = array.array('f')
        cdef array.array contextArray = array.array('f')
        cdef array.array scoresArray = array.array('f')
        for vec_ptr in replyEmbeddings:
            array.extend_buffer(res, <char*>vec_ptr, dimension)
        array.extend_buffer(contextArray, <char*>contextEmbbeding.data(), dimension)
        array.extend_buffer(scoresArray, <char*>scores.data(), len(scores))

        return [contextArray, res, replies, scores]
