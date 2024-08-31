#pragma once

#include <memory>

namespace NAlice::NHollywood::NVoiceprint::NImpl {

template <class TState>
struct THandleResult {
    THandleResult()
        : NewState{nullptr}
        , ContinueProcessing{false}
        , IsIrrelevant{false}
    {}
    
    THandleResult(std::unique_ptr<TState> newState, bool continueProcessing, bool isIrrelevant)
        : NewState{std::move(newState)}
        , ContinueProcessing{continueProcessing}
        , IsIrrelevant{isIrrelevant}
    {}

    std::unique_ptr<TState> NewState;
    bool ContinueProcessing;
    bool IsIrrelevant;
};

} // namespace NAlice::NHollywood::NVoiceprint::NImpl
