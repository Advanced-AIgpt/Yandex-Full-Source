#include "state.h"
#include <library/cpp/iterator/enumerate.h>

namespace NGranet {

// ~~~~ TParserStateKey ~~~~

bool TParserStateKey::operator==(const TParserStateKey& other) const {
    return Interval == other.Interval
        && Element == other.Element
        && TrieIterator == other.TrieIterator
        && SetOfCompleteRulesOfBag == other.SetOfCompleteRulesOfBag
        && ConstrainedIterationCount == other.ConstrainedIterationCount
        && Flags == other.Flags
        && TrieIteratorDepth == other.TrieIteratorDepth;
}

bool TParserStateKey::operator!=(const TParserStateKey& other) const {
    return !(*this == other);
}

size_t TParserStateKey::GetHash() const {
    Y_ASSERT(Element);
    return TrivialHash(Interval.Begin)
        ^ TrivialHash(Interval.End) << 4u
        ^ TrivialHash(Element->Id) << 8u
        ^ CalculateHash(TrieIterator)
        ^ TrivialHash(SetOfCompleteRulesOfBag) << 16u
        ^ TrivialHash(ConstrainedIterationCount) << 24u
        ^ TrivialHash(Flags.ToBaseType()) << 32u
        ^ TrivialHash(TrieIteratorDepth) << 40u;
}

// ~~~~ TParserStateList ~~~~

void TParserStateList::Init(size_t levelCount, size_t stateLimit) {
    Y_ENSURE(StatesOfLevels.empty());
    StatesOfLevels.resize(levelCount);
    StateLimit = stateLimit;
}

const TParserState* TParserStateList::AddBeginningState(const TParserStateKey& key, float solutionLogProbUpperBound) {
    if (!ProcessStateLimit()) {
        return nullptr;
    }

    // Usually beginning states has many alternatives. All of them are unnecessary.
    // We can keep only one beginning state for each group.
    TParserState*& state = BestStates[key];
    if (state == nullptr) {
        state = &AddStateCommon(key, PSET_BEGIN);
        state->BeginningState = state;
        state->SolutionLogProbUpperBound = solutionLogProbUpperBound;
    } else {
        state->SolutionLogProbUpperBound = Max(state->SolutionLogProbUpperBound, solutionLogProbUpperBound);
    }
    return state;
}

void TParserStateList::AddNextState(const TParserStateKey& key, EParserStateEventType event,
    const TParserState& prev, float logProbIncrement, const TParserState* predictedChild,
    const TParserState* passedChild, TRuleIndexes completeRules)
{
    Y_ASSERT(logProbIncrement <= 0);
    Y_ASSERT(event != PSET_BEGIN);

    if (!ProcessStateLimit()) {
        return;
    }

    TParserState& state = AddStateCommon(key, event);
    state.LogProb = prev.LogProb + logProbIncrement;
    state.SolutionLogProbUpperBound = prev.SolutionLogProbUpperBound + logProbIncrement;
    state.CompleteRules = completeRules;
    state.Prev = &prev;
    state.PassedChild = passedChild;
    state.PredictedChild = predictedChild;
    state.BeginningState = prev.BeginningState;
    if (predictedChild != nullptr) {
        state.WaitingParentsList = predictedChild->WaitingParentsList;
        GetWritableState(*predictedChild).WaitingParentsList = &state;
    }

    TrackBest(state);
}

TParserState& TParserStateList::AddStateCommon(const TParserStateKey& key, EParserStateEventType event) {
    StateCount++;
    EnabledStateCount++;
    TDeque<TParserState>& states = StatesOfLevels[key.Element->Level];
    return states.emplace_back(key, event, states.size());
}

TParserState& TParserStateList::GetWritableState(const TParserState& state) {
    // Fair but not optimized way to obtain writable state.
    Y_ASSERT(&StatesOfLevels[state.Element->Level][state.Index] == &state);
    // Optimized way through const_cast.
    return const_cast<TParserState&>(state);
}

// Insert state into the list of alternatives.
void TParserStateList::TrackBest(TParserState& state) {
    // No alternatives
    TParserState*& best = BestStates[state];
    if (best == nullptr) {
        best = &state;
        return;
    }
    Y_ASSERT(best->BeginningState == state.BeginningState);
    // 'state' is new best
    if (state.IsBetterThan(*best)) {
        DisableState(*best);
        best->BetterAlternative = &state;
        state.WorseAlternative = best;
        best = &state;
        return;
    }
    // 'state' is not best
    // find insertion point in the list of alternatives
    DisableState(state);
    TParserState* better = best;
    while (true) {
        TParserState* worse = better->WorseAlternative;
        if (worse == nullptr) {
            // insert to end
            state.BetterAlternative = better;
            better->WorseAlternative = &state;
            break;
        }
        if (state.IsBetterThan(*worse)) {
            // insert between 'worse' and 'better'
            state.WorseAlternative = worse;
            state.BetterAlternative = better;
            worse->BetterAlternative = &state;
            better->WorseAlternative = &state;
            break;
        }
        better = worse;
    }
}

bool TParserStateList::ProcessStateLimit() {
    if (EnabledStateCount < StateLimit) {
        return true;
    }
    StateLimitHasBeenReached = true;

    // State quality for sorting: {state quality, state index}
    using TStateQualityInfo = std::tuple<float, size_t, size_t>;
    TVector<TStateQualityInfo> infos(Reserve(EnabledStateCount));

    for (const auto& [level, states] : Enumerate(StatesOfLevels)) {
        for (const auto& [index, state] : Enumerate(states)) {
            if (!state.IsDisabled) {
                infos.emplace_back(state.SolutionLogProbUpperBound, level, index);
            }
        }
    }
    Y_ASSERT(EnabledStateCount == infos.size());
    SortDescending(infos);

    // Take into account state diversity.
    THashMap<TElementId, int> groupCounters;
    for (auto& [quality, level, index] : infos) {
        const TParserState& state = StatesOfLevels[level][index];
        const float numberInGroup = groupCounters[state.Element->Id]++;
        quality -= numberInGroup * 1000.f;
    }
    SortDescending(infos);

    // Keep enabled StateLimit / 2 states only.
    const size_t reduceTo = Min(infos.size(), StateLimit / 2);
    for (auto& [quality, level, index] : MakeArrayRef(infos).Slice(reduceTo)) {
        DisableState(StatesOfLevels[level][index]);
    }
    Y_ASSERT(EnabledStateCount == reduceTo);

    return StateCount < StateLimit * 100;
}

void TParserStateList::DisableState(TParserState& state) {
    if (!state.IsDisabled) {
        state.IsDisabled = true;
        EnabledStateCount--;
    }
}

bool TParserStateList::Check() const {
    size_t enabledStateCountActual = 0;
    for (const auto& states : StatesOfLevels) {
        for (const auto& state : states) {
            if (!state.IsDisabled) {
                enabledStateCountActual++;
            }
        }
    }
    return enabledStateCountActual == EnabledStateCount;
}

} // namespace NGranet
