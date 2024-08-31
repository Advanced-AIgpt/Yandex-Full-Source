#include "optimization_info_builder.h"
#include <alice/nlu/granet/lib/lang/word_logprob_table.h>
#include <alice/nlu/granet/lib/utils/trace.h>
#include <dict/nerutil/tstimer.h>
#include <library/cpp/iterator/enumerate.h>
#include <library/cpp/iterator/zip.h>
#include <util/generic/xrange.h>
#include <util/string/join.h>
#include <util/string/printf.h>
#include <util/stream/trace.h>

namespace NGranet::NCompiler::NOptimizer {

// ~~~~ TTokenSet ~~~~

void TTokenSet::AddLazy(TTokenId id) {
    if (!IsAll) {
        Tokens.push_back(id);
    }
}

void TTokenSet::AddLazy(const TTokenSet& other) {
    if (IsAll) {
        return;
    }
    if (other.IsAll) {
        IsAll = true;
        Tokens.clear();
        return;
    }
    Extend(other.Tokens, &Tokens);
}

void TTokenSet::Normalize() {
    SortUnique(Tokens);
}

TString TTokenSet::Print(const TTokenPool& tokenPool) const {
    if (IsAll) {
        return "<all>";
    } else if (IsEmpty()) {
        return "<empty>";
    }
    TVector<TString> words;
    for (const TTokenId token : Tokens) {
        words.push_back(tokenPool.PrintWordToken(token));
    }
    return JoinSeq("|", words);
}

void WriteTokenToElementsMap(const TVector<TGrammarElement>& elements, const TVector<TTokenSet>& elementToTokens,
    TTokenToElementsMap* map)
{
    Y_ENSURE(elementToTokens.size() == elements.size());
    Y_ENSURE(map);
    Y_ENSURE(map->ElementSetPool.empty());

    // Write CommonElements.
    for (const auto& [element, tokens] : Zip(elements, elementToTokens)) {
        if (!element.IsEntity() && tokens.IsAll) {
            map->CommonElements.Set(element.Id);
        }
    }

    // Transpose map: elementToTokens -> tokenToElements
    THashMap<TTokenId, TVector<TElementId>> tokenToElements;
    for (const auto& [elementId, tokens] : Enumerate(elementToTokens)) {
        for (const TTokenId token : tokens.Tokens) {
            tokenToElements[token].push_back(static_cast<TElementId>(elementId));
        }
    }

    // Convert THashMap<TTokenId, TVector<TElementId>> to TTokenToElementsMap.

    // Map: set of elements -> offset in TTokenToElementsMap::ElementSetPool.
    THashMap<TVector<TElementId>, ui32, TSimpleRangeHash> elementSetToOffset;

    // Add empty set
    elementSetToOffset[TVector<TElementId>{}] = 0;
    map->ElementSetPool.push_back(UNDEFINED_ELEMENT_ID);

    for (const auto& [token, elementSet] : tokenToElements) {
        const auto [it, isNew] = elementSetToOffset.try_emplace(elementSet, map->ElementSetPool.size());
        if (isNew) {
            Extend(elementSet, &map->ElementSetPool);
            map->ElementSetPool.push_back(UNDEFINED_ELEMENT_ID); // sentinel
        }
        map->TokenToOffset[token] = it->second;
    }
}

// ~~~~ TSpecificWordSet ~~~~

void TSpecificWordSet::AddLazy(const TSpecificWordSet& other) {
    Set.AddLazy(other.Set);
    MaxLogprob = Set.IsAll ? 0 : Max(MaxLogprob, other.MaxLogprob);
}

void TSpecificWordSet::Normalize() {
    Set.Normalize();
}

TString TSpecificWordSet::Print(const TTokenPool& tokenPool) const {
    if (Set.IsAll || Set.IsEmpty()) {
        return Set.Print(tokenPool);
    }
    return Sprintf("{%.3f, %s}", MaxLogprob, Set.Print(tokenPool).c_str());
}

// ~~~~ TOptimizerBySpecificWords ~~~~

TOptimizerBySpecificWords::TOptimizerBySpecificWords(TGrammarData* data, const TTokenPool& tokenPool, IOutputStream* log)
    : Data(*data)
    , Elements(data->Elements)
    , TokenPool(tokenPool)
    , Log(log)
{
    Y_ENSURE(data);
}

void TOptimizerBySpecificWords::Process() {
    DEBUG_TIMER("TOptimizerBySpecificWords::Process");
    Y_ENSURE(ElementToSpecificWords.empty());

    ElementToSpecificWords.resize(Elements.size());
    for (size_t level : xrange(Data.ElementLevelCount)) {
        for (const TGrammarElement& element : Elements) {
            if (element.Level == level) {
                ElementToSpecificWords[element.Id] = FindSpecificWordsOfElement(element);
            }
        }
    }
    DumpResults();
    WriteOptimizationInfo();
}

// Algorithm:
//   SW(word) = {word}
//   SW(a|b|c) = SW(a) + SW(b) + SW(c)
//   SW(a b c ...) = one of SW(a), SW(b), SW(c) with minimal MaxLogprob
//   SW(a*) = all
//   SW(a?) = all
//   SW(a+) = SW(a)
//   SW([a b c]) = SW(a b c)
TSpecificWordSet TOptimizerBySpecificWords::FindSpecificWordsOfElement(const TGrammarElement& element) {
    if (element.IsEntity()) {
        return TSpecificWordSet::MakeAll();
    }
    TSpecificWordSet words;
    if (element.Quantity.MaxCount > 1) {
        words = FindSpecificWordsOfBagElement(element);
    } else {
        words = FindSpecificWordsOfListElement(element);
    }
    words.Normalize();
    return words;
}

TSpecificWordSet TOptimizerBySpecificWords::FindSpecificWordsOfBagElement(const TGrammarElement& element) {
    TSpecificWordSet elementWords = TSpecificWordSet::MakeAll();
    for (const auto& [rule, data] : GetElementRules(element)) {
        if (!HasFlags(element.SetOfRequiredRules, data.GetRuleIndexAsFlag())) {
            continue;
        }
        TSpecificWordSet ruleWords = FindSpecificWordsOfRule(rule);
        if (ruleWords.MaxLogprob < elementWords.MaxLogprob) {
            elementWords = std::move(ruleWords);
        }
    }
    return elementWords;
}

TSpecificWordSet TOptimizerBySpecificWords::FindSpecificWordsOfListElement(const TGrammarElement& element) {
    TSpecificWordSet elementWords = TSpecificWordSet::MakeEmpty();
    for (const auto& [rule, data] : GetElementRules(element)) {
        elementWords.AddLazy(FindSpecificWordsOfRule(rule));
    }
    return elementWords;
}

TSpecificWordSet TOptimizerBySpecificWords::FindSpecificWordsOfRule(const TVector<TTokenId>& rule) {
    TSpecificWordSet ruleWords = TSpecificWordSet::MakeAll();
    for (const TTokenId id : rule) {
        if (NTokenId::IsElement(id)) {
            const TElementId elementId = NTokenId::ToElementId(id);
            if (Elements[elementId].CanSkip) {
                continue;
            }
            const TSpecificWordSet& childWords = ElementToSpecificWords[elementId];
            if (childWords.MaxLogprob < ruleWords.MaxLogprob) {
                ruleWords = childWords;
            }
            continue;
        }
        const float logprob = GetTokenLogprob(id);
        if (logprob < ruleWords.MaxLogprob) {
            ruleWords = TSpecificWordSet::MakeByToken(id, logprob);
        }
    }
    return ruleWords;
}

float TOptimizerBySpecificWords::GetTokenLogprob(TTokenId id) {
    DEBUG_TIMER("TOptimizerBySpecificWords::GetTokenLogprob");
    if (id == TOKEN_WILDCARD) {
        return 0;
    }
    Y_ENSURE(id != TOKEN_UNKNOWN);
    Y_ENSURE(NTokenId::IsWord(id));
    float& logprob = TokenLogprobs[id];
    if (logprob == 0) {
        logprob = NWordLogProbTable::GetWordLogProb(NTokenId::IsLemmaWord(id), TokenPool.GetWord(id), Data.Domain.Lang);
    }
    return logprob;
}

const TRuleTrie& TOptimizerBySpecificWords::GetElementRules(const TGrammarElement& element) const {
    Y_ENSURE(element.RuleTrieIndex != NPOS);
    return Data.RuleTriePool[element.RuleTrieIndex];
}

void TOptimizerBySpecificWords::DumpResults() const {
    if (!Log) {
        return;
    }
    *Log << "TOptimizerBySpecificWords results:" << Endl;
    for (const auto& [element, set] : Zip(Elements, ElementToSpecificWords)) {
        *Log << "  " << RightPad(element.Name + ": ", 20) << set.Print(TokenPool) << Endl;
    }
}

void TOptimizerBySpecificWords::WriteOptimizationInfo() {
    TVector<TTokenSet> sets(Reserve(ElementToSpecificWords.size()));
    for (const TSpecificWordSet& words : ElementToSpecificWords) {
        sets.push_back(words.Set);
    }
    WriteTokenToElementsMap(Elements, sets, &Data.OptimizationInfo.SpecificWordToElements);
}

// ~~~~ TOptimizerByFirstWords ~~~~

TOptimizerByFirstWords::TOptimizerByFirstWords(TGrammarData* data, const TTokenPool& tokenPool, IOutputStream* log)
    : Data(*data)
    , Elements(data->Elements)
    , TokenPool(tokenPool)
    , Log(log)
{
    Y_ENSURE(data);
}

void TOptimizerByFirstWords::Process() {
    DEBUG_TIMER("TOptimizerByFirstWords::Process");
    Y_ENSURE(ElementToFirstWords.empty());

    ElementToFirstWords.resize(Elements.size());
    for (size_t level : xrange(Data.ElementLevelCount)) {
        for (const TGrammarElement& element : Elements) {
            if (element.Level == level) {
                ElementToFirstWords[element.Id] = FindFirstWordsOfElement(element);
            }
        }
    }
    DumpResults();
    WriteOptimizationInfo();
}

// Algorithm:
//   FW(word) = {word}
//   FW(a|b|c) = FW(a) + FW(b) + FW(c)
//   FW([a b c]) = FW(a) + FW(b) + FW(c)
//   FW(a b c d ...) = FW(a) + FW(b) + FW(c), if flag CanSkip for (a, b, c, d, ...) is (true, true, false, any, ...)
//   FW(element) = all, if element has flag EF_ENABLE_EDGE_FILLERS
//   FW(entity) = all
TTokenSet TOptimizerByFirstWords::FindFirstWordsOfElement(const TGrammarElement& element) {
    if (element.IsEntity()) {
        return TTokenSet::MakeAll();
    }
    if (element.Flags.HasFlags(EF_ENABLE_EDGE_FILLERS)) {
        return TTokenSet::MakeAll();
    }
    TTokenSet result;
    for (const auto& [rule, data] : GetElementRules(element)) {
        for (const TTokenId id : rule) {
            if (!NTokenId::IsElement(id)) {
                result.AddLazy(id);
                break;
            }
            const TElementId elementId = NTokenId::ToElementId(id);
            result.AddLazy(ElementToFirstWords[elementId]);
            if (result.IsAll) {
                break;
            }
            if (!Elements[elementId].CanSkip) {
                break;
            }
        }
        if (result.IsAll) {
            break;
        }
    }
    result.Normalize();
    if (result.Tokens.size() > 10) {
        result = TTokenSet::MakeAll();
    }
    return result;
}

const TRuleTrie& TOptimizerByFirstWords::GetElementRules(const TGrammarElement& element) const {
    Y_ENSURE(element.RuleTrieIndex != NPOS);
    return Data.RuleTriePool[element.RuleTrieIndex];
}

void TOptimizerByFirstWords::DumpResults() const {
    if (!Log) {
        return;
    }
    *Log << "TOptimizerByFirstWords results:" << Endl;
    for (const auto& [element, set] : Zip(Elements, ElementToFirstWords)) {
        *Log << "  " << RightPad(element.Name + ": ", 20) << set.Print(TokenPool) << Endl;
    }
}

void TOptimizerByFirstWords::WriteOptimizationInfo() {
    WriteTokenToElementsMap(Elements, ElementToFirstWords, &Data.OptimizationInfo.FirstWordToElements);
}

} // namespace NGranet::NCompiler::NOptimizer
