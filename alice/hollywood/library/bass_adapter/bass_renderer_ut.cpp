#include "bass_renderer.h"

#include <alice/hollywood/library/nlg/nlg_wrapper.h>

#include <alice/hollywood/library/scenarios/music/nlg/register.h>

#include <alice/library/json/json.h>
#include <alice/library/unittest/message_diff.h>

#include <apphost/lib/service_testing/service_testing.h>

#include <library/cpp/testing/unittest/env.h>
#include <library/cpp/testing/unittest/registar.h>

#include <memory>

using namespace NAlice::NScenarios;

namespace NAlice::NHollywood {

void CheckBassForm(const TStringBuf expected, const TStringBuf bassForm) {
    const auto expectedProto = ParseProtoText<TSemanticFrame>(expected);
    const auto actualForm = NImpl::ParseBassForm(JsonFromString(bassForm));
    UNIT_ASSERT(actualForm.Defined());
    const auto actualProto = actualForm->ToProto();

    UNIT_ASSERT_MESSAGES_EQUAL_C(expectedProto, actualProto, "BASS form:\n" << bassForm);
}

namespace {

// TODO(daypatu): remove dependency on music scenario
void Register(NAlice::NNlg::TEnvironment& env) {
    NAlice::NHollywood::NLibrary::NScenarios::NMusic::NNlg::RegisterAll(env);
}

struct TFixture : public NUnitTest::TBaseFixture {
    TFixture()
        : Nlg_{Rng_, nullptr, &Register}
    {
    }

    std::unique_ptr<TScenarioRunResponse> Test(const TStringBuf runRequestStr,
                                               const TStringBuf bassResponseStr,
                                               bool addSuggestAutoAction = true) {
        const auto runRequestProto = ParseProtoText<TScenarioRunRequest>(runRequestStr);
        NAppHost::NService::TTestContext serviceCtx;
        const TScenarioRunRequestWrapper runRequest(runRequestProto, serviceCtx);

        NJson::TJsonValue bassResponseJson = JsonFromString(bassResponseStr);

        TNlgWrapper nlgWrapper = TNlgWrapper::Create(Nlg_, runRequest, Rng_, ELanguage::LANG_RUS);
        TRunResponseBuilder builder(&nlgWrapper);
        TBassResponseRenderer renderer(runRequest, runRequest.Input(), builder, TRTLogger::NullLogger(), addSuggestAutoAction);
        renderer.Render("music_play", "render_result", bassResponseJson);
        auto runResponse = std::move(builder).BuildResponse();

        UNIT_ASSERT(!runResponse->GetVersion().Empty());
        runResponse->SetVersion("trunk@TEST"); // For the sake of reproducible tests we overwrite the svn commit id in the Version

        return runResponse;
    }

    NAlice::TFakeRng Rng_;
    TCompiledNlgComponent Nlg_;
};

} // namespace

Y_UNIT_TEST_SUITE_F(BassRenderer, TFixture) {
    Y_UNIT_TEST(Smoke) {
        const auto runRequestStr = TStringBuf(R"()");
        const TString bassResponseStr = R"(
        {
            "blocks": [],
            "form": {},
        })";

        const auto runResponseExpectedStr = TStringBuf(R"(
        ResponseBody {
            Layout {
                Cards {
                    Text: "Открываю."
                }
                OutputSpeech: "Открываю"
            }
            AnalyticsInfo {
                NlgRenderHistoryRecords {
                    TemplateName: "music_play"
                    PhraseName: "render_result"
                    Language: L_RUS
                }
            }
        }
        Version: "trunk@TEST")");
        ParseProtoText<NScenarios::TScenarioRunResponse>(runResponseExpectedStr);
        const auto& runResponseExpected = ParseProtoText<NScenarios::TScenarioRunResponse>(runResponseExpectedStr);

        const auto& runResponse = Test(runRequestStr, bassResponseStr);
        const auto actualRunResponseStr = TStringBuilder{} << "RunResponse (actual):\n" << SerializeProtoText(*runResponse, /* singleLineMode = */ false);
        UNIT_ASSERT_MESSAGES_EQUAL_C(runResponseExpected, *runResponse, actualRunResponseStr);
    }

