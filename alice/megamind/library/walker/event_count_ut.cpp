#include <alice/megamind/library/walker/event_count.h>
#include <alice/megamind/library/testing/mock_session.h>
#include <library/cpp/testing/gmock_in_unittest/gmock.h>
#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/ptr.h>

namespace NAlice {
namespace {

const TMaybe<TString> NO_SESSION_SCENARIO_NAME = Nothing();

using namespace ::testing;

Y_UNIT_TEST_SUITE(UpdateEventCount) {

Y_UNIT_TEST(Update_givenInitialCounterAndRelevantScenario_returnsCountZero) {
    UNIT_ASSERT_VALUES_EQUAL(0, UpdateEventCount(NO_SESSION_SCENARIO_NAME, 0, "scenario_x", /* eventOccured= */ false));
}

Y_UNIT_TEST(Update_givenInitialCounterAndIrelevantScenario_returnsCountOne) {
    UNIT_ASSERT_VALUES_EQUAL(1, UpdateEventCount(NO_SESSION_SCENARIO_NAME, 0, "scenario_x", /* eventOccured= */ true));
}

Y_UNIT_TEST(Update_givenNonInitialCounterAndRelevantScenario_resetsCount) {
    const TString scenarioName = "scenario_x";
    UNIT_ASSERT_VALUES_EQUAL(0, UpdateEventCount(scenarioName, 42, scenarioName, /* eventOccured= */ false));
}

Y_UNIT_TEST(Update_givenNonInitialCounterAndIrrelevantScenario_incrementsCount) {
    const TString scenarioName = "scenario_x";
    const ui32 count = 42;
    UNIT_ASSERT_VALUES_EQUAL(count + 1, UpdateEventCount(scenarioName, count, scenarioName, /* eventOccured= */ true));
}

};

} // namespace
} // namespace NAlice
