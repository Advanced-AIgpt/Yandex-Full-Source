#pragma once

#include "domain.h"
#include "rule_trie.h"

#include <alice/nlu/granet/lib/utils/rle_array.h>
#include <alice/nlu/libs/tuple_like_type/tuple_like_type.h>
#include <alice/nlu/granet/lib/utils/utils.h>
#include <alice/nlu/libs/interval/interval.h>

#include <library/cpp/containers/comptrie/search_iterator.h>
#include <library/cpp/langs/langs.h>

#include <util/digest/sequence.h>
#include <util/generic/deque.h>
#include <util/generic/map.h>
#include <util/generic/maybe.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NGranet {

class TGrammar;

// TGrammarData serialization version.
inline const size_t GRAMMAR_DATA_CURRENT_VERSION = 23;

// ~~~~ ESynonymFlag ~~~~

enum ESynonymFlag : ui32 {
    SF_TRANSLIT             = FLAG32(0),
    SF_SYNSET               = FLAG32(1),
    SF_DIMIN_NAME           = FLAG32(2),
    SF_SYNON                = FLAG32(3),
    SF_FIO                  = FLAG32(4),
};

Y_DECLARE_FLAGS(ESynonymFlags, ESynonymFlag);
Y_DECLARE_OPERATORS_FOR_FLAGS(ESynonymFlags);

// ~~~~ ESlotMatchingType ~~~~

enum ESlotMatchingType {
    SMT_INSIDE      /* "inside" */,
    SMT_OVERLAP     /* "overlap" */,
    SMT_EXACT       /* "exact" */,
};

// ~~~~ TSlotDescription ~~~~

struct TSlotDescription {
    TString Name;
    TVector<TString> DataTypes;
    ESlotMatchingType MatchingType = SMT_INSIDE;
    bool ConcatenateStrings = false;
    bool KeepVariants = false;

    Y_SAVELOAD_DEFINE(Name, DataTypes, MatchingType, ConcatenateStrings, KeepVariants);

    void Dump(IOutputStream* log, const TString& indent = "") const;
};

// ~~~~ EParserTaskType ~~~~

enum EParserTaskType {
    PTT_FORM        /* "form" */,
    PTT_ENTITY      /* "entity" */,
    PTT_COUNT       /* "count" */,
};

// ~~~~ TParserTaskKey ~~~~

struct TParserTaskKey {
    EParserTaskType Type = PTT_FORM;
    TString Name;

    DECLARE_TUPLE_LIKE_TYPE(TParserTaskKey, Type, Name);
};

IOutputStream& operator<<(IOutputStream& out, const TParserTaskKey& key);

// ~~~~ TParserTask ~~~~

struct TParserTask {
    TString Name;
    EParserTaskType Type = PTT_FORM;
    size_t Index = NPOS;

    TElementId Root = UNDEFINED_ELEMENT_ID;
    TElementId UserFiller = UNDEFINED_ELEMENT_ID;
    TElementId AutoFiller = UNDEFINED_ELEMENT_ID;

    bool EnableGranetParser = false;
    bool EnableAliceTagger = false;
    bool EnableAutoFiller = false;
    ESynonymFlags EnableSynonymFlagsMask;
    ESynonymFlags EnableSynonymFlags;

    // For PTT_ENTITY.
    // Don't filter overlapped entities.
    bool KeepOverlapped = false;

    // For PTT_FORM.
    // true - write all parsing variants for each matched form (TParserFormResult::GetVariants().size() >= 1)
    // false - write only best variant for each matched form (TParserFormResult::GetVariants().size() == 1)
    bool KeepVariants = false;

    bool IsAction = false;
    bool IsFixlist = false;
    bool IsConditional = false;
    bool IsInternal = false;
    bool Fresh = false;
    ui32 Freshness = 0;

    TVector<TSlotDescription> Slots;
    TVector<TString> DependsOnEntities;

    Y_SAVELOAD_DEFINE(Name, Type, Index, Root, UserFiller, AutoFiller,
        EnableGranetParser, EnableAliceTagger, EnableAutoFiller,
        EnableSynonymFlagsMask, EnableSynonymFlags,
        KeepOverlapped, KeepVariants, Slots, DependsOnEntities,
        IsAction, IsFixlist, IsConditional, IsInternal, Fresh, Freshness);