    Y_UNIT_TEST(BlockScenarioAnalyticsInfo) {
        const auto runRequestStr = TStringBuf(R"(
        BaseRequest {
            Experiments {
                fields {
                    key: "analytics_info"
                    value {
                        string_value: "1"
                    }
                }
                fields {
                    key: "tunneller_analytics_info"
                    value {
                        string_value: "1"
                    }
                }
            }
        })");
        const TString bassResponseStr = R"(
        {
            "blocks": [{
                "type": "scenario_analytics_info",
                "data": "Qgdmb28gYmFy"
            }],
            "form": {},
        })";

        const auto runResponseExpectedStr = TStringBuf(R"(
        ResponseBody {
            Layout {
                Cards {
                    Text: "Открываю."
                }
                OutputSpeech: "Открываю"
            }
            AnalyticsInfo {
                TunnellerRawResponses: "foo bar"
                NlgRenderHistoryRecords {
                    TemplateName: "music_play"
                    PhraseName: "render_result"
                    Language: L_RUS
                }
            }
        }
        Version: "trunk@TEST")");
        ParseProtoText<NScenarios::TScenarioRunResponse>(runResponseExpectedStr);
        const auto& runResponseExpected = ParseProtoText<NScenarios::TScenarioRunResponse>(runResponseExpectedStr);

