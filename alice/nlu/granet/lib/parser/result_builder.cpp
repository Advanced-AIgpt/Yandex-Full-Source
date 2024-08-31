#include "result_builder.h"
#include <alice/nlu/granet/lib/sample/entity_utils.h>
#include <alice/nlu/granet/lib/utils/string_utils.h>
#include <library/cpp/iterator/enumerate.h>
#include <library/cpp/scheme/scheme.h>
#include <util/generic/array_ref.h>
#include <util/generic/is_in.h>
#include <util/generic/map.h>
#include <util/generic/stack.h>
#include <util/generic/xrange.h>
#include <util/string/join.h>
#include <util/string/split.h>

using namespace NNlu;

namespace NGranet {

// ~~~~ TParserResultBuilder ~~~~

TParserResultBuilder::TParserResultBuilder(const TPreprocessedSample::TConstRef& preprocessedSample,
        const TParserTask& task, const TVector<TParserStateList>& chart)
    : PreprocessedSample(preprocessedSample)
    , Grammar(preprocessedSample->GetGrammar())
    , GrammarData(preprocessedSample->GetGrammar()->GetData())
    , Sample(preprocessedSample->GetSample())
    , VertexCount(preprocessedSample->GetVertices().ysize())
    , Task(task)
    , Chart(chart)
{
    Y_ENSURE(PreprocessedSample);
    Y_ENSURE(Grammar);
    Y_ENSURE(Sample);
    Y_ENSURE(Task.Root != UNDEFINED_ELEMENT_ID);
    Root = &GrammarData.Elements[Task.Root];
}

TParserTaskResult::TRef TParserResultBuilder::BuildResult() const {
    if (Task.Type == PTT_FORM) {
        return BuildFormResult();
    } else if (Task.Type == PTT_ENTITY) {
        return BuildEntityResult();
    } else {
        Y_ENSURE(false);
        return nullptr;
    }
}

// ~~~~ BuildEntityResult ~~~~

TParserEntityResult::TRef TParserResultBuilder::BuildEntityResult() const {
    Y_ENSURE(Task.Type == PTT_ENTITY);

    TVector<TEntity> entities = CollectResultEntities();

    Sort(entities);

    ReemoveOverlappedEntities(&entities);
    ReemoveDuplicatedEntities(&entities);

    return TParserEntityResult::Create(Task.Name, Task.IsInternal, Sample, Grammar, std::move(entities));
}

TVector<TEntity> TParserResultBuilder::CollectResultEntities() const {
    TVector<TEntity> entities;
    for (const TParserStateList& stateList : Chart) {
        for (const TParserState& state : stateList.GetLevels()[Root->Level]) {
            if (state.Element != Root
                || !state.IsComplete()
                || state.IsNegative())
            {
                continue;
            }

            const size_t entityCountBefore = entities.size();
            CollectResultEntities(state, &entities);

            if (entities.size() > entityCountBefore) {
                continue;
            }

            // Entity without value
            TEntity& entity = entities.emplace_back();
            entity.Interval = state.Interval.ToInterval<size_t>();
            entity.Type = Task.Name;
            entity.Source = GrammarData.ExternalSource.GetOrElse(NEntitySources::GRANET);
            entity.LogProbability = state.LogProb;
        }
    }
    return entities;
}

void TParserResultBuilder::CollectResultEntities(const TParserState& state, TVector<TEntity>* entities) const {
    TryCreateResultEntity(state, entities);

    // Recursion
    for (const TParserState* prefix = &state; prefix != nullptr; prefix = prefix->Prev) {
        const TParserState* child = prefix->PassedChild;
        if (child == nullptr) {
            continue;
        }
        CollectResultEntities(*child, entities);
    }
}

void TParserResultBuilder::TryCreateResultEntity(const TParserState& state, TVector<TEntity>* entities) const {
    Y_ENSURE(state.IsComplete());
    Y_ENSURE(entities);

    if (state.Interval.Empty()) {
        return;
    }
    for (const TResultSlotValue& item : GetRuleData(state)) {
        if (!item.Type.empty() && item.Type != Task.Name) {
            continue;
        }
        TEntity& entity = entities->emplace_back();
        entity.Interval = item.Interval;
        entity.Type = Task.Name;
        entity.Value = item.Value;
        entity.Source = GrammarData.ExternalSource.GetOrElse(NEntitySources::GRANET);
        entity.LogProbability = state.LogProb;
    }
}

static bool IsWorse(const TEntity& entity1, const TEntity& entity2) {
    if (entity1.Interval.Length() != entity2.Interval.Length()) {
        return entity1.Interval.Length() < entity2.Interval.Length();
    }
    return entity1.LogProbability < entity2.LogProbability;
}

void TParserResultBuilder::ReemoveOverlappedEntities(TVector<TEntity>* entities) const {
    if (Task.KeepOverlapped) {
        return;
    }
    for (size_t i = 1; i < entities->size(); i++) {
        const TEntity& entity1 = (*entities)[i - 1];
        const TEntity& entity2 = (*entities)[i];
        if (!entity1.Interval.Overlaps(entity2.Interval)) {
            continue;
        }
        const size_t toErase = IsWorse(entity1, entity2) ? i - 1 : i;
        entities->erase(entities->begin() + toErase);
        i--;
    }
}

void TParserResultBuilder::ReemoveDuplicatedEntities(TVector<TEntity>* entities) const {
    for (size_t i = 1; i < entities->size(); i++) {
        const TEntity& entity1 = (*entities)[i - 1];
        const TEntity& entity2 = (*entities)[i];
        if (entity1.Interval != entity2.Interval
            || entity1.Value != entity2.Value)
        {
            continue;
        }
        const size_t toErase = IsWorse(entity1, entity2) ? i - 1 : i;
        entities->erase(entities->begin() + toErase);
        i--;
    }
}

// ~~~~ BuildFormResult ~~~~

TParserFormResult::TRef TParserResultBuilder::BuildFormResult() const {
    Y_ENSURE(Task.Type == PTT_FORM);
    Y_ENSURE(Chart.ysize() == VertexCount);

    TVector<const TParserState*> states = GetCompleteRootStates();
    if (!Task.KeepVariants) {
        states.crop(1);
    }
    TVector<TParserVariant::TConstRef> variants;
    for (const TParserState* state : states) {
        variants.push_back(BuildVariant(*state));
    }
    return TParserFormResult::Create(Task.Name, Task.IsInternal, Sample, Grammar, std::move(variants));
}

TVector<const TParserState*> TParserResultBuilder::GetCompleteRootStates() const {
    TVector<const TParserState*> states;
    for (const TParserState& state : Chart.back().GetLevels()[Root->Level]) {
        if (state.Element == Root
            && state.Interval.Begin == 0
            && state.IsComplete()
            && !state.IsDisabled
            && !state.IsNegative())
        {
            states.push_back(&state);
        }
    }
    SortBy(states, [](const TParserState* state) {return -state->LogProb;});
    return states;
}

TParserVariant::TConstRef TParserResultBuilder::BuildVariant(const TParserState& state) const {
    TParserVariant::TRef variant = MakeIntrusive<TParserVariant>();
    variant->Sample = Sample;
    variant->LogProb = state.LogProb;
    variant->ElementTree = BuildElementTree(state);
    variant->Slots = BuildSlots(state);
    return variant;
}

TElementOccurrence::TRef TParserResultBuilder::BuildElementTree(const TParserState& state) const {
    Y_ENSURE(state.IsComplete());

    // Create occurrence of current element
    TElementOccurrence::TRef occurrence = MakeIntrusive<TElementOccurrence>();
    occurrence->Interval = state.Interval.ToInterval<size_t>();
    occurrence->ElementId = state.Element->Id;
    occurrence->LogProb = state.LogProb;

    // Create occurrence of children
    for (const TParserState* prefix = &state; prefix != nullptr; prefix = prefix->Prev) {
        const TParserState* child = prefix->PassedChild;
        if (child != nullptr) {
            occurrence->PushChildFront(BuildElementTree(*child));
        }
    }

    return occurrence;
}

TVector<TResultSlotValue> TParserResultBuilder::GetRuleData(const TParserState& state) const {
    TVector<TResultSlotValue> data;

    for (size_t ruleOffset : xrange(state.CompleteRules.RuleCount)) {
        const size_t ruleIndex = state.CompleteRules.RuleIndex + ruleOffset;
        const TGrammarElement& element = *state.Element;

        if (element.IsEntity()) {
            const TEntity& entity = Sample->GetEntities()[ruleIndex];
            data.push_back({
                .Interval = state.Interval.ToInterval<size_t>(),
                .Type = entity.Type,
                .Value = entity.Value
            });
            continue;
        }
        if (element.Flags.HasFlags(EF_HAS_DATA)) {
            data.push_back({
                .Interval = state.Interval.ToInterval<size_t>(),
                .Type = GrammarData.StringPool[element.DataTypes[ruleIndex]],
                .Value = GrammarData.StringPool[element.DataValues[ruleIndex]],
            });
            continue;
        }
    }
    return data;
}

// ~~~~ Build slots ~~~~

TVector<TResultSlot> TParserResultBuilder::BuildSlots(const TParserState& state) const {
    if (Task.Slots.empty()) {
        return {};
    }
    TVector<TSlotMarkup> slots;
    FindSlots(state, &slots);
    StableSortBy(slots, [] (const TSlotMarkup& slot) {return slot.Interval;});
    return FillSlots(state, slots);
}

void TParserResultBuilder::FindSlots(const TParserState& state, TVector<TSlotMarkup>* slots) const {
    Y_ENSURE(state.IsComplete());
    Y_ENSURE(slots);

    for (const TParserState* prefix = &state; prefix != nullptr; prefix = prefix->Prev) {
        const TParserState* child = prefix->PassedChild;
        if (child != nullptr) {
            FindSlots(*child, slots);
        }
    }
    FindSlotsByWholeElements(state, slots);
    FindSlotsByRules(state, slots);
}

void TParserResultBuilder::FindSlotsByWholeElements(const TParserState& state, TVector<TSlotMarkup>* slots) const {
    Y_ENSURE(state.IsComplete());
    Y_ENSURE(slots);

    for (const TSlotDescriptionId& slotId : state.Element->SourceForSlots) {
        if (slotId.TaskIndex != Task.Index || slotId.TaskType != Task.Type) {
            continue;
        }
        TSlotMarkup slot;
        slot.Interval = state.Interval.ToInterval<size_t>();
        slot.Name = Task.Slots[slotId.SlotIndex].Name;
        slots->push_back(slot);
    }
}

void TParserResultBuilder::FindSlotsByRules(const TParserState& state, TVector<TSlotMarkup>* slots) const {
    Y_ENSURE(slots);

    const TGrammarElement& element = *state.Element;
    if (!element.Flags.HasFlags(EF_HAS_TAGS)) {
        return;
    }

    for (const TParserState* prefix = &state; prefix != nullptr; prefix = prefix->Prev) {
        if (prefix->CompleteRules.RuleCount == 0) {
            // Skip until reach the end of previous complete iteration of multi-element (bag
            // or element with quantifiers)
            continue;
        }
        const NNlu::TInterval& poolRange = element.TagPoolRanges[prefix->CompleteRules.RuleIndex];
        for (size_t i = poolRange.Begin; i < poolRange.End; i++) {
            const TElementRuleTag& tag = GrammarData.TagPool[i];
            TSlotMarkup slot;
            slot.Interval = ConvertIntervalFromRuleToSample(*prefix, tag.Interval);
            if (slot.Interval.Empty()) {
                continue;
            }
            slot.ReadMarkup(GrammarData.StringPool[tag.Tag]);
            slots->push_back(slot);
        }
    }
}

NNlu::TInterval TParserResultBuilder::ConvertIntervalFromRuleToSample(const TParserState& state,
    const NNlu::TBaseInterval<ui16>& src) const
{
    NNlu::TIntInterval dest = {Min<int>(), Max<int>()};

    for (const TParserState* prefix = state.Prev; prefix != nullptr; prefix = prefix->Prev) {
        if (prefix->IsWaitingForChild()) {
            continue;
        }
        const ui16 depth = prefix->TrieIteratorDepth;
        const int pos = prefix->Interval.End;

        // Max and Min used to skip fillers
        if (depth == src.Begin) {
            dest.Begin = Max(dest.Begin, pos);
        }
        if (depth == src.End) {
            dest.End = Min(dest.End, pos);
        }

        // Process only last iteration of bag element.
        if (depth == 0) {
            break;
        }
    }
    if (dest.Begin < 0 || dest.End > VertexCount - 1) {
        Y_ASSERT(false);
        return {};
    }
    return dest.ToInterval<size_t>();
}

TVector<TResultSlot> TParserResultBuilder::FillSlots(const TParserState& state, const TVector<TSlotMarkup>& markups) const {
    if (markups.empty()) {
        return {};
    }

    TVector<TResultSlotValue> valuesFromRules;
    CollectValuesFromRules(state, &valuesFromRules);

    TVector<TResultSlot> resultSlots;

    for (const TSlotMarkup& markup : markups) {
        for (const TSlotDescription& description : Task.Slots) {
            if (description.Name != markup.Name) {
                continue;
            }
            TResultSlot& resultSlot = resultSlots.emplace_back();
            resultSlot.Interval = markup.Interval;
            resultSlot.Name = markup.Name;
            resultSlot.Data = FindSlotData(markup, description, valuesFromRules);
            RemoveDuplicatedSlotData(&resultSlot.Data);
        }
    }
    return resultSlots;
}

void TParserResultBuilder::CollectValuesFromRules(const TParserState& state, TVector<TResultSlotValue>* values) const {
    Y_ENSURE(state.IsComplete());
    Y_ENSURE(values);

    for (const TParserState* prefix = &state; prefix != nullptr; prefix = prefix->Prev) {
        const TParserState* child = prefix->PassedChild;
        if (child != nullptr) {
            CollectValuesFromRules(*child, values);
        }
    }

    Extend(GetRuleData(state), values);
}

TVector<TResultSlotValue> TParserResultBuilder::FindSlotData(const TSlotMarkup& slot, const TSlotDescription& description,
    const TVector<TResultSlotValue>& valuesFromRules) const
{
    // Have explicit value and type from rule markup.
    if (slot.HasValues() && slot.HasTypes()) {
        return {TResultSlotValue{slot.Interval, slot.Types[0], slot.Values[0]}};
    }

    // Have explicit value from rule markup.
    if (slot.HasValues()) {
        if (description.DataTypes.empty()) {
            return {};
        }
        return {TResultSlotValue{slot.Interval, description.DataTypes[0], slot.Values[0]}};
    }

    // Have explicit type from rule markup.
    if (slot.HasTypes()) {
        return FindSlotValues(slot.Interval, slot.Types[0], description.MatchingType, valuesFromRules);
    }

    // Have just slot name and interval.
    TVector<TResultSlotValue> data;
    for (const TString& type : description.DataTypes) {
        Extend(FindSlotValues(slot.Interval, type, description.MatchingType, valuesFromRules), &data);
    }
    return data;
}

TVector<TResultSlotValue> TParserResultBuilder::FindSlotValues(const NNlu::TInterval& slotInterval, TStringBuf slotType,
    ESlotMatchingType matchingType, const TVector<TResultSlotValue>& valuesFromRules) const
{
    // {is_explicit, jaccard_index, -value_interval_begin, log_probability}
    using TValueQuality = std::tuple<bool, float, int, double>;
    TVector<std::pair<TValueQuality, TResultSlotValue>> hypotheses;

    // Values from rules. Example:
    //  $MyElement:
    //      %type "custom.action"
    //      %value "turn_on"
    //      включи  # This rule has data with type "custom.action" and value "turn_on"
    for (const TResultSlotValue& value : valuesFromRules) {
        if (value.Type == slotType && value.Interval.Overlaps(slotInterval)) {
            const TValueQuality quality = {true, JaccardIndex(value.Interval, slotInterval), -value.Interval.Begin, 0};
            hypotheses.push_back({quality, value});
        }
    }

    // Values from entities which interval overlaps interval of slot.
    for (const TEntity* entity : PreprocessedSample->GetEntitiesOfType(slotType)) {
        if (!entity->Interval.Overlaps(slotInterval)) {
            continue;
        }
        const TResultSlotValue value = {entity->Interval, entity->Type, entity->Value};
        const TValueQuality quality = {false, JaccardIndex(value.Interval, slotInterval), -value.Interval.Begin, entity->LogProbability};
        hypotheses.push_back({quality, value});
    }

    SortDescending(hypotheses);
    TVector<TResultSlotValue> data;
    for (const auto& [quality, value] : hypotheses) {
        if (matchingType != SMT_OVERLAP && !slotInterval.Contains(value.Interval)) {
            continue;
        }
        if (matchingType == SMT_EXACT && value.Interval != slotInterval) {
            continue;
        }
        data.push_back(value);
    }

    if (data.empty() && slotType == NEntityTypes::STRING) {
        data.push_back({
            .Interval = slotInterval,
            .Type = NEntityTypes::STRING,
            .Value = Sample->GetTextByIntervalOnTokens(slotInterval)
        });
    }

    return data;
}

// static
void TParserResultBuilder::RemoveDuplicatedSlotData(TVector<TResultSlotValue>* data) {
    Y_ENSURE(data);
    for (int i = data->size() - 1; i > 0; i--) {
        if (IsIn(data->begin(), data->begin() + i, (*data)[i])) {
            data->erase(data->begin() + i);
        }
    }
}

} // namespace NGranet
