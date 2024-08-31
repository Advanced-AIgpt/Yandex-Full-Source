#include "event.h"
#include "voice_input_event.h"

#include <alice/library/experiments/flags.h>

#include <library/cpp/testing/unittest/registar.h>

namespace {

using namespace NAlice;

Y_UNIT_TEST_SUITE(VoiceEvent) {
    Y_UNIT_TEST(Normalized) {
        const TString NORMALIZED_UTTERANCE = "Normalized, utterance!";

        TEvent eventProto;
        eventProto.SetType(EEventType::voice_input);
        auto* bestAsrResult = eventProto.AddAsrResult();

        bestAsrResult->SetNormalized(NORMALIZED_UTTERANCE);

        auto event = IEvent::CreateEvent(eventProto);
        UNIT_ASSERT(event);
        UNIT_ASSERT(event->HasAsrNormalizedUtterance());
        UNIT_ASSERT_EQUAL(NORMALIZED_UTTERANCE, event->GetAsrNormalizedUtterance());
    }
}

} // namespace
