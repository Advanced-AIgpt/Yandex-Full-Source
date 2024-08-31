#pragma once


#include <util/generic/typelist.h>
#include <util/stream/output.h>
#include "states.h"


namespace NSM {


namespace NSMPrivate {


template <class T, class TContext, class TState, class TEvent, class TRestEvents>
class TEventIterator
    : public TEventIterator<T, TContext, TState, typename TRestEvents::THead, typename TRestEvents::TTail>
{
    using TParent = TEventIterator<T, TContext, TState, typename TRestEvents::THead, typename TRestEvents::TTail>;

public:
    void DebugEvents(IOutputStream& out) {
        out << " " << TEvent::Name;
        TParent::DebugEvents(out);
    }

    template <class E>
    uint64_t ProcessEventImpl(T& self, TContext& context, const E& event) {
        if (E::Id == TEvent::Id) {
            TState State;
            self.OnEventPreHook(context, State, event);
            self.OnEvent(context, State, event);
            self.OnEventPostHook(context, State, event);
            return 1;
        } else {
            return TParent::template ProcessEventImpl<E>(self, context, event);
        }
    }
};


template <class T, class TContext, class TState, class TEvent>
class TEventIterator<T, TContext, TState, TEvent, TTypeList<>>
{
public:
    template <class E>
    uint64_t ProcessEventImpl(T& self, TContext& context, const E& event) {
        if (E::Id == TEvent::Id) {
            TState State;
            self.OnEventPreHook(context, State, event);
            self.OnEvent(context, State, event);
            self.OnEventPostHook(context, State, event);
            return 1;
        }
        return 0;
    }

    void DebugEvents(IOutputStream& out) {
        out << " " << TEvent::Name;
    }
};


template <class T, class TContext, class TState, class TRestStates, class TEvents>
class TStateIterator
    : public TEventIterator<T, TContext, TState, typename TEvents::THead, typename TEvents::TTail>
    , public TStateIterator<T, TContext, typename TRestStates::THead, typename TRestStates::TTail, TEvents>
{
    using TEventParent = TEventIterator<T, TContext, TState, typename TEvents::THead, typename TEvents::TTail>;
    using TStateParent = TStateIterator<T, TContext, typename TRestStates::THead, typename TRestStates::TTail, TEvents>;

public:
    template <class U>
    void SetStateImpl(T& self, TContext& context, uint64_t state) {
        if (state == TState::Id) {
            self.OnTransitionPreHook(context, TState(), U());
            self.OnTransition(context, TState(), U());
            self.SetStateImpl(U::Id);
            self.OnTransitionPostHook(context, TState(), U());
        } else {
            TStateParent::template SetStateImpl<U>(self, context, state);
        }
    }

    template <class E>
    uint64_t ProcessEventImpl(T& self, TContext& context, uint64_t state, const E& event) {
        if (state == TState::Id) {
            return TEventParent::template ProcessEventImpl<E>(self, context, event);
        } else {
            return TStateParent::template ProcessEventImpl<E>(self, context, state, event);
        }
    }

    void DebugStatesImpl() {
        TEventParent::DebugEvents(Cerr);
        Cerr << Endl;
        TStateParent::DebugStatesImpl();
    }

    void DebugStateName(uint64_t state) {
        if (state == TState::Id) {
            Cerr << TState::Name;
        } else {
            return TStateParent::DebugStateName(state);
        }
    }
};

template <class T, class TContext, class TState, class TEvents>
class TStateIterator<T, TContext, TState, TTypeList<>, TEvents>
    : public TEventIterator<T, TContext, TState, typename TEvents::THead, typename TEvents::TTail>
{
    using TEventParent = TEventIterator<T, TContext, TState, typename TEvents::THead, typename TEvents::TTail>;

public:
    template <class U>
    void SetStateImpl(T& self, TContext& context, uint64_t state) {
        if (state == TState::Id) {
            self.OnTransitionPreHook(context, TState(), U());
            self.OnTransition(context, TState(), U());
            self.SetStateImpl(U::Id);
            self.OnTransitionPostHook(context, TState(), U());
        }
    }

    template <class E>
    uint64_t ProcessEventImpl(T& self, TContext& context, uint64_t state, const E& event) {
        if (state == TState::Id) {
            return TEventParent::template ProcessEventImpl<E>(self, context, event);
        }
        return 0;
    }

    void DebugStatesImpl() {
        Cerr << "TStateIterator(" << TState::Name << ") => ";
        TEventParent::DebugEvents(Cerr);
        Cerr << Endl;
    }

    void DebugStateName(uint64_t state) {
        if (state == TState::Id) {
            Cerr << TState::Name;
        }
    }

};


template <class T, class TContext, class TState>
class TStateIterator<T, TContext, TState, TTypeList<>, TTypeList<>>
{
public:
    void DebugStatesImpl() {
        Cerr << "TStateIterator(" << TState::Name << ") => <>" << Endl;
    }

