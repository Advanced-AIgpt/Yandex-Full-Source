#pragma once

#include "preprocessed_sample.h"
#include "result.h"
#include "state.h"
#include <alice/nlu/granet/lib/utils/utils.h>
#include <alice/nlu/granet/lib/grammar/grammar.h>
#include <alice/nlu/granet/lib/sample/markup.h>
#include <library/cpp/containers/comptrie/search_iterator.h>

namespace NGranet {

// ~~~~ TParserResultBuilder ~~~~

class TParserResultBuilder : public TMoveOnly {
public:
    TParserResultBuilder(const TPreprocessedSample::TConstRef& preprocessedSample, const TParserTask& task,
        const TVector<TParserStateList>& chart);

    // Abstract result for PTT_FORM or PTT_ENTITY
    TParserTaskResult::TRef BuildResult() const;

private:
    TParserEntityResult::TRef BuildEntityResult() const;
    TVector<TEntity> CollectResultEntities() const;
    void CollectResultEntities(const TParserState& state, TVector<TEntity>* entities) const;
    void TryCreateResultEntity(const TParserState& state, TVector<TEntity>* entities) const;
    void ReemoveOverlappedEntities(TVector<TEntity>* entities) const;
    void ReemoveDuplicatedEntities(TVector<TEntity>* entities) const;

    TParserFormResult::TRef BuildFormResult() const;
    TVector<const TParserState*> GetCompleteRootStates() const;
    TParserVariant::TConstRef BuildVariant(const TParserState& state) const;
    TElementOccurrence::TRef BuildElementTree(const TParserState& state) const;
    void GetRuleData(const TParserState& state, TString* dataType, TString* dataValue) const;
    TVector<TResultSlotValue> GetRuleData(const TParserState& state) const;
    TVector<TResultSlot> BuildSlots(const TParserState& state) const;
    void FindSlots(const TParserState& state, TVector<TSlotMarkup>* slots) const;
    void FindSlotsByWholeElements(const TParserState& state, TVector<TSlotMarkup>* slots) const;
    void FindSlotsByRules(const TParserState& state, TVector<TSlotMarkup>* slots) const;
    NNlu::TInterval ConvertIntervalFromRuleToSample(const TParserState& state,
        const NNlu::TBaseInterval<ui16>& src) const;
    TVector<TResultSlot> FillSlots(const TParserState& state, const TVector<TSlotMarkup>& slots) const;
    void CollectValuesFromRules(const TParserState& state, TVector<TResultSlotValue>* values) const;
    TVector<TResultSlotValue> FindSlotData(const TSlotMarkup& slot, const TSlotDescription& description,
        const TVector<TResultSlotValue>& valuesFromRules) const;
    TVector<TResultSlotValue> FindSlotValues(const NNlu::TInterval& slotInterval, TStringBuf slotType,
        ESlotMatchingType matchingType, const TVector<TResultSlotValue>& valuesFromRules) const;
    static void RemoveDuplicatedSlotData(TVector<TResultSlotValue>* data);

private:
    TPreprocessedSample::TConstRef PreprocessedSample;
    TGrammar::TConstRef Grammar;
    const TGrammarData& GrammarData;
    TSample::TConstRef Sample;
    const int VertexCount = 0;
    const TParserTask& Task;
    const TGrammarElement* Root = nullptr;
    const TVector<TParserStateList>& Chart;
};

} // namespace NGranet
