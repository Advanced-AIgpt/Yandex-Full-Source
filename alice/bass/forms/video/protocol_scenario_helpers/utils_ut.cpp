#include "utils.h"

#include <alice/library/frame/description.h>
#include <library/cpp/testing/unittest/registar.h>
#include <alice/library/unittest/ut_helpers.h>

using namespace NAlice;
using namespace NAlice::NVideoCommon;
using namespace NVideoProtocol;

namespace {

NAlice::TSemanticFrame::TSlot MakeSlot(TStringBuf name, TStringBuf type, const TString& value) {
    NAlice::TSemanticFrame::TSlot slot;
    slot.SetName(TString{name});
    slot.SetType(TString{type});
    slot.SetValue(value);
    return slot;
}

Y_UNIT_TEST_SUITE(VideoUtils) {
    Y_UNIT_TEST(ConstructVideoSlot) {
        { // Screen slot name mapping.
            NSc::TValue result;
            ConstructVideoSlot(MakeSlot(SLOT_SCREEN_NAME, SLOT_SCREEN_TYPE, ToString(EScreenName::NewScreen)), result);
            NSc::TValue expected = NSc::TValue::FromJson(
                R"({"name":"screen", "optional": 1, "type": "quasar_video_screen", "value": "new_screen"})");
            UNIT_ASSERT(NTestingHelpers::EqualJson(expected, result));
        }
        { // Screen slot name mapping.
            NSc::TValue result;
            ConstructVideoSlot(MakeSlot(SLOT_SCREEN, SLOT_CUSTOM_SCREEN_TYPE, ToString(EScreenName::NewScreen)), result);
            NSc::TValue expected = NSc::TValue::FromJson(
                R"({"name":"screen", "optional": 1, "type": "quasar_video_screen", "value": "new_screen"})");
            UNIT_ASSERT(NTestingHelpers::EqualJson(expected, result));
        }
        { // Numeric season should be passed as number.
            NSc::TValue result;
            ConstructVideoSlot(MakeSlot(SLOT_SEASON, SLOT_NUM_TYPE, "2"), result);
            NSc::TValue expected = NSc::TValue::FromJson(
                R"({"name":"season", "optional": 1, "type": "num", "value": 2})");
            UNIT_ASSERT(NTestingHelpers::EqualJson(expected, result));
        }
        { // Non-numeric season should be passed as-is.
            NSc::TValue result;
            ConstructVideoSlot(MakeSlot(SLOT_SEASON, SLOT_SEASON_TYPE, "last"), result);
            NSc::TValue expected = NSc::TValue::FromJson(
                R"({"name":"season", "optional": 1, "type": "video_season", "value": "last"})");
            UNIT_ASSERT(NTestingHelpers::EqualJson(expected, result));
        }
        { // Numeric episode should be passed as number.
            NSc::TValue result;
            ConstructVideoSlot(MakeSlot(SLOT_EPISODE, SLOT_NUM_TYPE, "2"), result);
            NSc::TValue expected = NSc::TValue::FromJson(
                R"({"name":"episode", "optional": 1, "type": "num", "value": 2})");
            UNIT_ASSERT(NTestingHelpers::EqualJson(expected, result));
        }
        { // Non-numeric episode should be passed as-is.
            NSc::TValue result;
            ConstructVideoSlot(MakeSlot(SLOT_EPISODE, SLOT_EPISODE_TYPE, "last"), result);
            NSc::TValue expected = NSc::TValue::FromJson(
                R"({"name":"episode", "optional": 1, "type": "video_episode", "value": "last"})");
            UNIT_ASSERT(NTestingHelpers::EqualJson(expected, result));
        }
    }
}

} // namespace
