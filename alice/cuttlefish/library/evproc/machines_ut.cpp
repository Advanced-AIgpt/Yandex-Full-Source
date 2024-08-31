#include <library/cpp/testing/unittest/registar.h>

#include "machines.h"
#include "events.h"
#include "states.h"


#include <util/generic/list.h>


struct TSampleMachineContext {
    uint64_t TransitionFrom { 0 };
    uint64_t TransitionTo { 0 };
    uint64_t LastEvent { 0 };
    uint64_t LastState { 0 };
    uint64_t CurrentState { 0 };
};


class TSampleMachine
    : public NSM::TStateMachine<
        TSampleMachine,             // T

        TSampleMachineContext,      // TContext

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
    // using TParentType::OnEventPreHook;
    // using TParentType::OnEventPostHook;

    template <class TCurrentState, class TCurrentEvent>
    void OnEventPreHook(TSampleMachineContext&, const TCurrentState&, const TCurrentEvent&) {
        Cerr << "TSampleMachine::OnEvent State=" << TCurrentState::Name << " Event=" << TCurrentEvent::Name << Endl;
    }

    template <class TCurrentState, class TNextState>
    void OnTransitionPreHook(TSampleMachineContext&, const TCurrentState&, const TNextState&) {
        Cerr << "TSampleMachine::OnTransition Current=" << TCurrentState::Name << " Next=" << TNextState::Name << Endl;
    }


    void OnEvent(TSampleMachineContext& ctx, const NSM::TInitState&, const NSM::TStartEv&) {
        ctx.LastState = State_;
        ctx.LastEvent = NSM::TStartEv::Id;
        SetState<NSM::TRunningState>(ctx);
        ctx.CurrentState = State_;
    }

    void OnEvent(TSampleMachineContext& ctx, const NSM::TRunningState&, const NSM::TShutdownEv&) {
        ctx.LastState = State_;
        ctx.LastEvent = NSM::TShutdownEv::Id;
        SetState<NSM::TFinalState>(ctx);
        ctx.CurrentState = State_;
    }

    void OnTransition(TSampleMachineContext& ctx, const NSM::TInitState&, const NSM::TRunningState&) {
        ctx.TransitionFrom = NSM::TInitState::Id;
        ctx.TransitionTo = NSM::TRunningState::Id;
    }

    void OnTransition(TSampleMachineContext& ctx, const NSM::TRunningState&, const NSM::TFinalState&) {
        ctx.TransitionFrom = NSM::TRunningState::Id;
        ctx.TransitionTo = NSM::TFinalState::Id;
    }
};

Y_UNIT_TEST_SUITE(StateMachine) {
    Y_UNIT_TEST(Instantiation) {
        THolder<TSampleMachine> Ptr = MakeHolder<TSampleMachine>();
        Ptr.Reset();
    }

    Y_UNIT_TEST(EventsAndTransitions) {
        TSampleMachineContext Context;

        THolder<TSampleMachine> Ptr = MakeHolder<TSampleMachine>();

        Ptr->ProcessEvent(Context, NSM::TStartEv{});

        UNIT_ASSERT_VALUES_EQUAL(Context.LastState, NSM::TInitState::Id);
        UNIT_ASSERT_VALUES_EQUAL(Context.LastEvent, NSM::TStartEv::Id);
        UNIT_ASSERT_VALUES_EQUAL(Context.TransitionFrom, NSM::TInitState::Id);
        UNIT_ASSERT_VALUES_EQUAL(Context.TransitionTo, NSM::TRunningState::Id);
        UNIT_ASSERT_VALUES_UNEQUAL(Context.LastState, Context.CurrentState);


        Context = { 0 };
        Ptr->ProcessEvent(Context, NSM::TStartEv{});
        UNIT_ASSERT_VALUES_EQUAL(Context.LastState, 0);
        UNIT_ASSERT_VALUES_EQUAL(Context.LastEvent, 0);
        UNIT_ASSERT_VALUES_EQUAL(Context.TransitionFrom, 0);
        UNIT_ASSERT_VALUES_EQUAL(Context.TransitionTo, 0);


        Context = { 0 };
        Ptr->ProcessEvent(Context, NSM::TShutdownEv{});
        UNIT_ASSERT_VALUES_EQUAL(Context.LastState, NSM::TRunningState::Id);
        UNIT_ASSERT_VALUES_EQUAL(Context.LastEvent, NSM::TShutdownEv::Id);
        UNIT_ASSERT_VALUES_EQUAL(Context.TransitionFrom, NSM::TRunningState::Id);
        UNIT_ASSERT_VALUES_EQUAL(Context.TransitionTo, NSM::TFinalState::Id);


        Context = { 0 };
        Ptr->ProcessEvent(Context, NSM::TStartEv{});
        UNIT_ASSERT_VALUES_EQUAL(Context.LastState, 0);
        UNIT_ASSERT_VALUES_EQUAL(Context.LastEvent, 0);
        UNIT_ASSERT_VALUES_EQUAL(Context.TransitionFrom, 0);
        UNIT_ASSERT_VALUES_EQUAL(Context.TransitionTo, 0);


        Context = { 0 };
        Ptr->ProcessEvent(Context, NSM::TShutdownEv{});
        UNIT_ASSERT_VALUES_EQUAL(Context.LastState, 0);
        UNIT_ASSERT_VALUES_EQUAL(Context.LastEvent, 0);
        UNIT_ASSERT_VALUES_EQUAL(Context.TransitionFrom, 0);
        UNIT_ASSERT_VALUES_EQUAL(Context.TransitionTo, 0);
    }
}