    TParserTaskKey GetTaskKey() const;
    void Dump(const TGrammar& grammar, IOutputStream* log, const TString& indent = "") const;
};

// ~~~~ TSlotDescriptionId ~~~~

struct TSlotDescriptionId {
    EParserTaskType TaskType = PTT_FORM;
    size_t TaskIndex = 0;
    size_t SlotIndex = 0;

    DECLARE_TUPLE_LIKE_TYPE(TSlotDescriptionId, TaskType, TaskIndex, SlotIndex);
};

// ~~~~ TQuantityParams ~~~~

// Result of parsing element quantity unary operator.
// Examples of operator:  *  ?  +  {n}  {n,m}
struct TQuantityParams {
    ui8 MinCount = 1;
    ui8 MaxCount = 1;

    DECLARE_TUPLE_LIKE_TYPE(TQuantityParams, MinCount, MaxCount);

    TString GetNormalizedText() const;
};

// ~~~~ TElementRuleTag ~~~~

struct TElementRuleTag {
    TStringId Tag = 0;
    NNlu::TBaseInterval<ui16> Interval;

    DECLARE_TUPLE_LIKE_TYPE(TElementRuleTag, Tag, Interval);
};

// ~~~~ EElementFlag ~~~~

enum EElementFlag : ui32 {
    EF_ANCHOR_TO_BEGIN                      = FLAG32(8),
    EF_ANCHOR_TO_END                        = FLAG32(9),
    EF_ENABLE_FILLERS                       = FLAG32(10),
    EF_ENABLE_EDGE_FILLERS                  = FLAG32(11),
    EF_COVER_FILLERS                        = FLAG32(12),
    EF_HAS_DATA                             = FLAG32(14),
    EF_HAS_TAGS                             = FLAG32(15),
};

Y_DECLARE_FLAGS(EElementFlags, EElementFlag);
Y_DECLARE_OPERATORS_FOR_FLAGS(EElementFlags);

// ~~~~ TGrammarElement ~~~~

// Nonterminal (here is used more short name for nonterminal: element)
struct TGrammarElement {
    TElementId Id = UNDEFINED_ELEMENT_ID;

    TString Name;

    // Rules (chains of text tokens mixed with elements) stored in trie.
    // Index in TGrammar::RuleTriePool.
    size_t RuleTrieIndex = NPOS;

    // (Optimization) Iterator initialized as begin (root) of rules trie.
    TSearchIterator<TRuleTrie> RulesBeginIterator;

    // (Optimization) All TElementId used in dictionary Rules.
    // TCompactTrie doesn't have efficient way to obtain all children of some node (represented
    // by TSearchIterator). We have to check every possible element. Field ElementsInRules
    // is used to reduce that search.
    TVector<TElementId> ElementsInRules;

    EElementFlags Flags;
    ESynonymFlags EnableSynonymFlagsMask;
    ESynonymFlags EnableSynonymFlags;

    // Additional parameters for fake element which represents external named entity.
    TString EntityName;

    // Data associated with rules of element

    // Value associated with rules.
    TRleArray<TStringId> DataTypes;
    TRleArray<TStringId> DataValues;

    // Rule index -> range in TGrammarData::TagPool
    TRleArray<NNlu::TInterval> TagPoolRanges;

    // Log probability of this rule relative to whole trie.
    // Calculated from weights of rules:
    //   LogProb = ln(weight_of_this_rule / sum_by_trie(weight_of_rule_i))
    // By default weight of rule is 1.
    TVector<float> RulesLogProbs;

    // Rules marked by directive %force_negative or %force_positive (the latter doesn't exist actually).
    TDynBitMap ForcedRules;
    // Rules marked by directive %negative or %force_negative.
    // If parser choose 'negative' rule on some interval (rule wins by probability), then do not
    // generate completed element on this interval.
    TDynBitMap NegativeRules;

    // Multi-element params
    TQuantityParams Quantity;
    ui32 SetOfRequiredRules = 0;
    ui32 SetOfLimitedRules = 0;

    ui32 Level = 0;

