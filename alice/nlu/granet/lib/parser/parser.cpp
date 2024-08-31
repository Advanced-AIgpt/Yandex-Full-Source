#include "parser.h"
#include "debug.h"
#include "result_builder.h"
#include "state_dumper.h"
#include <alice/nlu/granet/lib/sample/entity_utils.h>
#include <alice/nlu/granet/lib/utils/flag_utils.h>
#include <alice/nlu/granet/lib/utils/string_utils.h>
#include <alice/nlu/granet/lib/utils/trace.h>
#include <dict/nerutil/tstimer.h>
#include <library/cpp/iterator/enumerate.h>
#include <library/cpp/iterator/zip.h>
#include <util/generic/adaptor.h>
#include <util/generic/array_ref.h>
#include <util/generic/is_in.h>
#include <util/generic/map.h>
#include <util/generic/stack.h>
#include <util/generic/xrange.h>
#include <util/string/join.h>
#include <util/string/split.h>

using namespace NNlu;

namespace NGranet {

// ~~~~ TParser ~~~~

// Maximum number of parser states for each token.
static const size_t STATE_LIMIT = 500;
static const ESynonymFlags DEFAULT_ENABLED_SYNONYMS = ESynonymFlag::SF_TRANSLIT;

TParser::TParser(const TPreprocessedSample::TConstRef& preprocessedSample, const TParserTask& task)
    : PreprocessedSample(preprocessedSample)
    , Grammar(preprocessedSample->GetGrammar())
    , GrammarData(preprocessedSample->GetGrammar()->GetData())
    , Sample(preprocessedSample->GetSample())
    , Vertices(preprocessedSample->GetVertices())
    , VertexCount(preprocessedSample->GetVertices().ysize())
    , Task(task)
{
    Y_ENSURE(PreprocessedSample);
    Y_ENSURE(Grammar);
    Y_ENSURE(Sample);
    Root = GrammarData.MaybeGetElement(Task.Root);
    UserFiller = GrammarData.MaybeGetElement(Task.UserFiller);
    AutoFiller = GrammarData.MaybeGetElement(Task.AutoFiller);
}

TParser& TParser::SetLog(IOutputStream* log, bool isVerbose) {
    Log = log;
    IsLogVerbose = isVerbose;
    return *this;
}

TParser& TParser::SetNeedDebugInfo(bool value) {
    NeedDebugInfo = value;
    return *this;
}

TParser& TParser::SetCollectBlockersMode(bool value) {
    CollectBlockersMode = value;
    return *this;
}

TParserTaskResult::TRef TParser::Parse() {
    DumpInitialization();
    if (!IsTaskPromising()) {
        TRACE_LINE(Log, "  Task is unpromising")
        return CreateParserEmptyResult(Task, Sample, Grammar);
    }
    PrepareChart();
    for (int pos = 0; pos < VertexCount; pos++) {
        ProcessPosition(pos);
    }
    return BuildResult();
}

void TParser::DumpInitialization() const {
    if (!Log) {
        return;
    }
    *Log << "TParser:" << Endl;
    *Log << "  Task:" << Endl;
    Task.Dump(*Grammar, Log, "    ");
    *Log << "  PreprocessedSample:" << Endl;
    PreprocessedSample->Dump(Log, "    ");
}

bool TParser::IsTaskPromising() const {
    if (Task.Root == UNDEFINED_ELEMENT_ID) {
        return false;
    }
    return CollectBlockersMode ||
        PreprocessedSample->GetPromisingRootElements().Get(Task.Root);
}

bool TParser::IsElementPromising(int pos, TElementId id) const {
    return CollectBlockersMode ||
        Vertices[pos].PromisingElements.Get(id);
}

void TParser::PrepareChart() {
    Y_ENSURE(Root);

    size_t stateLimit = Min(STATE_LIMIT, STATE_LIMIT * 10 / VertexCount);
    if (CollectBlockersMode) {
        stateLimit *= 100; // Almost unlimited
    }
    PrepareChart(false, Root->Level + 1, stateLimit);

    ui32 fillerMaxLevel = 0;
    if (UserFiller != nullptr) {
        fillerMaxLevel = Max(fillerMaxLevel, UserFiller->Level);
    }
    if (AutoFiller != nullptr) {
        fillerMaxLevel = Max(fillerMaxLevel, AutoFiller->Level);
    }
    PrepareChart(true, fillerMaxLevel + 1, stateLimit);
}

void TParser::PrepareChart(bool isFiller, size_t levelCount, size_t stateLimit) {
    TVector<TParserStateList>& chart = GetChart(isFiller);
    Y_ENSURE(chart.empty());
    chart.resize(VertexCount);
    for (TParserStateList& list : chart) {
        list.Init(levelCount, stateLimit);
    }
}

const TVector<TParserStateList>& TParser::GetChart(bool isFiller) const {
    return isFiller ? FillerChart : MainChart;
}

TVector<TParserStateList>& TParser::GetChart(bool isFiller) {
    return isFiller ? FillerChart : MainChart;
}

void TParser::ProcessPosition(int pos) {
    if (Task.Type == PTT_ENTITY || pos == 0) {
        BeginRoot(pos);
    }
    if (MainChart[pos].GetStateCount() == 0 && FillerChart[pos].GetStateCount() == 0) {
        return;
    }
    ProcessPosition(true, pos);
    ProcessPosition(false, pos);
}

void TParser::ProcessPosition(bool isFiller, int pos) {
    if (isFiller && UserFiller == nullptr && AutoFiller == nullptr) {
        return;
    }

    const TParserStateList& stateList = GetChart(isFiller)[pos];
    const int levelCount = stateList.GetLevels().ysize();
    TVector<size_t> doneCounters(levelCount);

    // Upward pass for Complete and Dispatch.
    if (stateList.GetStateCount() > 0) {
        for (int level = 0; level < levelCount; level++) {
            const TDeque<TParserState>& states = stateList.GetLevels()[level];

            // Can't use range based for loop, because state list is growing during this loop.
            for (size_t s = 0; s < states.size(); s++) {
                const TParserState& state = states[s];
                Predict(isFiller, state);
                Complete(isFiller, state);
                Scan(isFiller, state);
            }
            doneCounters[level] = states.size();

            // 'Dispatch' is performed after all completions to make sure that all negative rules (on this level)
            // have been completed and are able to block less probable positive rules.
            for (size_t s = 0; s < states.size(); s++) {
                Dispatch(isFiller, states[s]);
            }
            // 'Dispatch' creates states on upper levels only.
            Y_ENSURE(doneCounters[level] == states.size());
        }
    }

    if (isFiller && MainChart[pos].GetStateCount() > 0) {
        if (UserFiller != nullptr) {
            AddBeginningState(true, pos, *UserFiller, 0);
        }
        if (AutoFiller != nullptr) {
            AddBeginningState(true, pos, *AutoFiller, 0);
        }
    }

    // Downward pass to process new states predicted in upward pass.
    if (stateList.GetStateCount() > 0) {
        for (int level = levelCount - 1; level >= 0; level--) {
            const TDeque<TParserState>& states = stateList.GetLevels()[level];

            // Can't use range based for loop, because state list is growing during this loop.
            for (size_t s = doneCounters[level]; s < states.size(); s++) {
                const TParserState& state = states[s];
                Y_ENSURE(state.Interval.Empty()); // hence 'Complete' and 'Dispatch' not needed
                Predict(isFiller, state);
                Scan(isFiller, state);
            }
        }
    }

    if (Log && IsLogVerbose) {
        StateDumper.DumpStateList(isFiller, pos, stateList, Log, "  ");
    }
}

void TParser::BeginRoot(int pos) {
    Y_ENSURE(Root);
    const TParserState* state = AddBeginningState(false, pos, *Root, 0);
    if (state == nullptr) {
        return;
    }
    if (VertexCount <= 1 && Root->CanSkip && Task.Type == PTT_FORM) {
        TParserStateKey completed = *state;
        completed.Flags |= PSKF_COMPLETE;
        AddNextState(false, completed, PSET_COMPLETE, *state, Root->LogProbOfSkip);
    }
}

const TParserState* TParser::AddBeginningState(bool isFiller, int pos, const TGrammarElement& element,
    float solutionLogProbUpperBound)
{
    if (!IsElementPromising(pos, element.Id)) {
        return nullptr;
    }
    TParserStateKey key;
    key.Interval = {pos, pos};
    key.Element = &element;
    key.TrieIterator = element.RulesBeginIterator;
    return GetChart(isFiller)[pos].AddBeginningState(key, solutionLogProbUpperBound);
}

// If state located in chain before some child element, generate all begin states for that element.
void TParser::Predict(bool isFiller, const TParserState& state) {
    if (state.IsWaitingForChild() || state.IsComplete() || state.IsDisabled) {
        return;
    }
    const TGrammarElement& element = *state.Element;
    if (element.ElementsInRules.empty()) {
        return;
    }
    const int pos = state.Interval.End;

    for (const TElementId childId : element.ElementsInRules) {
        const TGrammarElement& child = GrammarData.Elements[childId];
        if (!IsElementPromising(pos, childId) && !child.CanSkip) {
            continue;
        }
        TSearchIterator<TRuleTrie> iterator = state.TrieIterator;
        if (!iterator.Advance(NTokenId::FromElementId(childId))) {
            continue;
        }
        if (child.Flags.HasFlags(EF_ANCHOR_TO_BEGIN) && pos > 0) {
            continue;
        }

        // Try skip child.
        if (child.CanSkip) {
            TParserStateKey skipping = state;
            skipping.TrieIterator = iterator;
            skipping.TrieIteratorDepth++;
            AddNextState(isFiller, skipping, PSET_SKIP_CHILD, state, child.LogProbOfSkip);
        }

        // Add begin of predicted child
        const TParserState* childState = AddBeginningState(isFiller, pos, child, state.SolutionLogProbUpperBound);
        if (childState == nullptr) {
            continue;
        }

        // Add state of waiting for complete predicted child
        TParserStateKey waitingKey = state;
        waitingKey.TrieIterator = iterator;
        waitingKey.TrieIteratorDepth++;
        waitingKey.Flags.RemoveFlags(PSKF_PASSED_FILLER);
        waitingKey.Flags |= PSKF_WAITING_FOR_CHILD;
        AddNextState(isFiller, waitingKey, PSET_WAIT_CHILD, state, 0, childState);
    }
}

// If state points to token, try go deeper into the dictionary and generate next state.
void TParser::Scan(bool isFiller, const TParserState& state) {
    if (state.IsWaitingForChild() || state.IsComplete() || state.IsDisabled) {
        return;
    }
    const int pos = state.Interval.End;
    if (pos + 1 >= VertexCount) {
        return;
    }
    const TParserVertex& vertex = Vertices[pos];
    const TGrammarElement& element = *state.Element;

    // Scan inside entity element
    if (element.IsEntity()) {
        const TVector<TParserEntityArc>* arcs = vertex.EntityArcs.FindPtr(element.Id);
        if (!arcs) {
            return;
        }
        for (const TParserEntityArc& arc : *arcs) {
            TParserStateKey next = state;
            next.Interval.End = arc.To;
            next.Flags |= PSKF_HAS_WORDS_IN_CURRENT_ITERATION | PSKF_COMPLETE;
            next.Flags.RemoveFlags(PSKF_PASSED_FILLER);
            AddNextState(isFiller, next, PSET_COMPLETE_ENTITY, state, arc.LogProb, nullptr, nullptr, arc.EntityIndexesInSample);
        }
        return;
    }

    // Scan inside normal element
    for (const TParserTokenArc& arc : vertex.TokenArcs) {
        if (arc.SynonymFlag) {
            ESynonymFlags enabledSynonyms = DEFAULT_ENABLED_SYNONYMS;
            enabledSynonyms |= (Task.EnableSynonymFlagsMask & Task.EnableSynonymFlags);
            enabledSynonyms &= ~(Task.EnableSynonymFlagsMask & ~Task.EnableSynonymFlags);
            enabledSynonyms |= (element.EnableSynonymFlagsMask & element.EnableSynonymFlags);
            enabledSynonyms &= ~(element.EnableSynonymFlagsMask & ~element.EnableSynonymFlags);
            if ((enabledSynonyms & arc.SynonymFlag) == static_cast<ESynonymFlags>(0)) {
                continue;
            }
        }
        TSearchIterator<TRuleTrie> iterator = state.TrieIterator;
        if (!iterator.Advance(arc.Token)) {
            continue;
        }
        TParserStateKey next = state;
        next.TrieIterator = iterator;
        next.TrieIteratorDepth++;
        next.Interval.End = arc.To;
        next.Flags |= PSKF_HAS_WORDS_IN_CURRENT_ITERATION;
        next.Flags.RemoveFlags(PSKF_PASSED_FILLER);
        AddNextState(isFiller, next, PSET_PASS_TOKEN, state, arc.LogProb);
    }
}

// Try create state of complete element.
void TParser::Complete(bool isFiller, const TParserState& state) {
    if (state.IsWaitingForChild() || state.IsComplete() || state.IsDisabled) {
        return;
    }

    // Empty matches processed in method Predict as CanSkip
    if (!state.HasWordsInCurrentIteration()) {
        return;
    }

    // Complete states of entity elements created in Scan stage
    const TGrammarElement& element = *state.Element;
    if (element.IsEntity()) {
        return;
    }

    // Check whether trie iterator points to the end of some rule.
    if (!state.TrieIterator.HasValue()) {
        return;
    }

    // Read data of completed rule.
    TRuleIndexes completeRules;
    state.TrieIterator.GetValue(&completeRules);

    // Prohibit repeating of limited rule of multi-element.
    const ui32 ruleIndexFlag = completeRules.GetRuleIndexAsFlag();

    if ((state.SetOfCompleteRulesOfBag & element.SetOfLimitedRules & ruleIndexFlag) != 0) {
        return;
    }

    float logProb = element.RulesLogProbs[completeRules.RuleIndex];

    // Start new iteration of multi-element
    const int pos = state.Interval.End;
    if (state.ConstrainedIterationCount < element.Quantity.MaxCount) {
        TParserStateKey restarted = state;
        restarted.TrieIterator = element.RulesBeginIterator;
        restarted.TrieIteratorDepth = 0;
        restarted.SetOfCompleteRulesOfBag |= ruleIndexFlag;
        restarted.Flags.RemoveFlags(PSKF_HAS_WORDS_IN_CURRENT_ITERATION);
        // If ConstrainedIterationCount not needed dont use it. To reduce number of different TParserStateKey.
        if (restarted.ConstrainedIterationCount < element.Quantity.MinCount
            || element.Quantity.MaxCount < Max<ui8>())
        {
            restarted.ConstrainedIterationCount++;
        }
        AddNextState(isFiller, restarted, PSET_RESTART, state, logProb, nullptr, nullptr, completeRules);
    }

    // Try generate completed state
    if (element.Flags.HasFlags(EF_ANCHOR_TO_END) && pos < VertexCount - 1) {
        return;
    }
    if (!element.Flags.HasFlags(EF_ENABLE_EDGE_FILLERS) && state.IsPassedFiller()) {
        return;
    }
    if (state.ConstrainedIterationCount < element.Quantity.MinCount) {
        return;
    }

    if (ui32 incompleteRequiredRules = element.SetOfRequiredRules & ~(state.SetOfCompleteRulesOfBag | ruleIndexFlag)) {
        if (!HasFlags(element.SetOfPossibleEmptyRequiredRules, incompleteRequiredRules)) {
            return;
        }
        for (const auto& [skippedRuleFlag, logProbOfSkip] : element.LogProbOfRequiredRulesSkip) {
            if (HasFlags(incompleteRequiredRules, skippedRuleFlag)) {
                logProb += logProbOfSkip;
                incompleteRequiredRules &= ~skippedRuleFlag;
            }
        }
        Y_ASSERT(incompleteRequiredRules == 0);
    }

    TParserStateKey completed = state;
    // Reset not needed fields. To reduce number of different TParserStateKey.
    completed.TrieIterator = state.Element->RulesBeginIterator;
    completed.TrieIteratorDepth = 0;
    completed.SetOfCompleteRulesOfBag = 0;
    completed.ConstrainedIterationCount = 0;
    completed.Flags.RemoveFlags(PSKF_HAS_WORDS_IN_CURRENT_ITERATION | PSKF_PASSED_FILLER);
    completed.Flags |= PSKF_COMPLETE;
    AddNextState(isFiller, completed, PSET_COMPLETE, state, logProb, nullptr, nullptr, completeRules);
}

// If state points to the end of element, make one step for parent elements.
void TParser::Dispatch(bool isFiller, const TParserState& state) {
    if (!state.IsComplete() || state.IsNegative() || state.IsDisabled) {
        return;
    }
    if (state.Element == UserFiller || state.Element == AutoFiller) {
        Y_ASSERT(isFiller);
        DispatchFiller(state);
        return;
    }
    DispatchNormal(isFiller, state);
}

void TParser::DispatchNormal(bool isFiller, const TParserState& child) {
    const TParserState* beginning = child.BeginningState;
    for (const TParserState* waiting = beginning->WaitingParentsList; waiting != nullptr;
        waiting = waiting->WaitingParentsList)
    {
        Y_ASSERT(waiting->IsWaitingForChild());
        Y_ASSERT(waiting->PredictedChild == beginning);
        if (waiting->IsDisabled) {
            continue;
        }
        TParserStateKey next = *waiting;
        next.Interval.End = child.Interval.End;
        if (!child.Interval.Empty()) {
            next.Flags |= PSKF_HAS_WORDS_IN_CURRENT_ITERATION;
        }
        next.Flags.RemoveFlags(PSKF_WAITING_FOR_CHILD | PSKF_PASSED_FILLER);
        AddNextState(isFiller, next, PSET_PASS_CHILD, *waiting, child.LogProb, nullptr, &child);
    }
}

void TParser::DispatchFiller(const TParserState& filler) {
    Y_ASSERT(filler.IsComplete());
    Y_ASSERT(filler.Element == UserFiller || filler.Element == AutoFiller);

    if (filler.Interval.Empty()) {
        return;
    }
    for (const TDeque<TParserState>& states : MainChart[filler.Interval.Begin].GetLevels()) {
        for (const TParserState& waiting : states) {
            if (!CanPassFiller(waiting)) {
                continue;
            }
            TParserStateKey next = waiting;
            next.Interval.End = filler.Interval.End;
            next.Flags |= PSKF_HAS_WORDS_IN_CURRENT_ITERATION | PSKF_PASSED_FILLER;
            const float penalty = waiting.Element->Flags.HasFlags(EF_COVER_FILLERS)
                ? COVERED_FILLER_LOG_PROB : FILLER_LOG_PROB;
            AddNextState(false, next, PSET_PASS_FILLER, waiting, filler.LogProb + penalty, nullptr, &filler);
        }
    }
}

bool TParser::CanPassFiller(const TParserState& state) const {
    if (state.IsWaitingForChild() || state.IsComplete() || state.IsDisabled) {
        return false;
    }
    const TGrammarElement& element = *state.Element;
    if (element.IsEntity()) {
        return false;
    }
    if (!element.Flags.HasFlags(EF_ENABLE_FILLERS)) {
        return false;
    }
    if (!element.Flags.HasFlags(EF_ENABLE_EDGE_FILLERS)
        && state.TrieIterator == element.RulesBeginIterator)
    {
        return false;
    }
    // Just restarted multi-element. Filler was added to previous state before restarting.
    if (!state.Interval.Empty() && !state.HasWordsInCurrentIteration()) {
        return false;
    }
    return true;
}

void TParser::AddNextState(bool isFiller, const TParserStateKey& key, EParserStateEventType type,
    const TParserState& prev, float logProbIncrement, const TParserState* predictedChild,
    const TParserState* passedChild, TRuleIndexes completeRules)
{
    GetChart(isFiller)[key.Interval.End].AddNextState(key, type, prev, logProbIncrement,
        predictedChild, passedChild, completeRules);
}

// ~~~~ Getting of results ~~~~

TParserTaskResult::TRef TParser::BuildResult() {
    TParserTaskResult::TRef result = TParserResultBuilder(PreprocessedSample, Task, MainChart).BuildResult();
    if (NeedDebugInfo) {
        result->SetDebugInfo(MakeIntrusive<TParserDebugInfo>(Grammar, Sample, std::move(MainChart), std::move(FillerChart)));
    }
    if (Log) {
        *Log << "  Result:" << Endl;
        result->Dump(Log, "    ");
    }
    return result;
}

} // namespace NGranet
