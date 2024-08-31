#include "states.h"

#include <library/cpp/testing/unittest/registar.h>


Y_UNIT_TEST_SUITE(States) {

Y_UNIT_TEST(Equal) {
    NSM::TInitState One, Two;

    UNIT_ASSERT_VALUES_EQUAL(One.Id, Two.Id);
    UNIT_ASSERT_VALUES_EQUAL(TStringBuf(One.Name), TStringBuf(Two.Name));
}


Y_UNIT_TEST(NotEqual) {
    NSM::TInitState  One;
    NSM::TFinalState Two;

    UNIT_ASSERT_VALUES_UNEQUAL(One.Id, Two.Id);
    UNIT_ASSERT_VALUES_UNEQUAL(TStringBuf(One.Name), TStringBuf(Two.Name));
}


Y_UNIT_TEST(InitState) {
    NSM::TInitState  State;
    UNIT_ASSERT_VALUES_EQUAL(TStringBuf(State.Name), TStringBuf("ev.state.init"));
}


Y_UNIT_TEST(FinalState) {
    NSM::TFinalState  State;
    UNIT_ASSERT_VALUES_EQUAL(TStringBuf(State.Name), TStringBuf("ev.state.final"));
}

};
