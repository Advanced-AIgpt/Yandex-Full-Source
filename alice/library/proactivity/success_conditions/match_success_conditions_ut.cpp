#include "match_success_conditions.h"

#include <alice/library/json/json.h>
#include <library/cpp/testing/unittest/registar.h>

using namespace NAlice;

namespace {

constexpr auto SLOT_VALUE_JSON = TStringBuf(R"(
{
    "string": "string value 1",
    "string_upper": "STRING UPPER CASE",
    "int": 42,
    "double": 42.5,
    "array": [1, 2, 3],
    "map": {"key_1": 1, "key_2": 2}
}
)");

constexpr auto SLOT_WITH_STRING_VALUE = TStringBuf(R"(
{
    "value": "VALUE_string",
    "type": "string",
    "name": "slot_name"
}
)");
constexpr auto SLOT_WITH_STRING_VALUE_EXPECTED = TStringBuf(R"(
{
    "value": "value_STRING",
    "name": "slot_name"
}
)");
constexpr auto SLOT_WITH_STRING_VALUE_EXPECTED_NOT_MATCH = TStringBuf(R"(
{
    "value": "value_not_match",
    "name": "slot_name"
}
)");
constexpr auto SLOT_WITH_STRING_VALUE_EXPECTED_EMPTY = TStringBuf(R"(
{
    "name": "slot_name"
}
)");
constexpr auto SLOT_WITH_STRING_VALUE_EXPECTED_MATCH_TYPE =TStringBuf(R"(
{
    "value": "value_STRING",
    "type": "str.*",
    "name": "slot_name"
}
)");
constexpr auto SLOT_WITH_STRING_VALUE_EXPECTED_NOT_MATCH_TYPE = TStringBuf(R"(
{
    "value": "value_STRING",
    "type": "not_match_type",
    "name": "slot_name"
}
)");

constexpr auto SLOT_WITH_MAP_VALUE = TStringBuf(R"(
{
    "value": "{\"key_1\": \"value_1\", \"key_2\": 2, \"key_3\": 3}",
    "type": "string",
    "name": "slot_name"
}
)");
constexpr auto SLOT_WITH_MAP_VALUE_EXPECTED = TStringBuf(R"(
{
    "value": "{\"key_1\": \"val.*1\", \"key_2\": 2}",
    "name": "slot_name"
}
)");
constexpr auto SLOT_WITH_MAP_VALUE_EXPECTED_NOT_MATCH = TStringBuf(R"(
{
    "value": "{\"key_1\": \"val.*2\"}",
    "name": "slot_name"
}
)");

constexpr auto SLOT_WITH_TWO_VALUES = TStringBuf(R"(
{
    "typed_value": {
      "type": "type_1",
      "string": "value_1"
    },
    "type": "string",
    "value": "value",
    "name": "slot_name"
}
)");
constexpr auto SLOT_WITH_TWO_VALUES_EXPECTED = TStringBuf(R"(
{
    "typed_value": {
      "type": "type_2",
      "string": "value_2"
    },
    "type": "string",
    "value": "value",
    "name": "slot_name"
}
)");
constexpr auto SLOT_WITH_DEFAULT_VALUE = TStringBuf(R"(
{
    "type": "type",
    "value": "value",
    "name": "slot_name"
}
)");
constexpr auto SLOT_WITH_TYPED_VALUE = TStringBuf(R"(
{
    "typed_value": {
      "type": "type",
      "string": "value"
    },
    "name": "slot_name"
}
)");

constexpr auto SEMANTIC_FRAME = TStringBuf(R"(
{
    "name": "frame_name",
    "slots": [
    {
      "value": "value_1",
      "type": "string",
      "name": "slot_1"
    },
    {
      "value": "{\"key_1\": \"value_1\", \"key_2\": 2}",
      "type": "string",
      "name": "slot_2"
    }
  ]
}
)");
constexpr auto SEMANTIC_FRAME_FOR_WRONG_FRAME_NAME = TStringBuf(R"(
{
    "name": "frame_name not match"
}
)");
constexpr auto SEMANTIC_FRAME_FOR_ABSENT_SLOT = TStringBuf(R"(
{
    "name": "frame_name",
    "slots": [
        {
            "value": "value",
            "name": "slot"
        }
    ]
}
)");
constexpr TStringBuf POSTROLL_CONDITION_FRAME_EMPTY = "{}";
constexpr auto POSTROLL_CONDITION_FRAME = TStringBuf(R"(
{
    "name": "frame_name",
    "slots": [
        {
            "value": "value_1",
            "name": "slot_1"
        },
        {
            "type": "string",
            "value": "{\"key_1\": \"value_1\"}",
            "name": "slot_2"
        }
    ]
}
)");
constexpr auto POSTROLL_CONDITION_FRAME_WRONG_FRAME_NAME = TStringBuf(R"(
{
    "name": "frame_name not match"
}
)");
constexpr auto POSTROLL_CONDITION_FRAME_ABSENT_SLOT = TStringBuf(R"(
{
    "name": "frame_name",
    "slots": [
        {
            "value": "value",
            "name": "slot"
        }
    ]
}
)");
constexpr auto POSTROLL_CONDITION_FRAME_WRONG_STRING_SLOT = TStringBuf(R"(
{
    "name": "frame_name",
    "slots": [
        {
            "value": "value_2",
            "name": "slot_1"
        }
    ]
}
)");
constexpr auto POSTROLL_CONDITION_FRAME_WRONG_MAP_SLOT = TStringBuf(R"(
{
    "name": "frame_name",
    "slots": [
        {
            "value": "{\"key_1\": \"value\"}",
            "name": "slot_1"
        }
    ]
}
)");

constexpr auto DEVICE_STATE_TV_IS_NOT_PLUGGED_IN = TStringBuf(R"(
{
    "is_tv_plugged_in": false
}
)");
constexpr auto DEVICE_STATE_TV_IS_PLUGGED_IN = TStringBuf(R"(
{
    "is_tv_plugged_in": true
}
)");
constexpr TStringBuf DEVICE_STATE_EMPTY = "{}";


void TestSlotValueMatching(const TStringBuf actualValue, const TStringBuf expectedValue, bool match) {
    UNIT_ASSERT_C(
        match == SlotValueSatisfiesCondition(ToString(actualValue), ToString(expectedValue)),
        TStringBuilder{} << "Slot value \"" << actualValue << "\" must" << (match ? "" : " not") << " match slot value \"" << expectedValue << "\"");
}

void TestSlotMatching(const TSemanticFrame::TSlot& actualSlot, const TStringBuf expectedSlotString, bool match) {
    TSemanticFrame::TSlot expectedSlot;
    const auto status = JsonToProto(NJson::ReadJsonFastTree(expectedSlotString), expectedSlot);
    UNIT_ASSERT_C(status.ok(), status.ToString());

    UNIT_ASSERT_C(
        match == SlotSatisfiesCondition(actualSlot, expectedSlot),
        TStringBuilder{} << "Slot " << JsonStringFromProto(actualSlot) << " must" << (match ? "" : " not") << " match slot " << expectedSlotString);

}

void TestSemanticFrameMatching(const TSemanticFrame& frame, const TStringBuf expectedFrameString, bool match) {
    TSemanticFrame expectedFrame;
    const auto status = JsonToProto(NJson::ReadJsonFastTree(expectedFrameString), expectedFrame);
    UNIT_ASSERT_C(status.ok(), status.ToString());

    UNIT_ASSERT_C(
        match == FrameSatisfiesCondition(frame, expectedFrame),
        TStringBuilder{} << "Frame " << JsonStringFromProto(frame) << " must" << (match ? "" : " not") << " match frame " << expectedFrameString);

}

void TestAnySemanticFrameMatching(const TVector<TStringBuf>& actualFramesStrings, const TStringBuf expectedFrameString, bool match) {
    TSemanticFrame expectedFrame;
    const auto status = JsonToProto(NJson::ReadJsonFastTree(expectedFrameString), expectedFrame);
    UNIT_ASSERT_C(status.ok(), status.ToString());

    TVector<TSemanticFrame> actualFrames;
    TStringBuilder errorMessage;

    for (const auto& frameString : actualFramesStrings) {
        TSemanticFrame frame;
        const auto statusActual = JsonToProto(NJson::ReadJsonFastTree(frameString), frame);
        UNIT_ASSERT_C(statusActual.ok(), statusActual.ToString());
        actualFrames.push_back(frame);
    }

    UNIT_ASSERT_C(
        match == AnyFrameSatisfiesCondition(actualFrames, expectedFrame),
        TStringBuilder{} << "Frames must" << (match ? "" : " not") << " match frame " << expectedFrameString);

}

void TestDeviceStateMatching(const TStringBuf actualStateString, const TStringBuf expectedStateString, bool match) {
    TDeviceState actualState;
    const auto statusActual = JsonToProto(NJson::ReadJsonFastTree(actualStateString), actualState);
    UNIT_ASSERT_C(statusActual.ok(), statusActual.ToString());

    TDeviceState expectedState;
    const auto statusExpected = JsonToProto(NJson::ReadJsonFastTree(expectedStateString), expectedState);
    UNIT_ASSERT_C(statusExpected.ok(), statusExpected.ToString());

    UNIT_ASSERT_C(
        match == DeviceStateSatisfiesCondition(actualState, expectedState),
        TStringBuilder{} << "Device state " << JsonStringFromProto(actualState) << " must" << (match ? "" : " not") << " match device state " << expectedState);

}


Y_UNIT_TEST_SUITE(ProactivityActions) {
    Y_UNIT_TEST(CheckStringSlotValues) {
        TestSlotValueMatching("slot value text", "slot value text", /* match */ true);
        TestSlotValueMatching("slot value text", ".*value.*", /* match */ true);
        TestSlotValueMatching("slot value text", "value", /* match */ true);
        TestSlotValueMatching("slot value text", "^slot value text$", /* match */ true);
        TestSlotValueMatching("slot value text", "^value$", /* match */ false);
        TestSlotValueMatching("slot value text", "^value.*", /* match */ false);
        TestSlotValueMatching("SLOT VALUE TEXT", ".*value.*", /* match */ true);
        TestSlotValueMatching("slot value text", ".*VALUE.*", /* match */ true);

        TestSlotValueMatching("вести фм", "Вести ФМ", /* match */ true);
        TestSlotValueMatching("Вести ФМ", "[вг]ести ФМ", /* match */ true);
        TestSlotValueMatching("Вести", "[Вв]ести ФМ", /* match */ false);
        TestSlotValueMatching("вести фм1111", "[Вв]ести ФМ", /* match */ true);
        TestSlotValueMatching("вести фм", "^[Вв]ести ФМ$", /* match */ true);
        TestSlotValueMatching("вести фм1111", "^[Вв]ести ФМ$", /* match */ false);
        TestSlotValueMatching("1111вести фм", "^[Вв]ести ФМ$", /* match */ false);

        TestSlotValueMatching("[slot value text]", "slot value text", /* match */ true);
        TestSlotValueMatching("[slot value text]", "[sS]lot value text", /* match */ true);
        TestSlotValueMatching("[slot]", "[sS]lot value text", /* match */ false);

        TestSlotValueMatching("text 1", "(?:text 1|text 2)", /* match */ true);
        TestSlotValueMatching("text 21", "(?:text 1|text 2)", /* match */ true);
        TestSlotValueMatching("text 2", "(?:text 1|text 21)", /* match */ false);
        TestSlotValueMatching("абв", "(?:[аг]бв|эюя)", /* match */ true);
        TestSlotValueMatching("АБВ", "(?:[аг]бв|эюя)", /* match */ true);

        TestSlotValueMatching("invalid regex", "invalid regex\\", /* match */ false);
        TestSlotValueMatching("text", "*", /* match */ false);
        TestSlotValueMatching("text", "[a-z]++", /* match */ false);
    }

    Y_UNIT_TEST(CheckJsonSlotValues) {
        TestSlotValueMatching(SLOT_VALUE_JSON, "{\"string\": \"string value 1\"}", /* match */ true);
        TestSlotValueMatching(SLOT_VALUE_JSON, "{\"string\": \"STRING VALUE 1\"}", /* match */ true);
        TestSlotValueMatching(SLOT_VALUE_JSON, "{\"string\": \"string value 2\"}", /* match */ false);
        TestSlotValueMatching(SLOT_VALUE_JSON, "{\"string\": \"string value.\\w\"}", /* match */ false);

        TestSlotValueMatching(SLOT_VALUE_JSON, "{\"string_upper\": \"STRING UPPER CASE\"}", /* match */ true);
        TestSlotValueMatching(SLOT_VALUE_JSON, "{\"string_upper\": \"string upper case\"}", /* match */ true);
        TestSlotValueMatching(SLOT_VALUE_JSON, "{\"string_upper\": \"^STRING.UPPER.*\"}", /* match */ true);
        TestSlotValueMatching(SLOT_VALUE_JSON, "{\"string\": \"STRING UPPER\"}", /* match */ false);

        TestSlotValueMatching(SLOT_VALUE_JSON, "{\"string\": \"^string.value.*\"}", /* match */ true);
        TestSlotValueMatching(SLOT_VALUE_JSON, "{\"string\": \"^string value 1$\"}", /* match */ true);
        TestSlotValueMatching(SLOT_VALUE_JSON, "{\"string\": \"^string\"}", /* match */ true);
        TestSlotValueMatching(SLOT_VALUE_JSON, "{\"string\": \"value 1$\"}", /* match */ true);
        TestSlotValueMatching(SLOT_VALUE_JSON, "{\"string\": \"value\"}", /* match */ true);
        TestSlotValueMatching(SLOT_VALUE_JSON, "{\"string\": \"^value 1\"}", /* match */ false);

        TestSlotValueMatching(SLOT_VALUE_JSON, "{\"string\": \"[sqw]tring.value.*\"}", /* match */ true);
        TestSlotValueMatching(SLOT_VALUE_JSON, "{\"string\": \"[qw]tring.value.*\"}", /* match */ false);
        TestSlotValueMatching(SLOT_VALUE_JSON, "{\"string\": \"[^qw]tring.value.*\"}", /* match */ true);
        TestSlotValueMatching(SLOT_VALUE_JSON, "{\"string\": \"^string value [123]$\"}", /* match */ true);
        TestSlotValueMatching(SLOT_VALUE_JSON, "{\"string\": \"^string value [23]$\"}", /* match */ false);

        TestSlotValueMatching(SLOT_VALUE_JSON, "{\"int\": 42}", /* match */ true);
        TestSlotValueMatching(SLOT_VALUE_JSON, "{\"int\": 43}", /* match */ false);

        TestSlotValueMatching(SLOT_VALUE_JSON, "{\"double\": 42.5}", /* match */ true);
        TestSlotValueMatching(SLOT_VALUE_JSON, "{\"double\": 43.5}", /* match */ false);

        TestSlotValueMatching(SLOT_VALUE_JSON, "{\"array\": [1, 2, 3]}", /* match */ true);
        TestSlotValueMatching(SLOT_VALUE_JSON, "{\"array\": [1, 2, 3, 4]}", /* match */ false);

        TestSlotValueMatching(SLOT_VALUE_JSON, "{\"map\": {\"key_1\": 1, \"key_2\": 2}}", /* match */ true);
        TestSlotValueMatching(SLOT_VALUE_JSON, "{\"map\": {\"key_1\": 1}}", /* match */ false);

        TestSlotValueMatching(SLOT_VALUE_JSON, "", /* match */ true);
        TestSlotValueMatching("", "", /* match */ true);
    }

    Y_UNIT_TEST(CheckSlotsWithStringValue) {
        TSemanticFrame::TSlot actualSlot;
        const auto statusActual = JsonToProto(NJson::ReadJsonFastTree(SLOT_WITH_STRING_VALUE), actualSlot);
        UNIT_ASSERT_C(statusActual.ok(), statusActual.ToString());

        TestSlotMatching(actualSlot, SLOT_WITH_STRING_VALUE_EXPECTED, /* match */ true);
        TestSlotMatching(actualSlot, SLOT_WITH_STRING_VALUE_EXPECTED_NOT_MATCH, /* match */ false);
        TestSlotMatching(actualSlot, SLOT_WITH_STRING_VALUE_EXPECTED_EMPTY, /* match */ true);

        TestSlotMatching(actualSlot, SLOT_WITH_STRING_VALUE_EXPECTED_MATCH_TYPE, /* match */ true);
        TestSlotMatching(actualSlot, SLOT_WITH_STRING_VALUE_EXPECTED_NOT_MATCH_TYPE, /* match */ false);
    }

    Y_UNIT_TEST(CheckSlotsWithMapValue) {
        TSemanticFrame::TSlot actualSlot;
        const auto statusActual = JsonToProto(NJson::ReadJsonFastTree(SLOT_WITH_MAP_VALUE), actualSlot);
        UNIT_ASSERT_C(statusActual.ok(), statusActual.ToString());

        TestSlotMatching(actualSlot, SLOT_WITH_MAP_VALUE_EXPECTED, /* match */ true);
        TestSlotMatching(actualSlot, SLOT_WITH_MAP_VALUE_EXPECTED_NOT_MATCH, /* match */ false);
    }


    Y_UNIT_TEST(CheckSlotsWithTwoValueTypes) {
        {
            TSemanticFrame::TSlot actualSlot;
            const auto statusActual = JsonToProto(NJson::ReadJsonFastTree(SLOT_WITH_TWO_VALUES), actualSlot);
            UNIT_ASSERT_C(statusActual.ok(), statusActual.ToString());
            TestSlotMatching(actualSlot, SLOT_WITH_TWO_VALUES_EXPECTED, /* match */ true);
        }
        {
            TSemanticFrame::TSlot actualSlot;
            const auto statusActual = JsonToProto(NJson::ReadJsonFastTree(SLOT_WITH_DEFAULT_VALUE), actualSlot);
            UNIT_ASSERT_C(statusActual.ok(), statusActual.ToString());
            TestSlotMatching(actualSlot, SLOT_WITH_TYPED_VALUE, /* match */ true);
        }
        {
            TSemanticFrame::TSlot actualSlot;
            const auto statusActual = JsonToProto(NJson::ReadJsonFastTree(SLOT_WITH_TYPED_VALUE), actualSlot);
            UNIT_ASSERT_C(statusActual.ok(), statusActual.ToString());
            TestSlotMatching(actualSlot, SLOT_WITH_DEFAULT_VALUE, /* match */ true);
        }
    }

    Y_UNIT_TEST(CheckSemanticFrame) {
        TSemanticFrame semanticFrame;
        const auto status = JsonToProto(NJson::ReadJsonFastTree(SEMANTIC_FRAME), semanticFrame);
        UNIT_ASSERT_C(status.ok(), status.ToString());

        TestSemanticFrameMatching(semanticFrame, POSTROLL_CONDITION_FRAME_EMPTY, /* match */ true);
        TestSemanticFrameMatching(semanticFrame, POSTROLL_CONDITION_FRAME, /* match */ true);
        TestSemanticFrameMatching(semanticFrame, POSTROLL_CONDITION_FRAME_WRONG_FRAME_NAME, /* match */ false);
        TestSemanticFrameMatching(semanticFrame, POSTROLL_CONDITION_FRAME_ABSENT_SLOT, /* match */ false);
        TestSemanticFrameMatching(semanticFrame, POSTROLL_CONDITION_FRAME_WRONG_STRING_SLOT, /* match */ false);
        TestSemanticFrameMatching(semanticFrame, POSTROLL_CONDITION_FRAME_WRONG_MAP_SLOT, /* match */ false);
    }

    Y_UNIT_TEST(CheckSeveralSemanticFrame) {
        TestAnySemanticFrameMatching({SEMANTIC_FRAME}, POSTROLL_CONDITION_FRAME_EMPTY, /* match */ true);
        TestAnySemanticFrameMatching({SEMANTIC_FRAME}, POSTROLL_CONDITION_FRAME, /* match */ true);

        TestAnySemanticFrameMatching({SEMANTIC_FRAME}, POSTROLL_CONDITION_FRAME_WRONG_FRAME_NAME, /* match */ false);
        TestAnySemanticFrameMatching({SEMANTIC_FRAME, SEMANTIC_FRAME_FOR_WRONG_FRAME_NAME},
                                     POSTROLL_CONDITION_FRAME_WRONG_FRAME_NAME, /* match */ true);

        TestAnySemanticFrameMatching({SEMANTIC_FRAME}, POSTROLL_CONDITION_FRAME_ABSENT_SLOT, /* match */ false);
        TestAnySemanticFrameMatching({SEMANTIC_FRAME, SEMANTIC_FRAME_FOR_ABSENT_SLOT},
                                     POSTROLL_CONDITION_FRAME_ABSENT_SLOT, /* match */ true);
    }

    Y_UNIT_TEST(CheckDeviceState) {
        TestDeviceStateMatching(DEVICE_STATE_TV_IS_PLUGGED_IN, DEVICE_STATE_EMPTY, /* match */ true);
        TestDeviceStateMatching(DEVICE_STATE_TV_IS_NOT_PLUGGED_IN, DEVICE_STATE_EMPTY, /* match */ true);

        TestDeviceStateMatching(DEVICE_STATE_TV_IS_PLUGGED_IN, DEVICE_STATE_TV_IS_PLUGGED_IN, /* match */ true);
        TestDeviceStateMatching(DEVICE_STATE_TV_IS_NOT_PLUGGED_IN, DEVICE_STATE_TV_IS_NOT_PLUGGED_IN, /* match */ true);

        TestDeviceStateMatching(DEVICE_STATE_TV_IS_PLUGGED_IN, DEVICE_STATE_TV_IS_NOT_PLUGGED_IN, /* match */ false);
        TestDeviceStateMatching(DEVICE_STATE_TV_IS_NOT_PLUGGED_IN, DEVICE_STATE_TV_IS_PLUGGED_IN, /* match */ false);
    }
}

} // namespace
