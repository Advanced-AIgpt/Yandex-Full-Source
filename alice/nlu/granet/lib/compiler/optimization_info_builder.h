#pragma once

#include <alice/nlu/granet/lib/grammar/grammar_data.h>
#include <alice/nlu/granet/lib/grammar/rule_trie.h>
#include <alice/nlu/granet/lib/grammar/token_id.h>
#include <library/cpp/langs/langs.h>
#include <util/digest/sequence.h>
#include <util/generic/deque.h>

namespace NGranet::NCompiler::NOptimizer {

// ~~~~ TTokenSet ~~~~

class TTokenSet {
public:
    bool IsAll = false;
    TVector<TTokenId> Tokens;

public:
    static TTokenSet MakeEmpty() {
        return {false, {}};
    }

    static TTokenSet MakeAll() {
        return {true, {}};
    }

    bool IsEmpty() const {
        return !IsAll && Tokens.empty();
    }

    void AddLazy(TTokenId id);
    void AddLazy(const TTokenSet& other);
    void Normalize();

    TString Print(const TTokenPool& tokenPool) const;
};

void WriteTokenToElementsMap(const TVector<TGrammarElement>& elements, const TVector<TTokenSet>& elementToWords,
    TTokenToElementsMap* map);

// ~~~~ TSpecificWordSet ~~~~

class TSpecificWordSet {
public:
    TTokenSet Set;
    float MaxLogprob = -Max<float>();

public:
    static TSpecificWordSet MakeEmpty() {
        return {};
    }

    static TSpecificWordSet MakeAll() {
        return {{true, {}}, 0};
    }

    static TSpecificWordSet MakeByToken(TTokenId token, float logprop) {
        return {{false, {token}}, logprop};
    }

    void AddLazy(const TSpecificWordSet& other);
    void Normalize();

    TString Print(const TTokenPool& tokenPool) const;
};

// ~~~~ TOptimizerBySpecificWords ~~~~

class TOptimizerBySpecificWords {
public:
    TOptimizerBySpecificWords(TGrammarData* data, const TTokenPool& tokenPool, IOutputStream* log = nullptr);

    void Process();

private:
    void FindSpecificWords();
    TSpecificWordSet FindSpecificWordsOfElement(const TGrammarElement& element);
    TSpecificWordSet FindSpecificWordsOfBagElement(const TGrammarElement& element);
    TSpecificWordSet FindSpecificWordsOfListElement(const TGrammarElement& element);
    TSpecificWordSet FindSpecificWordsOfRule(const TVector<TTokenId>& rule);
    float GetTokenLogprob(TTokenId id);
    const TRuleTrie& GetElementRules(const TGrammarElement& element) const;
    void DumpResults() const;
    void WriteOptimizationInfo();

private:
    TGrammarData& Data;
    const TVector<TGrammarElement>& Elements;
    const TTokenPool& TokenPool;
    IOutputStream* Log = nullptr;
    TVector<TSpecificWordSet> ElementToSpecificWords;
    THashMap<TTokenId, float> TokenLogprobs;
};

// ~~~~ TOptimizerByFirstWords ~~~~

class TOptimizerByFirstWords {
public:
    TOptimizerByFirstWords(TGrammarData* data, const TTokenPool& tokenPool, IOutputStream* log = nullptr);

    void Process();

private:
    void FindFirstWords();
    TTokenSet FindFirstWordsOfElement(const TGrammarElement& element);
    const TRuleTrie& GetElementRules(const TGrammarElement& element) const;
    void WriteMapTokenToElements(const TVector<TTokenSet>& elementToWordSet, TTokenToElementsMap* map);
    void DumpResults() const;
    void WriteOptimizationInfo();

private:
    TGrammarData& Data;
    const TVector<TGrammarElement>& Elements;
    const TTokenPool& TokenPool;
    IOutputStream* Log = nullptr;
    TVector<TTokenSet> ElementToFirstWords;
    THashMap<TTokenId, float> TokenLogprobs;
};

} // namespace NGranet::NCompiler::NOptimizer
