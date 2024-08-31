#include "intents.h"

#include <alice/bass/ut/helpers.h>

#include <alice/library/json/json.h>
#include <alice/library/video_common/defs.h>

#include <alice/megamind/protos/blackbox/blackbox.pb.h>
#include <alice/megamind/protos/common/data_source_type.pb.h>
#include <alice/megamind/protos/scenarios/external_markup.pb.h>

#include <library/cpp/protobuf/json/json2proto.h>

using namespace NProtobufJson;
using namespace NBASS;
using namespace NVideoProtocol;

namespace {

constexpr TStringBuf VIDEO_PLAY_REQUEST = TStringBuf(R"(
{
    "input": {
        "semantic_frames": [
            {
                "name": "personal_assistant.scenarios.video_play",
                "slots" : [
                    {
                        "name": "film_genre",
                        "type": "video_film_genre",
                        "value": "comedy"
                    }
                ]
            },
            {
                "name": "personal_assistant.scenarios.video_play_text",
                "slots" : [
                    {
                        "name": "film_genre",
                        "type": "string",
                        "value": "комедия"
                    }
                ]
            }
        ]
    }
}
)");

const auto VIDEO_PLAY_FORM = NSc::TValue::FromJson(R"(
{
    "form":
        {
            "name": "personal_assistant.scenarios.video_play",
            "slots":
                [
                    {
                        "name": "film_genre",
                        "optional": 1,
                        "source_text": "комедия",
                        "type": "video_film_genre",
                        "value": "comedy"
                    }
                ]
        }
}
)");

constexpr TStringBuf VIDEO_PLAY_NO_TEXT_FRAME_REQUEST = TStringBuf(R"(
{
    "input": {
        "semantic_frames": [
            {
                "name": "personal_assistant.scenarios.video_play",
                "slots" : [
                    {
                        "name": "film_genre",
                        "type": "video_film_genre",
                        "value": "comedy"
                    }
                ]
            }
        ]
    }
}
)");

constexpr TStringBuf VIDEO_PLAY_NO_SLOTS = TStringBuf(R"(
{
    "input": {
        "semantic_frames": [
            {
                "name": "personal_assistant.scenarios.video_play"
            }
        ]
    }
}
)");

const auto VIDEO_PLAY_NO_TEXT_FRAME_FORM = NSc::TValue::FromJson(R"(
{
    "form":
        {
            "name": "personal_assistant.scenarios.video_play",
            "slots":
                [
                    {
                        "name": "film_genre",
                        "optional": 1,
                        "type": "video_film_genre",
                        "value": "comedy"
                    }
                ]
        }
}
)");


Y_UNIT_TEST_SUITE(ProtocolScenarioIntents) {
    TVideoIntent<TSearchVideoIntent> videoIntent(NAlice::NVideoCommon::SEARCH_VIDEO);
    NAlice::NScenarios::TScenarioRunRequest protocolRequest;
    Y_UNIT_TEST(SourceTextSlot) {
        Json2Proto(VIDEO_PLAY_REQUEST, protocolRequest, NAlice::JSON2PROTO_CONFIG_WITH_JSON_NAMES);
        NSc::TValue resultRequest;
        const auto err = videoIntent.MakeRunRequest(protocolRequest, resultRequest);
        UNIT_ASSERT_C(!err.Defined(), *err);
        UNIT_ASSERT(NTestingHelpers::EqualJson(/* expected= */ VIDEO_PLAY_FORM, resultRequest));
    }

    Y_UNIT_TEST(NoSourceTextSlot) {
        Json2Proto(VIDEO_PLAY_NO_TEXT_FRAME_REQUEST, protocolRequest, NAlice::JSON2PROTO_CONFIG_WITH_JSON_NAMES);
        NSc::TValue resultRequest;
        const auto err = videoIntent.MakeRunRequest(protocolRequest, resultRequest);
        UNIT_ASSERT_C(!err.Defined(), *err);
        UNIT_ASSERT(NTestingHelpers::EqualJson(/* expected= */ VIDEO_PLAY_NO_TEXT_FRAME_FORM, resultRequest));
    }

    Y_UNIT_TEST(BlackBoxData) {
        TVideoIntent<TSearchVideoIntent> videoIntent(NAlice::NVideoCommon::SEARCH_VIDEO);
        NAlice::NScenarios::TScenarioRunRequest protocolRequest;

        Json2Proto(VIDEO_PLAY_NO_TEXT_FRAME_REQUEST, protocolRequest, NAlice::JSON2PROTO_CONFIG_WITH_JSON_NAMES);
        auto& bbData =
            *(*protocolRequest.MutableDataSources())[NAlice::EDataSourceType::BLACK_BOX].MutableUserInfo();
        *bbData.MutableUid() = "12345";
        bbData.SetHasYandexPlus(true);

        NSc::TValue resultRequest;
        const auto err = videoIntent.MakeRunRequest(protocolRequest, resultRequest);
        UNIT_ASSERT_C(!err.Defined(), *err);

        NSc::TValue expected = VIDEO_PLAY_NO_TEXT_FRAME_FORM;
        auto data = NSc::TValue::FromJson(R"({"uid": "12345", "hasYandexPlus": true})");
        expected["data_sources"]["2"]["user_info"] = data;
        UNIT_ASSERT(NTestingHelpers::EqualJson(expected, resultRequest));
    }

    Y_UNIT_TEST(Irrelevant) {
        TVideoIntent<TSearchVideoIntent> videoIntent(NAlice::NVideoCommon::SEARCH_VIDEO);

        {
            NAlice::NScenarios::TScenarioRunRequest protocolRequest;
            Json2Proto(VIDEO_PLAY_NO_SLOTS, protocolRequest, NAlice::JSON2PROTO_CONFIG_WITH_JSON_NAMES);
            NSc::TValue resultRequest;
            const auto err = videoIntent.MakeRunRequest(protocolRequest, resultRequest);
            UNIT_ASSERT_C(err, *err);
            UNIT_ASSERT(err->Type == TError::EType::PROTOCOL_IRRELEVANT);
        }

        {
            NAlice::NScenarios::TScenarioRunRequest protocolRequest;
            Json2Proto(VIDEO_PLAY_NO_SLOTS, protocolRequest, NAlice::JSON2PROTO_CONFIG_WITH_JSON_NAMES);
            (*protocolRequest.MutableDataSources())[NAlice::EDataSourceType::BEGEMOT_EXTERNAL_MARKUP]
                .MutableBegemotExternalMarkup()
                ->MutablePorn()
                ->SetIsPornoQuery(true);

            NSc::TValue resultRequest;
            const auto err = videoIntent.MakeRunRequest(protocolRequest, resultRequest);
            UNIT_ASSERT_C(err, *err);
            UNIT_ASSERT(err->Type == TError::EType::PROTOCOL_IRRELEVANT);
        }
    }
}

} // namespace
