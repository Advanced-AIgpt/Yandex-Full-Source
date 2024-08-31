#pragma once

#include <util/system/defaults.h>
#include <alice/cuttlefish/library/digest/murmur.h>


namespace NSM {


struct TState {
    constexpr TState() = default;

    constexpr TState(const TState&) = default;

    constexpr TState(uint64_t state) : State(state) { }

    constexpr operator uint64_t() const {
        return State;
    }

private:
    uint64_t State { 0 };
};


template <class T>
struct TStateBase : TState {
    using TState::TState;

    TStateBase()
        : TState(T::Id)
    { }
};


#define NSM_STATE(x) \
    static constexpr const char* Name = x; \
    static constexpr const uint64_t Id = NTL::MurmurHash64(x);


struct TInitState : TStateBase<TInitState> {
    NSM_STATE("ev.state.init");
};


struct TRunningState : TStateBase<TRunningState> {
    NSM_STATE("ev.state.running");
};


struct TFinalState : TStateBase<TFinalState> {
    NSM_STATE("ev.state.final");
};

}   // namespace NSM
