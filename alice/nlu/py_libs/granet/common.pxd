from libcpp cimport bool, nullptr
from libcpp.pair cimport pair
from libcpp.vector cimport vector

from ads.yacontext.lib.cython.util cimport (IOutputStream, StdOutStream)

from util.folder.path cimport TFsPath
from util.generic.deque cimport TDeque
from util.generic.hash cimport THashMap
from util.generic.ptr cimport TIntrusivePtr
from util.generic.string cimport TStringBuf, TString
from util.generic.vector cimport TVector


cdef extern from "util/generic/ptr.h" nogil:
    cdef cppclass TIntrusiveConstPtr[TYPE]:
        TIntrusiveConstPtr(TYPE* t)
        TIntrusiveConstPtr()
        TYPE* Get() const


cdef extern from "util/stream/str.h":
    cdef cppclass TStringOutput:
        TStringOutput(TString& s)
        void Flush()


cdef extern from "util/generic/flags.h":
    cdef cppclass TFlags[T]:
        ctypedef T TEnum
        TFlags operator&(TFlags l, TFlags.TEnum r)
        #operator bool() const


cdef extern from "library/cpp/containers/comptrie/key_selector.h":
    cdef cppclass TCompactTrieKeySelector[T]:
        ctypedef TVector[T] TKey;

cdef extern from "library/cpp/containers/comptrie/comptrie_trie.h":
    cdef cppclass TCompactTrie[T, D]:
        ctypedef TCompactTrieKeySelector[T].TKey TKey
        ctypedef pair[TCompactTrie.TKey, D] TValueType

        size_t Size() const
        cppclass TConstIterator:
            TCompactTrie.TValueType& operator*()
            TConstIterator operator++()
            TConstIterator operator--()
            bint operator==(TConstIterator)
            bint operator!=(TConstIterator)
        TConstIterator begin()
        TConstIterator end()


cdef extern from "library/cpp/langs/langs.h":
    cdef enum ELanguage:
        LANG_RUS


cdef extern from "alice/nlu/libs/interval/interval.h" namespace "NNlu":
    cdef cppclass TInterval:
        size_t Begin
        size_t End
