#include <library/cpp/testing/benchmark/bench.h>

#include <alice/cuttlefish/library/evproc/machines.h>
#include <alice/cuttlefish/library/evproc/states.h>
#include <alice/cuttlefish/library/evproc/events.h>


struct TEventInfo {
    int64_t StartEvents = 0;
    int64_t ShutdownEvents = 0;
};


class TSampleMachine
    : public NSM::TStateMachine<
        TSampleMachine,             // T

        TEventInfo,                 // TContext

        TTypeList<
            NSM::TInitState,
            NSM::TRunningState,
            NSM::TFinalState
        >,                          // TStates

        TTypeList<
            NSM::TStartEv,
            NSM::TShutdownEv
        >                           // TEvents
    >
{
public:
    TSampleMachine() = default;

    using TParentType::OnEvent;
    using TParentType::OnTransition;
    using TParentType::SetState;


    void OnEvent(TEventInfo& ctx, const NSM::TInitState&, const NSM::TStartEv&) {
        ctx.StartEvents += 1;
    }

    void OnEvent(TEventInfo& ctx, const NSM::TInitState&, const NSM::TShutdownEv&) {
        ctx.ShutdownEvents += 1;
    }

    void OnEvent(TEventInfo& ctx, const NSM::TRunningState&, const NSM::TStartEv&) {
        ctx.StartEvents += 1;
    }

    void OnEvent(TEventInfo& ctx, const NSM::TRunningState&, const NSM::TShutdownEv&) {
        ctx.ShutdownEvents += 1;
    }

    void OnEvent(TEventInfo& ctx, const NSM::TFinalState&, const NSM::TStartEv&) {
        ctx.StartEvents += 1;
    }

    void OnEvent(TEventInfo& ctx, const NSM::TFinalState&, const NSM::TShutdownEv&) {
        ctx.ShutdownEvents += 1;
    }
};


#define X_CPU_BENCHMARK(state, event) \
Y_CPU_BENCHMARK(state ## State_ ## event ## Event, iface) { \
    TEventInfo E; \
    TSampleMachine SM; \
    SM.SetState<NSM::T ## state ## State>(E); \
    for (size_t i = 0; i < iface.Iterations(); ++i) { \
        Y_DO_NOT_OPTIMIZE_AWAY(SM.ProcessEvent(E, NSM::T ## event ## Ev{})); \
    } \
    Y_DO_NOT_OPTIMIZE_AWAY(E.StartEvents); \
    Y_DO_NOT_OPTIMIZE_AWAY(E.ShutdownEvents); \
}


X_CPU_BENCHMARK(Init, Start);
X_CPU_BENCHMARK(Init, Shutdown);
X_CPU_BENCHMARK(Running, Start);
X_CPU_BENCHMARK(Running, Shutdown);
X_CPU_BENCHMARK(Final, Start);
X_CPU_BENCHMARK(Final, Shutdown);