        const auto& runResponse = Test(runRequestStr, bassResponseStr);
        const auto actualRunResponseStr = TStringBuilder{} << "RunResponse (actual):\n" << SerializeProtoText(*runResponse, /* singleLineMode = */ false);
        UNIT_ASSERT_MESSAGES_EQUAL_C(runResponseExpected, *runResponse, actualRunResponseStr);
    }

    Y_UNIT_TEST(SemanticFrame) {
        const auto runRequestStr = TStringBuf(R"(
        Input {
            SemanticFrames {
                Name: "TestSemanticFrame"
            }
        })");
        const TString bassResponseStr = R"(
        {
            "blocks": [],
            "form": {
                "name": "TestSemanticFrame"
            },
        })";

        const auto runResponseExpectedStr = TStringBuf(R"(
        ResponseBody {
            Layout {
                Cards {
                    Text: "Открываю."
                }
                OutputSpeech: "Открываю"
            }
            SemanticFrame {
                Name: "TestSemanticFrame"
            }
            AnalyticsInfo {
                NlgRenderHistoryRecords {
                    TemplateName: "music_play"
                    PhraseName: "render_result"
                    Language: L_RUS
                }
            }
        }
        Version: "trunk@TEST")");
        ParseProtoText<NScenarios::TScenarioRunResponse>(runResponseExpectedStr);
        const auto& runResponseExpected = ParseProtoText<NScenarios::TScenarioRunResponse>(runResponseExpectedStr);

        const auto& runResponse = Test(runRequestStr, bassResponseStr);
        const auto actualRunResponseStr = TStringBuilder{} << "RunResponse (actual):\n" << SerializeProtoText(*runResponse, /* singleLineMode = */ false);
        UNIT_ASSERT_MESSAGES_EQUAL_C(runResponseExpected, *runResponse, actualRunResponseStr);
    }

    Y_UNIT_TEST(BlockDirectiveMusicPlay) {
        const auto runRequestStr = TStringBuf(R"()");
        const TString bassResponseStr = R"(
        {
            "blocks": [{
                "command_sub_type": "music_smart_speaker_play",
                "command_type": "music_play",
                "data": {
                    "first_track_id": "1710820",
                    "first_track_human_readable": "Queen, Killer Queen",
                    "offset": 0,
                    "session_id": "D9z2A7P5",
                    "uri": ""
                },
                "type": "command"
            }],
            "form": {},
        })";

        const auto runResponseExpectedStr = TStringBuf(R"(
        ResponseBody {
            Layout {
                Cards {
                    Text: "Открываю."
                }
                OutputSpeech: "Открываю"
                Directives {
                    MusicPlayDirective {
                        Name: "music_smart_speaker_play"
                        SessionId: "D9z2A7P5"
                        FirstTrackId: "1710820"
                    }
                }
            }
            AnalyticsInfo {
                Objects {
                    Id: "music.first_track_id"
                    Name: "first_track_id"
                    HumanReadable: "Queen, Killer Queen"
                    FirstTrack {
                        Id: "1710820"
                    }
                }
                NlgRenderHistoryRecords {
                    TemplateName: "music_play"
                    PhraseName: "render_result"
                    Language: L_RUS
                }
            }
        }
        Version: "trunk@TEST")");
        ParseProtoText<NScenarios::TScenarioRunResponse>(runResponseExpectedStr);
        const auto& runResponseExpected = ParseProtoText<NScenarios::TScenarioRunResponse>(runResponseExpectedStr);

        const auto& runResponse = Test(runRequestStr, bassResponseStr);
        const auto actualRunResponseStr = TStringBuilder{} << "RunResponse (actual):\n" << SerializeProtoText(*runResponse, /* singleLineMode = */ false);
        UNIT_ASSERT_MESSAGES_EQUAL_C(runResponseExpected, *runResponse, actualRunResponseStr);
    }

    Y_UNIT_TEST(FormSlotAnswer) {
        const auto runRequestStr = TStringBuf(R"()");
        const TString bassResponseStr = R"(
        {
            "blocks": [],
            "form": {
                "name": "TestSemanticFrame",
                "slots": [{
                    "name": "answer",
                    "optional": true,
                    "type": "music_result",
                    "value": {
                        "coverUri": "TestCoverUri",
                        "first_track": {
                            "album": {
                                "genre": "hardrock",
                                "id": "227551",
                                "title": "The Platinum Collection"
                            },
                            "artists": [{
                                "composer": false,
                                "id": "79215",
                                "is_various": false,
                                "name": "Queen"
                            }],
                            "coverUri": "TestCoverUri",
                            "id": "1710820",
                            "subtype": "music",
                            "title": "Killer Queen",
                            "type": "track",
                            "uri": "TestUri"
                        },
                        "first_track_uri": "TestFirstTrackUri",
                        "id": "79215",
                        "name": "Queen",
                        "session_id": "D9z2A7P5",
                        "subtype": null,
                        "type": "artist",
                        "uri": "TestUri"
                    }
                }]
            }
        })";

        const auto runResponseExpectedStr = TStringBuf(R"(
        ResponseBody {
            Layout {
                Cards {
                    Text: "Открываю: Queen."
                }
                OutputSpeech: "Открываю"
            }
            SemanticFrame {
                Name: "TestSemanticFrame"
                Slots {
                    Name: "answer"
                    Type: "music_result"
                    AcceptedTypes: "music_result"
                }
            }
            AnalyticsInfo {
                NlgRenderHistoryRecords {
                    TemplateName: "music_play"
                    PhraseName: "render_result"
                    Language: L_RUS
                }
            }
        }
        Version: "trunk@TEST")");
        ParseProtoText<NScenarios::TScenarioRunResponse>(runResponseExpectedStr);
        const auto& runResponseExpected = ParseProtoText<NScenarios::TScenarioRunResponse>(runResponseExpectedStr);

        const auto& runResponse = Test(runRequestStr, bassResponseStr);
        const auto actualRunResponseStr = TStringBuilder{} << "RunResponse (actual):\n" << SerializeProtoText(*runResponse, /* singleLineMode = */ false);
        UNIT_ASSERT_MESSAGES_EQUAL_C(runResponseExpected, *runResponse, actualRunResponseStr);
    }

    Y_UNIT_TEST(BlockDivCard) {
        const auto runRequestStr = TStringBuf(R"()");
        const TString bassResponseStr = R"(
        {
            "blocks": [{
                "card_template": "music__artist",
                "data": {
                    "coverUri": "TestCoverUri",
                    "log_id": "music_play_alicesdk",
                    "not_available": false,
                    "playUri": "TestPlayUri",
                    "play_inside_app": true
                },
                "type": "div_card"
            }],
            "form": {
                "slots": [{
                    "name": "answer",
                    "optional": true,
                    "type": "music_result",
                    "value": {
                        "id": "79215",
                        "name": "Queen",
                        "session_id": "D9z2A7P5",
                        "subtype": null,
                        "type": "artist",
                        "uri": "TestUri"
                    }
                }]
            },
        })";
        const auto& runResponse = Test(runRequestStr, bassResponseStr);
        const auto& cards = runResponse->GetResponseBody().GetLayout().GetCards();
        UNIT_ASSERT_VALUES_EQUAL(cards.size(), 1);
        UNIT_ASSERT(cards[0].GetCardCase() == TLayout_TCard::kDivCard);
    }

    Y_UNIT_TEST(BlockAttention) {
        const auto runRequestStr = TStringBuf(R"(
        BaseRequest {
            ClientInfo {
                AppId: "aliced"
            }
        })");
        const TString bassResponseStr = R"(
        {
            "blocks": [
                {
                    "attention_type": "explicit_content",
                    "data": {
                        "code": "may-contain-explicit-content"
                    },
                    "type": "attention"
                },
                {
                    "attention_type": "supports_music_player",
                    "data": null,
                    "type": "attention"
                }
            ],
            "form": {},
        })";

        const auto runResponseExpectedStr = TStringBuf(R"(
        ResponseBody {
            Layout {
                Cards {
                    Text: "Включаю."
                }
                OutputSpeech: "Включаю. Осторожно! Детям лучше этого не слышать."
            }
            AnalyticsInfo {
                NlgRenderHistoryRecords {
                    TemplateName: "music_play"
                    PhraseName: "render_result"
                    Language: L_RUS
                }
            }
        }
        Version: "trunk@TEST")");
        ParseProtoText<NScenarios::TScenarioRunResponse>(runResponseExpectedStr);
        const auto& runResponseExpected = ParseProtoText<NScenarios::TScenarioRunResponse>(runResponseExpectedStr);

        const auto& runResponse = Test(runRequestStr, bassResponseStr);
        const auto actualRunResponseStr = TStringBuilder{} << "RunResponse (actual):\n" << SerializeProtoText(*runResponse, /* singleLineMode = */ false);
        UNIT_ASSERT_MESSAGES_EQUAL_C(runResponseExpected, *runResponse, actualRunResponseStr);
    }

    Y_UNIT_TEST(BlockSuggestMusicOpenUri) {
        const auto runRequestStr = TStringBuf(R"(
        BaseRequest {
            Interfaces {
                SupportsButtons: true
            }
        })");
        const TString bassResponseStr = R"(
        {
            "blocks": [{
                "suggest_type": "music__open_uri",
                "type": "suggest"
            }],
            "form": {
                "slots": [{
                    "name": "answer",
                    "optional": true,
                    "type": "music_result",
                    "value": {
                        "id": "79215",
                        "name": "Queen",
                        "session_id": "D9z2A7P5",
                        "subtype": null,
                        "type": "artist",
                        "uri": "TestUri"
                    }
                }]
            },
        })";

        const auto runResponseExpectedStr = TStringBuf(R"(
        ResponseBody {
            Layout {
                Cards {
                    TextWithButtons {
                        Text: "Открываю: Queen."
                        Buttons {
                            Title: "Слушать на Яндекс.Музыке"
                            ActionId: "1"
                        }
                    }
                }
                OutputSpeech: "Открываю"
                Directives {
                    OpenUriDirective {
                        Name: "render_buttons_open_uri"
                        Uri: "TestUri"
                    }
                }
            }
            FrameActions {
                key: "1"
                value {
                    NluHint {
                        FrameName: "1"
                    }
                    Directives {
                        List {
                            OpenUriDirective {
                                Name: "render_buttons_open_uri"
                                Uri: "TestUri"
                            }
                        }
                    }
                }
            }
            AnalyticsInfo {
                NlgRenderHistoryRecords {
                    TemplateName: "music_play"
                    PhraseName: "render_suggest_caption__music__open_uri"
                    Language: L_RUS
                }
                NlgRenderHistoryRecords {
                    TemplateName: "music_play"
                    PhraseName: "render_suggest_uri__music__open_uri"
                    Language: L_RUS
                }
                NlgRenderHistoryRecords {
                    TemplateName: "music_play"
                    PhraseName: "render_result"
                    Language: L_RUS
                }
            }
        }
        Version: "trunk@TEST")");
        ParseProtoText<NScenarios::TScenarioRunResponse>(runResponseExpectedStr);
        const auto& runResponseExpected = ParseProtoText<NScenarios::TScenarioRunResponse>(runResponseExpectedStr);

        const auto& runResponse = Test(runRequestStr, bassResponseStr);
        const auto actualRunResponseStr = TStringBuilder{} << "RunResponse (actual):\n" << SerializeProtoText(*runResponse, /* singleLineMode = */ false);
        UNIT_ASSERT_MESSAGES_EQUAL_C(runResponseExpected, *runResponse, actualRunResponseStr);
    }

    Y_UNIT_TEST(BlockSuggestMusicOpenUriWithoutAutoAction) {
        const auto runRequestStr = TStringBuf(R"(
        BaseRequest {
            Interfaces {
                SupportsButtons: true
            }
        })");
        const TString bassResponseStr = R"(
        {
            "blocks": [{
                "suggest_type": "music__open_uri",
                "type": "suggest"
            }],
            "form": {
                "slots": [{
                    "name": "answer",
                    "optional": true,
                    "type": "music_result",
                    "value": {
                        "id": "79215",
                        "name": "Queen",
                        "session_id": "D9z2A7P5",
                        "subtype": null,
                        "type": "artist",
                        "uri": "TestUri"
                    }
                }]
            },
        })";

        const auto runResponseExpectedStr = TStringBuf(R"(
        ResponseBody {
            Layout {
                Cards {
                    TextWithButtons {
                        Text: "Открываю: Queen."
                        Buttons {
                            Title: "Слушать на Яндекс.Музыке"
                            ActionId: "1"
                        }
                    }
                }
                OutputSpeech: "Открываю"
            }
            FrameActions {
                key: "1"
                value {
                    NluHint {
                        FrameName: "1"
                    }
                    Directives {
                        List {
                            OpenUriDirective {
                                Name: "render_buttons_open_uri"
                                Uri: "TestUri"
                            }
                        }
                    }
                }
            }
            AnalyticsInfo {
                NlgRenderHistoryRecords {
                    TemplateName: "music_play"
                    PhraseName: "render_suggest_caption__music__open_uri"
                    Language: L_RUS
                }
                NlgRenderHistoryRecords {
                    TemplateName: "music_play"
                    PhraseName: "render_suggest_uri__music__open_uri"
                    Language: L_RUS
                }
                NlgRenderHistoryRecords {
                    TemplateName: "music_play"
                    PhraseName: "render_result"
                    Language: L_RUS
                }
            }
        }
        Version: "trunk@TEST")");
        ParseProtoText<NScenarios::TScenarioRunResponse>(runResponseExpectedStr);
        const auto& runResponseExpected = ParseProtoText<NScenarios::TScenarioRunResponse>(runResponseExpectedStr);

        const auto& runResponse = Test(runRequestStr, bassResponseStr, /* addSuggestAutoAction = */ false);
        const auto actualRunResponseStr = TStringBuilder{} << "RunResponse (actual):\n" << SerializeProtoText(*runResponse, /* singleLineMode = */ false);
        UNIT_ASSERT_MESSAGES_EQUAL_C(runResponseExpected, *runResponse, actualRunResponseStr);
    }

    Y_UNIT_TEST(BlockSuggestMusicSuggestArtist) {
        const auto runRequestStr = TStringBuf(R"()");
        const TString bassResponseStr = R"(
        {
            "blocks": [{
                "data": {
                    "coverUri": "TestCoverUri",
                    "id": "10987",
                    "name": "Elvis Presley",
                    "type": "artist",
                    "uri": "TestUri"
                },
                "form_update": {
                    "name": "TestSemanticFrame",
                    "resubmit": true,
                    "slots": [
                        {
                            "name": "answer",
                            "optional": true,
                            "type": "music_result",
                            "value": {
                                "coverUri": "TestCoverUri",
                                "id": "10987",
                                "name": "Elvis Presley",
                                "type": "artist",
                                "uri": "TestUri"
                            }
                        }
                    ]
                },
                "suggest_type": "music__suggest_artist",
                "type": "suggest"
            }],
            "form": {},
        })";

        // XXX(vitvlkv): Suggest music__suggest_artist is intentionally not rendered, see bass_renderer.cpp::TBassResponseRenderer::Render
        const auto runResponseExpectedStr = TStringBuf(R"(
        ResponseBody {
            Layout {
                Cards {
                    Text: "Открываю."
                }
                OutputSpeech: "Открываю"
                SuggestButtons {
                    ActionButton {
                        Title: "Elvis Presley"
                        ActionId: "1"
                    }
                }
            }
            FrameActions {
                key: "1"
                value {
                    NluHint {
                        FrameName: "1"
                    }
                    Directives {
                        List {
                            TypeTextDirective {
                                Name: "render_buttons_type"
                                Text: "Включи Elvis Presley"
                            }
                        }
                    }
                }
            }
            AnalyticsInfo {
                NlgRenderHistoryRecords {
                    TemplateName: "music_play"
                    PhraseName: "render_suggest_caption__music__suggest_artist"
                    Language: L_RUS
                }
                NlgRenderHistoryRecords {
                    TemplateName: "music_play"
                    PhraseName: "render_suggest_utterance__music__suggest_artist"
                    Language: L_RUS
                }
                NlgRenderHistoryRecords {
                    TemplateName: "music_play"
                    PhraseName: "render_result"
                    Language: L_RUS
                }
            }
        }
        Version: "trunk@TEST"
        )");
        ParseProtoText<NScenarios::TScenarioRunResponse>(runResponseExpectedStr);
        const auto& runResponseExpected = ParseProtoText<NScenarios::TScenarioRunResponse>(runResponseExpectedStr);

        const auto& runResponse = Test(runRequestStr, bassResponseStr);
        const auto actualRunResponseStr = TStringBuilder{} << "RunResponse (actual):\n" << SerializeProtoText(*runResponse, /* singleLineMode = */ false);
        UNIT_ASSERT_MESSAGES_EQUAL_C(runResponseExpected, *runResponse, actualRunResponseStr);
    }
}

