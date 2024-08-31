#include "event.h"
#include "image_input_event.h"

#include <alice/library/experiments/flags.h>
#include <library/cpp/testing/unittest/registar.h>

namespace {

using namespace NAlice;

Y_UNIT_TEST_SUITE(Event) {
    Y_UNIT_TEST(ImageInputSmoke) {
        TEvent eventProto;
        eventProto.SetType(EEventType::image_input);

        auto event = IEvent::CreateEvent(eventProto);
        Y_ASSERT(event);
        NScenarios::TInput input;
        UNIT_ASSERT_EXCEPTION(event->FillScenarioInput(Nothing(), &input), IEvent::TInvalidEvent);
    }
}

} // namespace NAlice
