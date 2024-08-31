#pragma once

#include "compiler_data.h"
#include <alice/nlu/granet/lib/grammar/grammar_data.h>
#include <alice/nlu/granet/lib/utils/utils.h>
#include <util/generic/deque.h>
#include <util/generic/vector.h>

namespace NGranet::NCompiler {

// ~~~~

// Write array of TCompilerElement to TGrammarData (as array of TGrammarElement and other additional data)
void WriteGrammarElements(const TDeque<TCompilerElement>& srcElements, TGrammarData* data);

// ~~~~ TElementWriter ~~~~

class TElementWriter : public TMoveOnly {
public:
    TElementWriter(const TCompilerElement& srcElement, TGrammarData* data,
        THashMap<TVector<TElementRuleTag>, NNlu::TInterval>* tagPoolRanges);

    void Write();

private:
    struct TRuleInfo {
        const TCompiledRule* Rule = nullptr;
        TVector<TTokenId> FilteredChain;
        NNlu::TInterval TagPoolRange;
    };

private:
    void WriteRules();
    void CollectRules();
    void ConvertMarkups();
    NNlu::TInterval AddRuleMarkup(const TVector<TElementRuleTag>& markup);
    void SortRules();
    void RemoveConflictedRules();
    void CalculateLogProb();
    void WriteData();
    void WriteMarkups();
    void WriteFlags();
    void CollectChildren();
    void BuildRuleTrie();
    void SetRuleTrie(size_t trieIndex);

private:
    const TCompilerElement& SrcElement;
    TGrammarData* Data = nullptr;
    THashMap<TVector<TElementRuleTag>, NNlu::TInterval>* TagPoolRanges = nullptr;
    TGrammarElement* Element = nullptr;
    TVector<TRuleInfo> Rules;
};

} // namespace NGranet::NCompiler
