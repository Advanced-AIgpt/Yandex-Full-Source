#include <library/cpp/testing/unittest/registar.h>

#include "events.h"


struct TSampleEvent : NSM::TEvent<TSampleEvent> {
    NSM_EVENT("ev.event.sample");

    uint64_t X { 0 };
    uint64_t Y { 1 };
};


struct TFooEvent : NSM::TEvent<TFooEvent> {
    NSM_EVENT("ev.event.foo");
};


Y_UNIT_TEST_SUITE(Events) {

    Y_UNIT_TEST(Is) {
        TSampleEvent Ev;
        NSM::TEventBase& Base = Ev;

        UNIT_ASSERT(Base.Is(TSampleEvent::Id));
        UNIT_ASSERT(!Base.Is(TFooEvent::Id));

        UNIT_ASSERT(Base.Is<TSampleEvent>());
        UNIT_ASSERT(!Base.Is<TFooEvent>());

        UNIT_ASSERT(Base.Is("ev.event.sample"));
        UNIT_ASSERT(!Base.Is("ev.event.foo"));
    }


    Y_UNIT_TEST(As) {
        TSampleEvent Ev;
        NSM::TEventBase& Base = Ev;

        UNIT_ASSERT_VALUES_EQUAL(&Ev, Base.As<TSampleEvent>());
        UNIT_ASSERT_VALUES_EQUAL(nullptr, Base.As<TFooEvent>());
    }
};
