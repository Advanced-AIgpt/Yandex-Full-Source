#include "utils.h"

#include <alice/megamind/library/request/request.h>
#include <alice/megamind/library/request/event/event.h>
#include <alice/megamind/library/testing/mock_context.h>
#include <alice/megamind/library/testing/speechkit.h>

#include <alice/megamind/protos/modifiers/modifier_body.pb.h>
#include <alice/megamind/protos/scenarios/analytics_info.pb.h>
#include <alice/protos/data/language/language.pb.h>
#include <alice/protos/data/contextual_data.pb.h>
#include <google/protobuf/struct.pb.h>

#include <alice/library/json/json.h>
#include <alice/library/unittest/message_diff.h>

#include <library/cpp/langs/langs.h>
#include <library/cpp/testing/unittest/registar.h>

namespace {

using namespace testing;
using namespace NAlice;
using namespace NAlice::NMegamind;
using namespace NAlice::NMegamind::NModifiers;
using namespace NAlice::NScenarios;
using namespace NAlice::NData;

inline constexpr TStringBuf TEST_SCENARIO = "test_scenario";
inline constexpr TStringBuf TEST_OUTPUT_SPEECH = "test_output_speech";
inline constexpr TStringBuf TEST_INTENT = "test_intent";
inline constexpr TStringBuf TEST_REQ_ID = "test_req_id";
inline constexpr TStringBuf TEST_EXPERIMENT = "test_experiment";
inline constexpr TStringBuf TEST_CALLBACK_OWNER_SCENARIO = "_callback_scenario_owner_";
inline constexpr ui64 TEST_RANDOM_SEED = 12345;

constexpr auto TEST_SKR = TStringBuf(R"(
{
    "application": {
        "app_id": "aliced",
        "device_manufacturer": "Yandex",
        "device_model": "yandexmicro",
        "device_color": "tiffany"
    },
    "header": {
        "request_id": "test_req_id",
        "random_seed": 12345
    },
    "interfaces": {
        "can_recognize_music": true,
        "can_server_action": true,
        "has_bluetooth": true,
        "has_microphone": true,
        "has_music_player": true,
        "has_reliable_speakers": true,
        "has_synchronized_push": true,
        "supports_any_player": true,
        "supports_buttons": true,
        "supports_video_play_directive": true,
        "supports_video_player": true,
        "supports_video_protocol": true
    },
    "request": {
        "experiments": {
            "test_experiment": "1"
        },
        "event": {
            "type": "voice_input",
            "asr_whisper": true,
            "asr_result":[
                    {
                        "endOfPhrase":true,
                        "normalized":"продолжи песню.",
                        "confidence":1,
                        "words":[
                            {
                                "value":"продолжи",
                                "confidence":1
                            },
                            {
                                "value":"песню",
                                "confidence":1
                            }
                        ],
                        "utterance":"продолжи песню.",
                    }
                ],
        },
        "device_state": {
            "sound_level": 33212,
            "multiroom": {
                "multiroom_session_id": "kek_session_id"
            }
        },
        "additional_options": {
            "server_time_ms": 12345
        }
    }
}
)");

const TString TEST_WINNER_SCENARIO_NAME = "_test_winner_scenario_name_";

TScenarioConfig BuildScenarioConfig() {
    TScenarioConfig result;
    result.SetName(TEST_WINNER_SCENARIO_NAME);
    result.AddLanguages(::NAlice::ELang::L_RUS);
    return result;
}

TModifierRequest GetDafaultExpectedModifierRequest() {
    TModifierRequest expected;
    *expected.MutableModifierBody()->MutableLayout()->MutableOutputSpeech() = TEST_OUTPUT_SPEECH;
    *expected.MutableFeatures()->MutableProductScenarioName() = TEST_SCENARIO;
    *expected.MutableFeatures()->MutableIntent() = TEST_INTENT;
    *expected.MutableBaseRequest()->MutableRequestId() = TEST_REQ_ID;
    expected.MutableFeatures()->SetPromoType(NClient::PT_GREEN_PERSONALITY);
    expected.MutableFeatures()->MutableContextualData()->MutableProactivity()->SetHint(
        TContextualData_TProactivity_EHint_AlreadyProactive);

    expected.MutableFeatures()->MutableSoundSettings()->SetIsWhisper(true);
    expected.MutableFeatures()->MutableSoundSettings()->SetSoundLevel(33212);
    expected.MutableFeatures()->MutableSoundSettings()->SetMultiroomSessionId("kek_session_id");
    expected.MutableFeatures()->MutableSoundSettings()->SetIsWhisperTagDisabled(true);
    expected.MutableFeatures()->MutableSoundSettings()->SetIsPreviousRequestWhisper(true);

    expected.MutableFeatures()->SetScenarioLanguage(ELang::L_RUS);

    auto& expectedInterfaces = *expected.MutableBaseRequest()->MutableInterfaces();
    expectedInterfaces.SetCanRecognizeMusic(true);
    expectedInterfaces.SetCanServerAction(true);
    expectedInterfaces.SetHasBluetooth(true);
    expectedInterfaces.SetHasMicrophone(true);
    expectedInterfaces.SetHasMusicPlayer(true);
    expectedInterfaces.SetHasReliableSpeakers(true);
    expectedInterfaces.SetHasSynchronizedPush(true);
    expectedInterfaces.SetSupportsAbsoluteVolumeChange(true);
    expectedInterfaces.SetSupportsAnyPlayer(true);
    expectedInterfaces.SetSupportsButtons(true);
    expectedInterfaces.SetSupportsMuteUnmuteVolume(true);
    expectedInterfaces.SetSupportsPlayerContinueDirective(true);
    expectedInterfaces.SetSupportsPlayerDislikeDirective(true);
    expectedInterfaces.SetSupportsPlayerLikeDirective(true);
    expectedInterfaces.SetSupportsPlayerNextTrackDirective(true);
    expectedInterfaces.SetSupportsPlayerPauseDirective(true);
    expectedInterfaces.SetSupportsPlayerPreviousTrackDirective(true);
    expectedInterfaces.SetSupportsPlayerRewindDirective(true);
    expectedInterfaces.SetSupportsVideoPlayDirective(true);
    expectedInterfaces.SetSupportsVideoPlayer(true);
    expectedInterfaces.SetSupportsVideoProtocol(true);

    expected.MutableBaseRequest()->SetUserLanguage(ELang::L_RUS);
    expected.MutableBaseRequest()->SetRandomSeed(TEST_RANDOM_SEED);
    expected.MutableBaseRequest()->SetServerTimeMs(12345);
    auto& experiments = *expected.MutableBaseRequest()->MutableExperiments()->mutable_fields();
    experiments[TString{TEST_EXPERIMENT}].set_string_value("1");
    return expected;
}

Y_UNIT_TEST_SUITE(ModifierUtils) {
    Y_UNIT_TEST(TestConstructModifierRequest) {
        TScenarioResponseBody responseBody;
        {
            *responseBody.MutableLayout()->MutableOutputSpeech() = TEST_OUTPUT_SPEECH;
            *responseBody.MutableAnalyticsInfo()->MutableProductScenarioName() = TEST_SCENARIO;
            *responseBody.MutableAnalyticsInfo()->MutableIntent() = TEST_INTENT;
            responseBody.MutableContextualData()->MutableProactivity()->
                SetHint(TContextualData_TProactivity_EHint_AlreadyProactive);
        }

        auto speechKitRequest = TSpeechKitRequestBuilder{JsonFromString(TEST_SKR)}.Build();

        NAlice::TMockContext ctx;
        EXPECT_CALL(ctx, SpeechKitRequest()).WillRepeatedly(Return(speechKitRequest));
        EXPECT_CALL(ctx, Language()).WillRepeatedly(Return(ELanguage::LANG_RUS));
        TScenarioInfraConfig config;
        config.SetDisableWhisperAsCallbackOwner(true);
        EXPECT_CALL(ctx, ScenarioConfig(TString{TEST_CALLBACK_OWNER_SCENARIO})).WillRepeatedly(ReturnRef(config));

        const auto actual =
            ConstructModifierRequest(responseBody, ctx,
                                     CreateRequest(IEvent::CreateEvent(speechKitRequest.Event()), speechKitRequest,
                                                   /* iotUserInfo= */ Nothing(),
                                                   /* requestSource= */ {},
                                                   /* semanticFrames= */ {},
                                                   /* recognizedActionEffectFrames= */ {},
                                                   /* stackEngineCore= */ {},
                                                   /* parameters= */ {},
                                                   /* contactsList= */ Nothing(),
                                                   /* origin= */ Nothing(),
                                                   /* lastWhisperTimeMs= */ 12340,
                                                   /* whisperTtlMs= */ 5,
                                                   TString{TEST_CALLBACK_OWNER_SCENARIO}),
                                     BuildScenarioConfig());

        TModifierRequest expected = GetDafaultExpectedModifierRequest();

        UNIT_ASSERT_MESSAGES_EQUAL(actual, expected);
    }

    Y_UNIT_TEST(TestConstructModifierRequestWithoutCallbackOwner) {
        TScenarioResponseBody responseBody;
        {
            *responseBody.MutableLayout()->MutableOutputSpeech() = TEST_OUTPUT_SPEECH;
            *responseBody.MutableAnalyticsInfo()->MutableProductScenarioName() = TEST_SCENARIO;
            *responseBody.MutableAnalyticsInfo()->MutableIntent() = TEST_INTENT;
            responseBody.MutableContextualData()->MutableProactivity()->
                SetHint(TContextualData_TProactivity_EHint_AlreadyProactive);
        }

        auto speechKitRequest = TSpeechKitRequestBuilder{JsonFromString(TEST_SKR)}.Build();

        NAlice::TMockContext ctx;
        EXPECT_CALL(ctx, SpeechKitRequest()).WillRepeatedly(Return(speechKitRequest));
        EXPECT_CALL(ctx, Language()).WillRepeatedly(Return(ELanguage::LANG_RUS));
        TScenarioInfraConfig config;
        config.SetDisableWhisperAsCallbackOwner(true);
        EXPECT_CALL(ctx, ScenarioConfig(TString{TEST_WINNER_SCENARIO_NAME})).WillRepeatedly(ReturnRef(config));

        const auto actual =
            ConstructModifierRequest(responseBody, ctx,
                                     CreateRequest(IEvent::CreateEvent(speechKitRequest.Event()), speechKitRequest,
                                                   /* iotUserInfo= */ Nothing(),
                                                   /* requestSource= */ {},
                                                   /* semanticFrames= */ {},
                                                   /* recognizedActionEffectFrames= */ {},
                                                   /* stackEngineCore= */ {},
                                                   /* parameters= */ {},
                                                   /* contactsList= */ Nothing(),
                                                   /* origin= */ Nothing(),
                                                   /* lastWhisperTimeMs= */ 12340,
                                                   /* whisperTtlMs= */ 5,
                                                   /* callbackOwnerScenario= */ Nothing()),
                                     BuildScenarioConfig());

        TModifierRequest expected = GetDafaultExpectedModifierRequest();

        UNIT_ASSERT_MESSAGES_EQUAL(actual, expected);
    }

    Y_UNIT_TEST(TestConstructModifierRequestScenarioLanguage) {
        TScenarioResponseBody responseBody;
        auto speechKitRequest = TSpeechKitRequestBuilder{JsonFromString(TEST_SKR)}.Build();

        auto request = CreateRequest(IEvent::CreateEvent(speechKitRequest.Event()), speechKitRequest,
                                                        /* iotUserInfo= */ Nothing(),
                                                        /* requestSource= */ {},
                                                        /* semanticFrames= */ {},
                                                        /* recognizedActionEffectFrames= */ {},
                                                        /* stackEngineCore= */ {},
                                                        /* parameters= */ {},
                                                        /* contactsList= */ Nothing(),
                                                        /* origin= */ Nothing(),
                                                        /* lastWhisperTimeMs= */ 12340,
                                                        /* whisperTtlMs= */ 5,
                                                        /* callbackOwnerScenario= */ Nothing());

        NAlice::TMockContext ctx;
        EXPECT_CALL(ctx, SpeechKitRequest()).WillRepeatedly(Return(speechKitRequest));
        EXPECT_CALL(ctx, Language()).WillRepeatedly(Return(ELanguage::LANG_ARA));
        TScenarioInfraConfig config;
        config.SetDisableWhisperAsCallbackOwner(true);
        EXPECT_CALL(ctx, ScenarioConfig(TString{TEST_WINNER_SCENARIO_NAME})).WillRepeatedly(ReturnRef(config));

        {
            auto scenarioConfig = BuildScenarioConfig();
            const std::array languages{L_RUS};
            *scenarioConfig.MutableLanguages() = {languages.cbegin(), languages.cend()};
            const auto modifierResponse = ConstructModifierRequest(responseBody, ctx, request, scenarioConfig);
            UNIT_ASSERT_EQUAL(modifierResponse.GetFeatures().GetScenarioLanguage(), ELang::L_RUS);
        }

        {
            auto scenarioConfig = BuildScenarioConfig();
            const std::array languages{L_RUS, L_ARA};
            *scenarioConfig.MutableLanguages() = {languages.cbegin(), languages.cend()};
            const auto modifierResponse = ConstructModifierRequest(responseBody, ctx, request, scenarioConfig);
            UNIT_ASSERT_EQUAL(modifierResponse.GetFeatures().GetScenarioLanguage(), ELang::L_ARA);
        }
    }
}

} // namespace