    void DebugStateName(uint64_t state) {
        if (state == TState::Id) {
            Cerr << TState::Name;
        }
    }
};



template <
    class T,
    class TContext,
    class TStates,
    class TEvents
>
class TStateMachine
    : public TStateIterator<T, TContext, typename TStates::THead, typename TStates::TTail, TEvents>
{
    using TParent = TStateIterator<T, TContext, typename TStates::THead, typename TStates::TTail, TEvents>;

public:
    template <class U>
    void SetStateImpl(T& self, TContext& context, uint64_t state) {
        TParent::template SetStateImpl<U>(self, context, state);
    }

    template <class E>
    uint64_t ProcessEventImpl(T& self, TContext& context, uint64_t state, const E& event) {
        return TParent::template ProcessEventImpl<E>(self, context, state, event);
    }

    void DebugStatesImpl() {
        TParent::DebugStatesImpl();
    }

    void DebugStateName(uint64_t id) {
        TParent::DebugStateName(id);
    }
};


}   // namespace NSMPrivate


template <
    class T,
    class TContext,
    class TStates,
    class TEvents
>
class TStateMachine
    : public NSMPrivate::TStateMachine<T, TContext, TStates, TEvents>
{
    using TParent = NSMPrivate::TStateMachine<T, TContext, TStates, TEvents>;

    T& GetMachine() {
        return *static_cast<T*>(this);
    }

public:
    using TParentType = TStateMachine<T, TContext, TStates, TEvents>;

    /**
     *  @brief called right berore the event processing
     */
    template <class TCurrentState, class TCurrentEvent>
    void OnEventPreHook(TContext&, const TCurrentState&, const TCurrentEvent&) { }

    /**
     *  @brief called right after the event is processed
     */
    template <class TCurrentState, class TCurrentEvent>
    void OnEventPostHook(TContext&, const TCurrentState&, const TCurrentEvent&) { }

    /**
     *  @brief called right before the state change
     */
    template <class TCurrentState, class TNextState>
    void OnTransitionPreHook(TContext&, const TCurrentState&, const TNextState&) { }

    /**
     *  @brief called right after the state change
     */
    template <class TCurrentState, class TNextState>
    void OnTransitionPostHook(TContext&, const TCurrentState&, const TNextState&) { }


    /**
     *  @brief called to process an event
     */
    template <class TCurrentState, class TCurrentEvent>
    void OnEvent(TContext&, const TCurrentState&, const TCurrentEvent&) { }


    /**
     *  @brief called when state is changing
     */
    template <class TSourceState, class TDestState>
    void OnTransition(TContext&, const TSourceState&, const TDestState&) { }


    template <class U>
    void SetState(TContext& context) {
        TParent::template SetStateImpl<U>(GetMachine(), context, State_);
    }

    template <class E>
    uint64_t ProcessEvent(TContext& context, const E& event) {
        return TParent::template ProcessEventImpl<E>(GetMachine(), context, State_, event);
    }


    void DebugStates() {
        Cerr << "State_=";
        NSMPrivate::TStateMachine<T, TContext, TStates, TEvents>::DebugStateName(State_);
        NSMPrivate::TStateMachine<T, TContext, TStates, TEvents>::DebugStatesImpl();
    }

    void SetStateImpl(uint64_t state) {
        State_ = state;
    }

    uint64_t State_ { TInitState::Id };
};

}   // namespace NSM