Y_UNIT_TEST_SUITE(ParseBassForm) {
    Y_UNIT_TEST(Undefined) {
        UNIT_ASSERT(!NImpl::ParseBassForm(NJson::TJsonValue{NJson::JSON_MAP}).Defined());
    }

    Y_UNIT_TEST(Empty) {
        const auto expected = TStringBuf(R"(Name: "hello")");
        const auto bassForm = TStringBuf(R"({"name": "hello"})");
        CheckBassForm(expected, bassForm);
    }

    Y_UNIT_TEST(SlotCensoring) {
        // NOTE(a-square): TFrame sorts slots by name
        const auto expected = TStringBuf(R"(
            Name: "hello"
            Slots {
                Name: "bar"
                Type: "bar_type"
                AcceptedTypes: "bar_type"
            }
            Slots {
                Name: "foo"
                Type: "foo_type"
                AcceptedTypes: "foo_type"
                Value: "foo_value"
                TypedValue {
                    Type: "foo_type"
                    String: "foo_value"
                }
            }
        )");
        const auto bassForm = TStringBuf(R"({
            "name": "hello",
            "slots": [
                {
                    "name": "bar",
                    "type": "bar_type",
                    "value": {"extended": "value"}
                },
                {
                    "name": "foo",
                    "type": "foo_type",
                    "value": "foo_value"
                }
            ]
        })");
        CheckBassForm(expected, bassForm);
    }
}

} // namespace NAlice::NHollywood
