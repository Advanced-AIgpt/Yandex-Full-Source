#include <alice/megamind/library/context/parsed_frames.h>
#include <alice/library/frame/builder.h>
#include <alice/library/proto/proto.h>
#include <alice/library/unittest/message_diff.h>

#include <library/cpp/testing/unittest/registar.h>

using namespace NAlice;

namespace {

::NBg::NProto::TAliceParsedFramesResult CreateBegemotResponse() {
    return ParseProtoText<::NBg::NProto::TAliceParsedFramesResult>(R"(
Frames {
    Name: "personal_assistant.scenarios.music_play"
    Slots {
        Name: "action_request"
        Type: "string"
        Value: "включи"
        AcceptedTypes: ["string"]
        IsFilled: true
    }
    Slots {
        Name: "search_text"
        Type: "string"
        Value: "радио европа плюс"
        AcceptedTypes: ["string"]
        IsFilled: true
    }
}
Frames {
    Name: "personal_assistant.scenarios.radio_play"
    Slots {
        Name: "fm_radio"
        Type: "custom.fm_radio_station"
        Value: "Европа Плюс"
        AcceptedTypes: ["custom.fm_radio_station"]
        IsFilled: true
    }
}
Confidences: [0.00053518184, 1]
Sources: ["AliceTagger", "Granet"]
)");
}

const TVector<TString> RESPONSE_KEYS = {"Frames", "Confidences", "Sources"};

Y_UNIT_TEST_SUITE(TestParsedFrames) {
    Y_UNIT_TEST(TestParseCorrectInput) {
        const auto parsedFramesResponse = TParsedFramesResponse(CreateBegemotResponse());

        const TString musicFrameName = "personal_assistant.scenarios.music_play";
        const TString radioFrameName = "personal_assistant.scenarios.radio_play";

        const TVector<TSemanticFrame>& frames = parsedFramesResponse.GetFrames();

        UNIT_ASSERT_VALUES_EQUAL_C(frames.size(), 2, "Invalid frame count: expected - 2, got - " << frames.size());

        const TSemanticFrame musicExpectedFrame = TSemanticFrameBuilder{musicFrameName}
            .AddSlot(
                "action_request" /* name */,
                {"string"} /* acceptedTypes */,
                "string" /* type */,
                "включи" /* value */,
                false /* isRequested */,
                true /* isFilled */
            )
            .AddSlot(
                "search_text" /* name */,
                {"string"} /* acceptedTypes */,
                "string" /* type */,
                "радио европа плюс" /* value */,
                false /* isRequested */,
                true /* isFilled */
            )
            .Build();

        UNIT_ASSERT_MESSAGES_EQUAL(frames.at(0), musicExpectedFrame);

        const TSemanticFrame radioExpectedFrame = TSemanticFrameBuilder{radioFrameName}
            .AddSlot(
                "fm_radio" /* name */,
                {"custom.fm_radio_station"} /* acceptedTypes */,
                "custom.fm_radio_station" /* type */,
                "Европа Плюс" /* value */,
                false /* isRequested */,
                true /* isFilled */
            )
            .Build();

        UNIT_ASSERT_MESSAGES_EQUAL(frames.at(1), radioExpectedFrame);

        const auto& sources = parsedFramesResponse.GetSources();
        const TVector<TString> expectedSources = {"AliceTagger", "Granet"};
        UNIT_ASSERT_VALUES_EQUAL_C(sources, expectedSources, "Unexpected sources");

        UNIT_ASSERT_VALUES_EQUAL(static_cast<float>(0.00053518184), parsedFramesResponse.GetFrameConfidence(musicFrameName));
        UNIT_ASSERT_VALUES_EQUAL(static_cast<float>(1.0), parsedFramesResponse.GetFrameConfidence(radioFrameName));
    }

    Y_UNIT_TEST(TestParseInvalidInput) {
        {
            const auto parsedFramesResponse = TParsedFramesResponse(::NBg::NProto::TAliceParsedFramesResult());
            UNIT_ASSERT_C(parsedFramesResponse.GetFrames().empty(), "Expects empty result for an invalid response");
        }

        {
            auto begemotResponse = CreateBegemotResponse();
            begemotResponse.MutableSources()->erase(begemotResponse.GetSources().begin());
            UNIT_ASSERT_EXCEPTION(TParsedFramesResponse(begemotResponse), yexception);
        }

        {
            auto begemotResponse = CreateBegemotResponse();
            begemotResponse.MutableConfidences()->erase(begemotResponse.GetConfidences().begin());
            UNIT_ASSERT_EXCEPTION(TParsedFramesResponse(begemotResponse), yexception);
        }
    }
}

} // namespace
