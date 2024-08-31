from common cimport *

from alice.nlu.granet.lib.utils.rle_array cimport TRleArray
from libc.stdint cimport uint32_t

cdef extern from "alice/nlu/granet/lib/grammar/grammar.h" namespace "NGranet":
    cdef cppclass TGrammar:
        ctypedef TIntrusivePtr[TGrammar] TRef
        ctypedef TIntrusiveConstPtr[TGrammar] TConstRef
        const TGrammarData& GetData() const


cdef extern from "alice/nlu/granet/lib/grammar/domain.h" namespace "NGranet":
    cdef cppclass TGranetDomain:
        ELanguage Lang
        bool IsPASkills
        bool IsWizard
        bool IsSnezhana
        TGranetDomain()


cdef extern from "alice/nlu/granet/lib/grammar/token_id.h" namespace "NGranet":
    ctypedef uint32_t TElementId
    ctypedef uint32_t TTokenId
    ctypedef TCompactTrie[char, TTokenId] TWordTrie

    cdef cppclass TTokenPool:
        TTokenPool()
        TTokenPool(const TWordTrie& trie)
        const TString& GetWord(TTokenId id) const


cdef extern from "alice/nlu/granet/lib/grammar/string_pool.h" namespace "NGranet":
    ctypedef uint32_t TStringId


cdef extern from "alice/nlu/granet/lib/grammar/token_id.h" namespace "NGranet::NTokenId":
    bool IsElement(TTokenId id)


cdef extern from "alice/nlu/granet/lib/grammar/rule_trie.h" namespace "NGranet":
    cdef cppclass TRuleIndexes:
        pass

    ctypedef TCompactTrie[TTokenId, TRuleIndexes] TRuleTrie


cdef extern from "alice/nlu/granet/lib/grammar/grammar_data.h" namespace "NGranet":
    cdef enum EElementFlag:
        EF_IS_PUBLIC

    ctypedef TFlags[EElementFlag] EElementFlags

    cdef cppclass TGrammarElement:
        TString Name
        TElementId OriginalElement
        size_t RuleTrieIndex
        EElementFlags Flags
        TRleArray[TStringId] DataTypes
        TRleArray[TStringId] DataValues
        bool IsEntity() const

    cdef cppclass TGrammarData:
        TWordTrie WordTrie
        TVector[TGrammarElement] Elements
        TDeque[TRuleTrie] RuleTriePool
        TVector[TString] StringPool
        const TGrammarElement* MaybeGetElement(TElementId id) const