    // Slots created by occurrences of this element.
    TVector<TSlotDescriptionId> SourceForSlots;

    bool CanSkip = false;
    float LogProbOfSkip = 0;
    ui32 SetOfPossibleEmptyRequiredRules = 0;
    TVector<std::pair<ui32, float>> LogProbOfRequiredRulesSkip;

    Y_SAVELOAD_DEFINE(Id, Name, RuleTrieIndex, ElementsInRules, Flags,
        EnableSynonymFlagsMask, EnableSynonymFlags, EntityName,
        DataTypes, DataValues, TagPoolRanges, RulesLogProbs, ForcedRules, NegativeRules,
        Quantity, SetOfRequiredRules, SetOfLimitedRules, Level, SourceForSlots,
        CanSkip, LogProbOfSkip, SetOfPossibleEmptyRequiredRules, LogProbOfRequiredRulesSkip);

    bool IsEntity() const {
        return !EntityName.empty();
    }
    void Dump(const TGrammar& grammar, const TTokenPool& tokenPool, IOutputStream* log,
        bool verbose = true, const TString& indent = "") const;
};

// ~~~~ TTokenToElementsMap ~~~~

// Map TTokenId -> set of TElementId.
struct TTokenToElementsMap {
    // Token (lemma or exact word) -> offset in ElementSetPool - beginning of sequence of TElementId
    // terminated by UNDEFINED_ELEMENT_ID.
    THashMap<TTokenId, ui32> TokenToOffset;

    // Pool of sets of elements used in TokenToOffset.
    TVector<TElementId> ElementSetPool;

    // Intersection of all sets of TElementId stored separately.
    // It must be added to any set obtained from this map.
    TDynBitMap CommonElements;

    Y_SAVELOAD_DEFINE(TokenToOffset, ElementSetPool, CommonElements);

    bool IsEmpty() const;
    void AddTokenUncommonElementsToSet(TTokenId token, TDynBitMap* set) const;
    void Dump(const TTokenPool& tokenPool, IOutputStream* log, const TString& indent = "") const;
};

// ~~~~ TGrammarOptimizationInfo ~~~~

struct TGrammarOptimizationInfo {
    // Token (lemma or exact word) -> set of elements whose 'specific word set' or 'first word set' contains this word.
    TTokenToElementsMap SpecificWordToElements;
    TTokenToElementsMap FirstWordToElements;

    Y_SAVELOAD_DEFINE(SpecificWordToElements, FirstWordToElements);

    void Dump(const TTokenPool& tokenPool, IOutputStream* log, const TString& indent = "") const;
};

// ~~~~ TGrammarData ~~~~

// Serialized data of TGrammar
struct TGrammarData {
    TGranetDomain Domain;
    TMaybe<TString> ExternalSource;
    TWordTrie WordTrie;
    TVector<TGrammarElement> Elements;
    TVector<TParserTask> Entities;
    TVector<TParserTask> Forms;
    TDeque<TRuleTrie> RuleTriePool;
    TVector<TString> StringPool;
    TVector<TElementRuleTag> TagPool;
    ui32 ElementLevelCount = 0;
    TGrammarOptimizationInfo OptimizationInfo;

    Y_SAVELOAD_DEFINE(Domain, ExternalSource, WordTrie, Elements, Entities, Forms, RuleTriePool, StringPool, TagPool,
        ElementLevelCount, OptimizationInfo);

    const TVector<TParserTask>& GetTasks(EParserTaskType type) const {
        return type == PTT_FORM ? Forms : Entities;
    }

    const TGrammarElement* MaybeGetElement(TElementId id) const {
        if (id == UNDEFINED_ELEMENT_ID) {
            return nullptr;
        } else {
            return &Elements[id];
        }
    }
};

} // namespace NGranet

// ~~~~ global namespace ~~~~

template <>
struct THash<NGranet::TParserTaskKey>: public TTupleLikeTypeHash {
};

template <>
struct THash<NGranet::TQuantityParams>: public TTupleLikeTypeHash {
};

template <>
struct THash<NGranet::TElementRuleTag>: public TTupleLikeTypeHash {
};

template <>
struct THash<TVector<NGranet::TElementRuleTag>>: public TSimpleRangeHash {
};
