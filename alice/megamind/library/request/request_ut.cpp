#include "builder.h"

#include <alice/megamind/library/experiments/flags.h>
#include <alice/megamind/library/scenarios/defs/names.h>
#include <alice/megamind/library/testing/speechkit.h>

#include <alice/library/experiments/flags.h>
#include <alice/library/json/json.h>
#include <alice/library/music/defs.h>
#include <alice/library/unittest/message_diff.h>

#include <google/protobuf/struct.pb.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/string/builder.h>

namespace NAlice::NMegamind {
namespace {

constexpr TStringBuf SCENARIO_NAME = "TestScenarioName";
constexpr TStringBuf DIALOG_ID = "TestDialogId";
constexpr TStringBuf DIALOG_ID2 = "TestDialogId2";
constexpr ui64 SERVER_TIME_MS = 12345;

const NJson::TJsonValue BASE_REQUEST = JsonFromString(R"({
    "request": {
        "event":{
            "name":"",
            "type":"text_input",
            "text":"давай поиграем в города"
        }
    }
})");

constexpr auto TEST_SKR_WITH_WHISPER = TStringBuf(R"(
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
        "supports_video_play_directive": true,
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

constexpr auto TEST_SKR_WITHOUT_WHISPER = TStringBuf(R"(
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
        "supports_video_play_directive": true,
        "supports_video_protocol": true
    },
    "request": {
        "experiments": {
            "test_experiment": "1"
        },
        "event": {
            "type": "voice_input",
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
            "server_time_ms" : 12345
        }
    }
}
)");

constexpr auto TEST_SKR_WITH_GUEST_DATA = TStringBuf(R"(
{
    "request": {
        "event": {
            "type": "text_input"
        },
        "additional_options": {
            "guest_user_options": {
                "pers_id": "kiy",
                "status": "Match"
            }
        }
    },
    "guest_user_data": {
        "raw_personal_data": "test_raw_personal_data"
    },
    "enrollment_headers": {
        "headers": [
            {
                "person_id": "kiy",
                "user_type": "GUEST"
            },
            {
                "person_id": "Pers-Id-12345",
                "user_type": "OWNER"
            }
        ]
    }
}
)");

constexpr auto TEST_SKR_WITH_GUEST_DATA_NO_OWNER_ENROLLMENT = TStringBuf(R"(
{
    "request": {
        "event": {
            "type": "text_input"
        },
        "additional_options": {
            "guest_user_options": {
            }
        }
    },
    "enrollment_headers": {
        "headers": [
            {
                "user_type": "GUEST"
            },
            {
                "user_type": "OWNER"
            }
        ]
    }
}
)");

constexpr auto TEST_SKR_WITH_GUEST_DATA_WITHOUT_ENROLLMENT_HEADERS = TStringBuf(R"(
{
    "request": {
        "event": {
            "type": "text_input"
        },
        "additional_options": {
            "guest_user_options": {
            }
        }
    }
}
)");

constexpr auto TEST_SKR_WITHOUT_GUEST_DATA_WITH_ENROLLMENT_HEADERS = TStringBuf(R"(
{
    "request": {
        "event": {
            "type": "text_input"
        },
        "additional_options": {
        }
    },
    "enrollment_headers": {
        "headers": [
            {
                "user_type": "GUEST"
            },
            {
                "person_id": "Pers-Id-12345",
                "user_type": "OWNER"
            }
        ]
    }
}
)");

constexpr auto TEST_SKR_WITHOUT_GUEST_DATA_WITH_ENROLLMENT_HEADERS_NO_OWNER_ENROLLMENT = TStringBuf(R"(
{
    "request": {
        "event": {
            "type": "text_input"
        },
        "additional_options": {
        }
    },
    "enrollment_headers": {
        "headers": [
            {
                "person_id": "kiy",
                "user_type": "GUEST"
            },
            {
                "user_type": "OWNER"
            }
        ]
    }
}
)");

constexpr auto TEST_TEXT_SKR = TStringBuf(R"({
    "request": {
        "event": {
            "type": "text_input"
        },
        "additional_options": {
            "server_time_ms" : 12345
        }
    }
})");

constexpr auto TEST_TEXT_WITH_ZERO_SERVER_TIME = TStringBuf(R"({
    "request": {
        "event": {
            "type": "text_input"
        },
        "additional_options": {
            "server_time_ms" : 0
        }
    }
})");

template <typename TFunc>
TTestSpeechKitRequest CreateSpeechKitRequest(TFunc&& onProto) {
    return TSpeechKitRequestBuilder(TSpeechKitRequestBuilder::EPredefined::MinimalWithTextEvent)
        .SetProtoPatcher(onProto)
        .Build();
}

TTestSpeechKitRequest CreateSpeechKitRequest() {
    return TSpeechKitRequestBuilder(TSpeechKitRequestBuilder::EPredefined::MinimalWithTextEvent).Build();
}

void CheckRequestFromProto(const TRequest& request) {
    const auto requestProto = request.ToProto();
    const auto newRequest = CreateRequest(requestProto);
    UNIT_ASSERT_MESSAGES_EQUAL(requestProto, newRequest.ToProto());
}

TRequest CreateTestRequest(TMaybe<bool> forcedShouldListen, TMaybe<TDirectiveChannel::EDirectiveChannel> channel) {
    TVector<TSemanticFrame> semanticFrames;
    TSemanticFrame frame;
    frame.SetName("1_SF");
    semanticFrames.push_back(frame);
    frame.SetName("2_SF");
    semanticFrames.push_back(frame);

    TVector<TSemanticFrame> allParsedSemanticFrames;
    frame.SetName("3_SF");
    allParsedSemanticFrames.push_back(frame);

    TStackEngineCore stackCore;
    stackCore.SetSessionId("stack_session");

    NAlice::NScenarios::TInterfaces interfaces;
    interfaces.SetHasScreen(true);

    NAlice::NScenarios::TOptions options;
    options.SetUserAgent("user_agnt");

    NAlice::NScenarios::TUserPreferences userPreferences;
    userPreferences.SetFiltrationMode(NAlice::NScenarios::TUserPreferences_EFiltrationMode_Moderate);

    NAlice::NScenarios::TUserClassification userClassification;
    userClassification.SetAge(NAlice::NScenarios::TUserClassification_EAge_Adult);

    TIoTUserInfo iot;
    iot.SetRawUserInfo("raw_info");

    NAlice::NData::TContactsList contactsList;
    contactsList.SetIsKnownUuid(true);

    auto& contact = *contactsList.AddContacts();
    contact.SetId(43);
    contact.SetLookupKey("abc");
    contact.SetAccountName("test@gmail.com");
    contact.SetFirstName("Test");
    contact.SetContactId(123);
    contact.SetAccountType("com.google");
    contact.SetDisplayName("Test");

    auto& phone = *contactsList.AddPhones();
    phone.SetId(44);
    phone.SetLookupKey("abc");
    phone.SetAccountType("com.google");
    phone.SetPhone("+79121234567");
    phone.SetType("mobile");

    TOrigin origin;
    origin.SetDeviceId("another-device-id");
    origin.SetUuid("another-uuid");

    const auto speechKitRequest = TSpeechKitRequestBuilder(BASE_REQUEST).Build();

    return std::move(TRequestBuilder{IEvent::CreateEvent(speechKitRequest.Event())}
        .SetDialogId("test_dialog_id")
        .SetScenarioDialogId("test_scenario_dialog_id")
        .SetScenario(TRequest::TScenarioInfo{"scenario_name", EScenarioNameSource::DialogId})
        .SetLocation(TRequest::TLocation{1,2,3,4,5})
        .SetUserLocation(TUserLocation{"tld", 1, "time_zone", 2})
        .SetContentRestrictionLevel(EContentRestrictionLevel::Medium)
        .SetSemanticFrames(semanticFrames)
        .SetRecognizedActionEffectFrames({frame})
        .SetStackEngineCore(stackCore)
        .SetServerTimeMs(12345)
        .SetInterfaces(interfaces)
        .SetOptions(options)
        .SetUserPreferences(userPreferences)
        .SetUserClassification(userClassification)
        .SetParameters(TRequest::TParameters{forcedShouldListen, channel})
        .SetRequestSource(NAlice::NScenarios::TScenarioBaseRequest_ERequestSourceType_GetNext)
        .SetIotUserInfo(iot)
        .SetContactsList(contactsList)
        .SetOrigin(origin)
        .SetIsWarmUp(true)
        .SetAllParsedSemanticFrames(allParsedSemanticFrames)
        .SetDisableVoiceSession(true)
        .SetDisableShouldListen(true))
        .Build();
}

} // namespace

Y_UNIT_TEST_SUITE(TestRequest) {
    Y_UNIT_TEST(TestGuestData) {
        auto speechKitRequest = TSpeechKitRequestBuilder{JsonFromString(TEST_SKR_WITH_GUEST_DATA)}.Build();
        const auto request = CreateRequest(IEvent::CreateEvent(speechKitRequest.Event()), speechKitRequest);
        UNIT_ASSERT(request.GetGuestData().Defined());
        UNIT_ASSERT(request.GetGuestOptions().Defined());
        UNIT_ASSERT_STRINGS_EQUAL(request.GetGuestData()->GetRawPersonalData(), "test_raw_personal_data");
        UNIT_ASSERT_STRINGS_EQUAL(CreateRequest(request.ToProto()).GetGuestData()->GetRawPersonalData(), "test_raw_personal_data");
        UNIT_ASSERT_STRINGS_EQUAL(TRequestBuilder{request}.Build().GetGuestData()->GetRawPersonalData(), "test_raw_personal_data");
        UNIT_ASSERT_STRINGS_EQUAL(request.GetGuestOptions()->GetPersId(), "kiy");
        UNIT_ASSERT_STRINGS_EQUAL(CreateRequest(request.ToProto()).GetGuestOptions()->GetPersId(), "kiy");
        UNIT_ASSERT_STRINGS_EQUAL(TRequestBuilder{request}.Build().GetGuestOptions()->GetPersId(), "kiy");
        UNIT_ASSERT_EQUAL(request.GetGuestOptions()->GetStatus(), TGuestOptions::Match);
        UNIT_ASSERT_EQUAL(CreateRequest(request.ToProto()).GetGuestOptions()->GetStatus(), TGuestOptions::Match);
        UNIT_ASSERT_EQUAL(TRequestBuilder{request}.Build().GetGuestOptions()->GetStatus(), TGuestOptions::Match);
        UNIT_ASSERT(request.GetGuestOptions()->HasIsOwnerEnrolled() && request.GetGuestOptions()->GetIsOwnerEnrolled());
        UNIT_ASSERT(CreateRequest(request.ToProto()).GetGuestOptions()->HasIsOwnerEnrolled() && CreateRequest(request.ToProto()).GetGuestOptions()->GetIsOwnerEnrolled());
        UNIT_ASSERT(TRequestBuilder{request}.Build().GetGuestOptions()->HasIsOwnerEnrolled() && TRequestBuilder{request}.Build().GetGuestOptions()->GetIsOwnerEnrolled());
    }
    Y_UNIT_TEST(TestGuestDataNoOwnerEnrollment) {
        auto speechKitRequest = TSpeechKitRequestBuilder{JsonFromString(TEST_SKR_WITH_GUEST_DATA_NO_OWNER_ENROLLMENT)}.Build();
        const auto request = CreateRequest(IEvent::CreateEvent(speechKitRequest.Event()), speechKitRequest);
        UNIT_ASSERT(request.GetGuestOptions().Defined());
        UNIT_ASSERT(request.GetGuestOptions()->HasIsOwnerEnrolled() && !request.GetGuestOptions()->GetIsOwnerEnrolled());
        UNIT_ASSERT(CreateRequest(request.ToProto()).GetGuestOptions()->HasIsOwnerEnrolled() && !CreateRequest(request.ToProto()).GetGuestOptions()->GetIsOwnerEnrolled());
        UNIT_ASSERT(TRequestBuilder{request}.Build().GetGuestOptions()->HasIsOwnerEnrolled() && !TRequestBuilder{request}.Build().GetGuestOptions()->GetIsOwnerEnrolled());
    }
    Y_UNIT_TEST(TestGuestDataWithoutEnrollmentHeaders) {
        auto speechKitRequest = TSpeechKitRequestBuilder{JsonFromString(TEST_SKR_WITH_GUEST_DATA_WITHOUT_ENROLLMENT_HEADERS)}.Build();
        const auto request = CreateRequest(IEvent::CreateEvent(speechKitRequest.Event()), speechKitRequest);
        UNIT_ASSERT(request.GetGuestOptions().Defined());
        UNIT_ASSERT(request.GetGuestOptions()->HasIsOwnerEnrolled() && !request.GetGuestOptions()->GetIsOwnerEnrolled());
        UNIT_ASSERT(CreateRequest(request.ToProto()).GetGuestOptions()->HasIsOwnerEnrolled() && !CreateRequest(request.ToProto()).GetGuestOptions()->GetIsOwnerEnrolled());
        UNIT_ASSERT(TRequestBuilder{request}.Build().GetGuestOptions()->HasIsOwnerEnrolled() && !TRequestBuilder{request}.Build().GetGuestOptions()->GetIsOwnerEnrolled());
    }
    Y_UNIT_TEST(TestNoGuestDataWithEnrollmentHeaders) {
        auto speechKitRequest = TSpeechKitRequestBuilder{JsonFromString(TEST_SKR_WITHOUT_GUEST_DATA_WITH_ENROLLMENT_HEADERS)}.Build();
        const auto request = CreateRequest(IEvent::CreateEvent(speechKitRequest.Event()), speechKitRequest);
        UNIT_ASSERT(request.GetGuestOptions().Defined());
        UNIT_ASSERT_EQUAL(request.GetGuestOptions()->GetStatus(), TGuestOptions::NoMatch);
        UNIT_ASSERT_EQUAL(CreateRequest(request.ToProto()).GetGuestOptions()->GetStatus(), TGuestOptions::NoMatch);
        UNIT_ASSERT_EQUAL(TRequestBuilder{request}.Build().GetGuestOptions()->GetStatus(), TGuestOptions::NoMatch);
        UNIT_ASSERT(request.GetGuestOptions()->HasIsOwnerEnrolled() && request.GetGuestOptions()->GetIsOwnerEnrolled());
        UNIT_ASSERT(CreateRequest(request.ToProto()).GetGuestOptions()->HasIsOwnerEnrolled() && CreateRequest(request.ToProto()).GetGuestOptions()->GetIsOwnerEnrolled());
        UNIT_ASSERT(TRequestBuilder{request}.Build().GetGuestOptions()->HasIsOwnerEnrolled() && TRequestBuilder{request}.Build().GetGuestOptions()->GetIsOwnerEnrolled());
    }
    Y_UNIT_TEST(TestNoGuestDataWithEnrollmentHeadersNoOwnerEnrollment) {
        auto speechKitRequest = TSpeechKitRequestBuilder{JsonFromString(TEST_SKR_WITHOUT_GUEST_DATA_WITH_ENROLLMENT_HEADERS_NO_OWNER_ENROLLMENT)}.Build();
        const auto request = CreateRequest(IEvent::CreateEvent(speechKitRequest.Event()), speechKitRequest);
        UNIT_ASSERT(request.GetGuestOptions().Defined());
        UNIT_ASSERT_EQUAL(request.GetGuestOptions()->GetStatus(), TGuestOptions::NoMatch);
        UNIT_ASSERT_EQUAL(CreateRequest(request.ToProto()).GetGuestOptions()->GetStatus(), TGuestOptions::NoMatch);
        UNIT_ASSERT_EQUAL(TRequestBuilder{request}.Build().GetGuestOptions()->GetStatus(), TGuestOptions::NoMatch);
        UNIT_ASSERT(request.GetGuestOptions()->HasIsOwnerEnrolled() && !request.GetGuestOptions()->GetIsOwnerEnrolled());
        UNIT_ASSERT(CreateRequest(request.ToProto()).GetGuestOptions()->HasIsOwnerEnrolled() && !CreateRequest(request.ToProto()).GetGuestOptions()->GetIsOwnerEnrolled());
        UNIT_ASSERT(TRequestBuilder{request}.Build().GetGuestOptions()->HasIsOwnerEnrolled() && !TRequestBuilder{request}.Build().GetGuestOptions()->GetIsOwnerEnrolled());
    }
    Y_UNIT_TEST(TestIsWarmUp) {
        auto speechKitRequest = TSpeechKitRequestBuilder{JsonFromString(TEST_TEXT_SKR)}.Build();
        const auto request = CreateRequest(IEvent::CreateEvent(speechKitRequest.Event()), speechKitRequest,
                                           /* iotUserInfo= */ Nothing(),
                                           /* requestSource= */ {},
                                           /* semanticFrames= */ {},
                                           /* recognizedActionEffectFrames= */ {},
                                           /* stackEngineCore= */ {},
                                           /* parameters= */ {},
                                           /* contactsList= */ Nothing(),
                                           /* origin= */ Nothing(),
                                           /* lastWhisperTimeMs= */ 2132,
                                           /* whisperTtlMs= */ 0,
                                           /* callbackOwnerScenario= */ Nothing(),
                                           /* whisperConfig= */ Nothing(),
                                           /* logger= */ TRTLogger::NullLogger(),
                                           /* isWarmUp= */ true);
        UNIT_ASSERT(request.IsWarmUp());
        UNIT_ASSERT(CreateRequest(request.ToProto()).IsWarmUp());
        UNIT_ASSERT(TRequestBuilder{request}.Build().IsWarmUp());
    }
    Y_UNIT_TEST(TestIsNoWarmUp) {
        auto speechKitRequest = TSpeechKitRequestBuilder{JsonFromString(TEST_TEXT_SKR)}.Build();
        const auto request = CreateRequest(IEvent::CreateEvent(speechKitRequest.Event()), speechKitRequest,
                                           /* iotUserInfo= */ Nothing(),
                                           /* requestSource= */ {},
                                           /* semanticFrames= */ {},
                                           /* recognizedActionEffectFrames= */ {},
                                           /* stackEngineCore= */ {},
                                           /* parameters= */ {},
                                           /* contactsList= */ Nothing(),
                                           /* origin= */ Nothing(),
                                           /* lastWhisperTimeMs= */ 2132,
                                           /* whisperTtlMs= */ 0,
                                           /* callbackOwnerScenario= */ Nothing(),
                                           /* whisperConfig= */ Nothing(),
                                           /* logger= */ TRTLogger::NullLogger(),
                                           /* isWarmUp= */ false);
        UNIT_ASSERT(!request.IsWarmUp());
        UNIT_ASSERT(!CreateRequest(request.ToProto()).IsWarmUp());
    }
    Y_UNIT_TEST(TestWhisperVoice) {
        auto speechKitRequest = TSpeechKitRequestBuilder{JsonFromString(TEST_SKR_WITH_WHISPER)}.Build();
        const auto request = CreateRequest(IEvent::CreateEvent(speechKitRequest.Event()), speechKitRequest,
                          /* iotUserInfo= */ Nothing(),
                          /* requestSource= */ {},
                          /* semanticFrames= */ {},
                          /* recognizedActionEffectFrames= */ {},
                          /* stackEngineCore= */ {},
                          /* parameters= */ {},
                          /* contactsList= */ Nothing(),
                          /* origin= */ Nothing(),
                          /* lastWhisperTimeMs= */ 2132,
                          /* whisperTtlMs= */ 0);
        const auto& whisperInfo = request.GetWhisperInfo();
        UNIT_ASSERT(whisperInfo.Defined());
        UNIT_ASSERT(whisperInfo->IsWhisper());
        UNIT_ASSERT_VALUES_EQUAL(whisperInfo->GetUpdatedLastWhisperTimeMs(), SERVER_TIME_MS);
        UNIT_ASSERT(!whisperInfo->IsPreviousRequestWhisper());
    }
    Y_UNIT_TEST(TestWhisperDisabledByConfig) {
        auto speechKitRequest = TSpeechKitRequestBuilder{JsonFromString(TEST_SKR_WITH_WHISPER)}.Build();
        const auto request =
            CreateRequest(IEvent::CreateEvent(speechKitRequest.Event()), speechKitRequest,
                          /* iotUserInfo= */ Nothing(),
                          /* requestSource= */ {},
                          /* semanticFrames= */ {},
                          /* recognizedActionEffectFrames= */ {},
                          /* stackEngineCore= */ {},
                          /* parameters= */ {},
                          /* contactsList= */ Nothing(),
                          /* origin= */ Nothing(),
                          /* lastWhisperTimeMs= */ 2132,
                          /* whisperTtlMs= */ 0,
                          /* callbackOwnerScenario= */ Nothing(), []() -> TRequest::TTtsWhisperConfig {
                              TRequest::TTtsWhisperConfig config;
                              config.SetEnabled(false);
                              return config;
                          }());
        const auto& whisperInfo = request.GetWhisperInfo();
        UNIT_ASSERT(whisperInfo.Defined());
        UNIT_ASSERT(!whisperInfo->IsWhisper());
        UNIT_ASSERT_VALUES_EQUAL(whisperInfo->GetUpdatedLastWhisperTimeMs(), 0);
    }
    Y_UNIT_TEST(TestWhisperNonVoice) {
        auto speechKitRequest = TSpeechKitRequestBuilder{JsonFromString(TEST_TEXT_SKR)}.Build();
        const ui64 lastWhisperTimeMs = 12340;
        const auto request =
            CreateRequest(IEvent::CreateEvent(speechKitRequest.Event()), speechKitRequest,
                          /* iotUserInfo= */ Nothing(),
                          /* requestSource= */ {},
                          /* semanticFrames= */ {},
                          /* recognizedActionEffectFrames= */ {},
                          /* stackEngineCore= */ {},
                          /* parameters= */ {},
                          /* contactsList= */ Nothing(),
                          /* origin= */ Nothing(),
                          lastWhisperTimeMs,
                          /* whisperTtlMs= */ 5);
        const auto& whisperInfo = request.GetWhisperInfo();
        UNIT_ASSERT(whisperInfo.Defined());
        UNIT_ASSERT(whisperInfo->IsWhisper());
        UNIT_ASSERT_VALUES_EQUAL(whisperInfo->GetUpdatedLastWhisperTimeMs(), lastWhisperTimeMs);
        UNIT_ASSERT(whisperInfo->IsPreviousRequestWhisper());
    }
    Y_UNIT_TEST(TestCallbackOwner) {
        auto speechKitRequest = TSpeechKitRequestBuilder{JsonFromString(TEST_TEXT_SKR)}.Build();
        const TString callbackScenarioOwner = "_callback_scenario_owner_";
        const auto request =
            CreateRequest(IEvent::CreateEvent(speechKitRequest.Event()), speechKitRequest,
                          /* iotUserInfo= */ Nothing(),
                          /* requestSource= */ {},
                          /* semanticFrames= */ {},
                          /* recognizedActionEffectFrames= */ {},
                          /* stackEngineCore= */ {},
                          /* parameters= */ {},
                          /* contactsList= */ Nothing(),
                          /* origin= */ Nothing(),
                          /* lastWhisperTimeMs= */ 0,
                          /* whisperTtlMs= */ 0,
                          callbackScenarioOwner);
        const auto& actual = request.GetCallbackOwnerScenario();
        UNIT_ASSERT(actual.Defined());
        UNIT_ASSERT_VALUES_EQUAL(*actual, callbackScenarioOwner);
    }
    Y_UNIT_TEST(TestWhisperVoiceWithinTtl) {
        auto speechKitRequest = TSpeechKitRequestBuilder{JsonFromString(TEST_SKR_WITHOUT_WHISPER)}.Build();
        const auto request =
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
                          /* whisperTtlMs= */ 666);
        const auto& whisperInfo = request.GetWhisperInfo();
        UNIT_ASSERT(whisperInfo.Defined());
        UNIT_ASSERT(!whisperInfo->IsWhisper());
        UNIT_ASSERT_VALUES_EQUAL(whisperInfo->GetUpdatedLastWhisperTimeMs(), 0);
        UNIT_ASSERT(whisperInfo->IsPreviousRequestWhisper());
    }
    Y_UNIT_TEST(TestWhisperNonVoiceTtlZero) {
        auto speechKitRequest = TSpeechKitRequestBuilder{JsonFromString(TEST_TEXT_SKR)}.Build();
        const auto request =
            CreateRequest(IEvent::CreateEvent(speechKitRequest.Event()), speechKitRequest,
                          /* iotUserInfo= */ Nothing(),
                          /* requestSource= */ {},
                          /* semanticFrames= */ {},
                          /* recognizedActionEffectFrames= */ {},
                          /* stackEngineCore= */ {},
                          /* parameters= */ {},
                          /* contactsList= */ Nothing(),
                          /* origin= */ Nothing(),
                          /* lastWhisperTimeMs= */ 12345,
                          /* whisperTtlMs= */ 0);
        const auto& whisperInfo = request.GetWhisperInfo();
        UNIT_ASSERT(whisperInfo.Defined());
        UNIT_ASSERT(!whisperInfo->IsWhisper());
        UNIT_ASSERT(!whisperInfo->IsPreviousRequestWhisper());
    }
    Y_UNIT_TEST(TestWhisperNonVoiceTimeoutExceeded) {
        auto speechKitRequest = TSpeechKitRequestBuilder{JsonFromString(TEST_TEXT_SKR)}.Build();
        const auto request = CreateRequest(IEvent::CreateEvent(speechKitRequest.Event()), speechKitRequest,
                                           /* iotUserInfo= */ Nothing(),
                                           /* requestSource= */ {},
                                           /* semanticFrames= */ {},
                                           /* recognizedActionEffectFrames= */ {},
                                           /* stackEngineCore= */ {},
                                           /* parameters= */ {},
                                           /* contactsList= */ Nothing(),
                                           /* origin= */ Nothing(),
                                           /* lastWhisperTimeMs= */ 12340,
                                           /* whisperTtlMs= */ 4);
        const auto& whisperInfo = request.GetWhisperInfo();
        UNIT_ASSERT(whisperInfo.Defined());
        UNIT_ASSERT(!whisperInfo->IsWhisper());
        UNIT_ASSERT(!whisperInfo->IsPreviousRequestWhisper());
    }
    Y_UNIT_TEST(TestWhisperNonVoiceBrokenServerTime) {
        auto speechKitRequest = TSpeechKitRequestBuilder{JsonFromString(TEST_TEXT_WITH_ZERO_SERVER_TIME)}.Build();
        const auto request = CreateRequest(IEvent::CreateEvent(speechKitRequest.Event()), speechKitRequest,
                                           /* iotUserInfo= */ Nothing(),
                                           /* requestSource= */ {},
                                           /* semanticFrames= */ {},
                                           /* recognizedActionEffectFrames= */ {},
                                           /* stackEngineCore= */ {},
                                           /* parameters= */ {},
                                           /* contactsList= */ Nothing(),
                                           /* origin= */ Nothing(),
                                           /* lastWhisperTimeMs= */ 0,
                                           /* whisperTtlMs= */ 4);
        const auto& whisperInfo = request.GetWhisperInfo();
        UNIT_ASSERT(whisperInfo.Defined());
        UNIT_ASSERT(!whisperInfo->IsWhisper());
        UNIT_ASSERT(!whisperInfo->IsPreviousRequestWhisper());
    }
    Y_UNIT_TEST(TestScenarioNameFromDialogId) {
        static const auto dialogId = TStringBuilder{} << SCENARIO_NAME << ":" << DIALOG_ID;
        auto speechKitRequest =
            CreateSpeechKitRequest([](auto& initCtx) { initCtx.Proto->MutableHeader()->SetDialogId(dialogId); });
        const auto actual = CreateRequest(IEvent::CreateEvent(speechKitRequest.Event()), speechKitRequest);
        CheckRequestFromProto(actual);

        UNIT_ASSERT(actual.GetDialogId().Defined());
        UNIT_ASSERT_VALUES_EQUAL(actual.GetDialogId().GetRef(), dialogId);
        UNIT_ASSERT(actual.GetScenarioDialogId().Defined());
        UNIT_ASSERT_VALUES_EQUAL(actual.GetScenarioDialogId().GetRef(), DIALOG_ID);

        const auto& scenario = actual.GetScenario();

        UNIT_ASSERT(scenario.Defined());
        UNIT_ASSERT_VALUES_EQUAL(scenario->GetName(), SCENARIO_NAME);
        UNIT_ASSERT_VALUES_EQUAL(scenario->GetSource(), EScenarioNameSource::DialogId);
    }

    Y_UNIT_TEST(TestScenarioNameFromDialogIdReturnDefault) {
        auto speechKitRequest = CreateSpeechKitRequest(
            [](auto& initCtx) { initCtx.Proto->MutableHeader()->SetDialogId(TString{DIALOG_ID}); });
        const auto actual = CreateRequest(IEvent::CreateEvent(speechKitRequest.Event()), speechKitRequest);
        CheckRequestFromProto(actual);

        UNIT_ASSERT(actual.GetDialogId().Defined());
        UNIT_ASSERT_VALUES_EQUAL(actual.GetDialogId().GetRef(), DIALOG_ID);
        UNIT_ASSERT(actual.GetScenarioDialogId().Defined());
        UNIT_ASSERT_VALUES_EQUAL(actual.GetScenarioDialogId().GetRef(), DIALOG_ID);

        const auto& scenario = actual.GetScenario();

        UNIT_ASSERT(scenario.Defined());
        UNIT_ASSERT_VALUES_EQUAL(scenario->GetName(), MM_DIALOGOVO_SCENARIO);
        UNIT_ASSERT_VALUES_EQUAL(scenario->GetSource(), EScenarioNameSource::VinsDialogId);
    }

    Y_UNIT_TEST(TestScenarioNameFromServerAction) {
        auto onProto = [](NMegamind::TSpeechKitInitContext& initCtx) {
            auto& event = *initCtx.EventProtoPtr;
            event.SetType(EEventType::server_action);
            auto* payload = event.MutablePayload();
            google::protobuf::Value value;
            value.set_string_value(TString{SCENARIO_NAME});
            payload->mutable_fields()->insert({"@scenario_name", value});
        };
        auto speechKitRequest = CreateSpeechKitRequest(onProto);
        const auto actual = CreateRequest(IEvent::CreateEvent(speechKitRequest.Event()), speechKitRequest);
        CheckRequestFromProto(actual);

        UNIT_ASSERT(!actual.GetDialogId().Defined());
        UNIT_ASSERT(!actual.GetScenarioDialogId().Defined());

        const auto& scenario = actual.GetScenario();

        UNIT_ASSERT(scenario.Defined());
        UNIT_ASSERT_VALUES_EQUAL(scenario->GetName(), SCENARIO_NAME);
        UNIT_ASSERT_VALUES_EQUAL(scenario->GetSource(), EScenarioNameSource::ServerAction);
    }

    Y_UNIT_TEST(TestScenarioNameFromServerActionReturnDefault) {
        auto speechKitRequest =
            CreateSpeechKitRequest([](auto& ctxInit) { ctxInit.EventProtoPtr->SetType(EEventType::server_action); });

        const auto actual = CreateRequest(IEvent::CreateEvent(speechKitRequest.Event()), speechKitRequest);
        CheckRequestFromProto(actual);
        const auto& scenario = actual.GetScenario();

        UNIT_ASSERT(scenario.Defined());
        UNIT_ASSERT_VALUES_EQUAL(scenario->GetName(), MM_PROTO_VINS_SCENARIO);
        UNIT_ASSERT_VALUES_EQUAL(scenario->GetSource(), EScenarioNameSource::VinsServerAction);
    }

    Y_UNIT_TEST(TestScenarioNameFromServerActionReturnDefaultExpDisabled) {
        auto onProto = [](NMegamind::TSpeechKitInitContext& ctxInit) {
            ctxInit.EventProtoPtr->SetType(EEventType::server_action);
            auto& experiments = *ctxInit.Proto->MutableRequest()->MutableExperiments()->MutableStorage();
            experiments[EXP_DISABLE_PROTO_VINS_SCENARIO].SetString(TString{"1"});
        };
        auto speechKitRequest = CreateSpeechKitRequest(onProto);
        const auto actual = CreateRequest(IEvent::CreateEvent(speechKitRequest.Event()), speechKitRequest);
        CheckRequestFromProto(actual);
        const auto& scenario = actual.GetScenario();

        UNIT_ASSERT(scenario.Defined());
        UNIT_ASSERT_VALUES_EQUAL(scenario->GetName(), MM_VINS_SCENARIO);
        UNIT_ASSERT_VALUES_EQUAL(scenario->GetSource(), EScenarioNameSource::VinsServerAction);
    }

    Y_UNIT_TEST(TestScenarioNameFromServerActionReturnDefaultQuasar) {
        auto onProto = [](NMegamind::TSpeechKitInitContext& ctxInit) {
            ctxInit.EventProtoPtr->SetType(EEventType::server_action);
            ctxInit.Proto->MutableApplication()->SetAppId("ru.yandex.quasar");
        };
        auto speechKitRequest = CreateSpeechKitRequest(onProto);

        const auto actual = CreateRequest(IEvent::CreateEvent(speechKitRequest.Event()), speechKitRequest);
        CheckRequestFromProto(actual);

        UNIT_ASSERT(!actual.GetDialogId().Defined());
        UNIT_ASSERT(!actual.GetScenarioDialogId().Defined());

        const auto& scenario = actual.GetScenario();

        UNIT_ASSERT(scenario.Defined());
        UNIT_ASSERT_VALUES_EQUAL(scenario->GetName(), MM_PROTO_VINS_SCENARIO);
        UNIT_ASSERT_VALUES_EQUAL(scenario->GetSource(), EScenarioNameSource::VinsServerAction);
    }

    Y_UNIT_TEST(TestScenarioNameFromServerActionWithDifferentDialogId) {
        static const auto dialogIdScenarioName = TStringBuilder{} << SCENARIO_NAME << "FromDialogId";
        static const auto dialogId = TStringBuilder{} << dialogIdScenarioName << ":" << DIALOG_ID;
        auto onProto = [](NMegamind::TSpeechKitInitContext& initCtx) {
            auto& proto = initCtx.Proto;
            auto& event = *initCtx.EventProtoPtr;
            proto->MutableHeader()->SetDialogId(dialogId);
            event.SetType(EEventType::server_action);
            auto* payload = event.MutablePayload();
            google::protobuf::Value value;
            value.set_string_value(TString{SCENARIO_NAME});
            payload->mutable_fields()->insert({"@scenario_name", value});
        };
        auto speechKitRequest = CreateSpeechKitRequest(onProto);

        const auto actual = CreateRequest(IEvent::CreateEvent(speechKitRequest.Event()), speechKitRequest);
        CheckRequestFromProto(actual);

        UNIT_ASSERT(actual.GetDialogId().Defined());
        UNIT_ASSERT_VALUES_EQUAL(actual.GetDialogId().GetRef(), dialogId);
        UNIT_ASSERT(actual.GetScenarioDialogId().Defined());
        UNIT_ASSERT_VALUES_EQUAL(actual.GetScenarioDialogId().GetRef(), DIALOG_ID);

        const auto& scenario = actual.GetScenario();

        UNIT_ASSERT(scenario.Defined());
        UNIT_ASSERT_VALUES_EQUAL(scenario->GetName(), SCENARIO_NAME);
        UNIT_ASSERT_VALUES_EQUAL(scenario->GetSource(), EScenarioNameSource::ServerAction);
    }

    Y_UNIT_TEST(TestScenarioNameFromServerActionWithSameDialogId) {
        static const auto dialogId = TStringBuilder{} << SCENARIO_NAME << ":" << DIALOG_ID;
        auto onProto = [](NMegamind::TSpeechKitInitContext& initCtx) {
            auto& proto = initCtx.Proto;
            auto& event = *initCtx.EventProtoPtr;
            proto->MutableHeader()->SetDialogId(dialogId);
            event.SetType(EEventType::server_action);
            auto* payload = event.MutablePayload();
            google::protobuf::Value value;
            value.set_string_value(TString{SCENARIO_NAME});
            payload->mutable_fields()->insert({"@scenario_name", value});
        };
        auto speechKitRequest = CreateSpeechKitRequest(onProto);
        const auto actual = CreateRequest(IEvent::CreateEvent(speechKitRequest.Event()), speechKitRequest);
        CheckRequestFromProto(actual);

        UNIT_ASSERT(actual.GetDialogId().Defined());
        UNIT_ASSERT_VALUES_EQUAL(actual.GetDialogId().GetRef(), dialogId);
        UNIT_ASSERT(actual.GetScenarioDialogId().Defined());
        UNIT_ASSERT_VALUES_EQUAL(actual.GetScenarioDialogId().GetRef(), DIALOG_ID);

        const auto& scenario = actual.GetScenario();

        UNIT_ASSERT(scenario.Defined());
        UNIT_ASSERT_VALUES_EQUAL(scenario->GetName(), SCENARIO_NAME);
        UNIT_ASSERT_VALUES_EQUAL(scenario->GetSource(), EScenarioNameSource::ServerAction);
    }

    Y_UNIT_TEST(TestScenarioNameWithoutDialogIdNorServerAction) {
        auto speechKitRequest = CreateSpeechKitRequest();
        const auto actual = CreateRequest(IEvent::CreateEvent(speechKitRequest.Event()), speechKitRequest);
        CheckRequestFromProto(actual);

        UNIT_ASSERT(!actual.GetDialogId().Defined());
        UNIT_ASSERT(!actual.GetScenarioDialogId().Defined());
        const auto& scenario = actual.GetScenario();
        UNIT_ASSERT(!scenario.Defined());
    }

    Y_UNIT_TEST(TestScenarioNewDialogSessionWithDialogId) {
        static const auto dialogId = TStringBuilder{} << SCENARIO_NAME << ":" << DIALOG_ID;
        auto onProto = [](NMegamind::TSpeechKitInitContext& initCtx) {
            auto& proto = initCtx.Proto;
            auto& event = *initCtx.EventProtoPtr;
            proto->MutableHeader()->SetDialogId(dialogId);
            event.SetType(EEventType::server_action);
            event.SetName("new_dialog_session");
            auto* payload = event.MutablePayload();
            google::protobuf::Value value;
            value.set_string_value(TString{DIALOG_ID2});
            payload->mutable_fields()->insert({"dialog_id", value});
        };

        auto speechKitRequest = CreateSpeechKitRequest(onProto);
        const auto actual = CreateRequest(IEvent::CreateEvent(speechKitRequest.Event()), speechKitRequest);
        CheckRequestFromProto(actual);

        UNIT_ASSERT(actual.GetDialogId().Defined());
        UNIT_ASSERT_VALUES_EQUAL(actual.GetDialogId().GetRef(), dialogId);
        UNIT_ASSERT(actual.GetScenarioDialogId().Defined());
        UNIT_ASSERT_VALUES_EQUAL(actual.GetScenarioDialogId().GetRef(), DIALOG_ID);

        const auto& scenario = actual.GetScenario();
        UNIT_ASSERT(scenario.Defined());
        UNIT_ASSERT_VALUES_EQUAL(scenario->GetName(), SCENARIO_NAME);
        UNIT_ASSERT_VALUES_EQUAL(scenario->GetSource(), EScenarioNameSource::DialogId);
    }

    Y_UNIT_TEST(TestScenarioNewDialogSessionWithScenarioName) {
        auto onProto = [](NMegamind::TSpeechKitInitContext& initCtx) {
            auto proto = initCtx.Proto;
            auto& event = *initCtx.EventProtoPtr;
            proto->MutableHeader()->SetDialogId(ToString(DIALOG_ID));
            event.SetType(EEventType::server_action);
            event.SetName("new_dialog_session");

            auto* payload = event.MutablePayload();
            google::protobuf::Value value;
            value.set_string_value(TString{SCENARIO_NAME});
            payload->mutable_fields()->insert({"@scenario_name", value});
        };
        auto speechKitRequest = CreateSpeechKitRequest(onProto);
        const auto actual = CreateRequest(IEvent::CreateEvent(speechKitRequest.Event()), speechKitRequest);
        CheckRequestFromProto(actual);

        UNIT_ASSERT(actual.GetDialogId().Defined());
        UNIT_ASSERT_VALUES_EQUAL(actual.GetDialogId().GetRef(), TString{DIALOG_ID});
        UNIT_ASSERT(actual.GetScenarioDialogId().Defined());
        UNIT_ASSERT_VALUES_EQUAL(actual.GetScenarioDialogId().GetRef(), DIALOG_ID);

        const auto& scenario = actual.GetScenario();
        UNIT_ASSERT(scenario.Defined());
        UNIT_ASSERT_VALUES_EQUAL(scenario->GetName(), SCENARIO_NAME);
        UNIT_ASSERT_VALUES_EQUAL(scenario->GetSource(), EScenarioNameSource::ServerAction);
    }

    Y_UNIT_TEST(TestDialogovoScenarioName) {
        auto onProto = [](NMegamind::TSpeechKitInitContext& initCtx) {
            auto& proto = initCtx.Proto;
            proto->MutableHeader()->SetDialogId(TString{DIALOG_ID});
            (*proto->MutableRequest()->MutableExperiments()->MutableStorage())["mm_enable_protocol_scenario=Dialogovo"]
                .SetString("");
        };
        auto speechKitRequest = CreateSpeechKitRequest(onProto);
        const auto actual = CreateRequest(IEvent::CreateEvent(speechKitRequest.Event()), speechKitRequest);
        CheckRequestFromProto(actual);

        UNIT_ASSERT(actual.GetDialogId().Defined());
        UNIT_ASSERT_VALUES_EQUAL(actual.GetDialogId().GetRef(), DIALOG_ID);
        UNIT_ASSERT(actual.GetScenarioDialogId().Defined());
        UNIT_ASSERT_VALUES_EQUAL(actual.GetScenarioDialogId().GetRef(), DIALOG_ID);

        const auto& scenario = actual.GetScenario();

        UNIT_ASSERT(scenario.Defined());
        UNIT_ASSERT_VALUES_EQUAL(scenario->GetName(), MM_DIALOGOVO_SCENARIO);
        UNIT_ASSERT_VALUES_EQUAL(scenario->GetSource(), EScenarioNameSource::VinsDialogId);
    }

    Y_UNIT_TEST(TestProtoVinsForwarding) {
        NJson::TJsonValue request{NJson::JSON_MAP};
        request["request"]["experiments"][EXP_ENABLE_PROTO_VINS_SCENARIO] = "";
        request["request"]["event"]["type"] = "server_action";

        const auto speechKitRequest = TSpeechKitRequestBuilder(request).Build();
        const auto actual = CreateRequest(IEvent::CreateEvent(speechKitRequest.Event()), speechKitRequest);
        CheckRequestFromProto(actual);
        const auto& scenario = actual.GetScenario();

        UNIT_ASSERT(scenario.Defined());
        UNIT_ASSERT_VALUES_EQUAL(scenario->GetName(), MM_PROTO_VINS_SCENARIO);
        UNIT_ASSERT_VALUES_EQUAL(scenario->GetSource(), EScenarioNameSource::VinsServerAction);
    }

    Y_UNIT_TEST(TestProtoVinsForwardingOnCommonVins) {
        NJson::TJsonValue request;
        request["request"]["experiments"][EXP_ENABLE_PROTO_VINS_SCENARIO] = "";
        request["request"]["event"]["payload"]["@scenario_name"] = MM_VINS_SCENARIO;
        request["request"]["event"]["type"] = "server_action";

        const auto speechKitRequest = TSpeechKitRequestBuilder(request).Build();
        const auto actual = CreateRequest(IEvent::CreateEvent(speechKitRequest.Event()), speechKitRequest);
        CheckRequestFromProto(actual);
        const auto& scenario = actual.GetScenario();

        UNIT_ASSERT(scenario.Defined());
        UNIT_ASSERT_VALUES_EQUAL(scenario->GetName(), MM_PROTO_VINS_SCENARIO);
        UNIT_ASSERT_VALUES_EQUAL(scenario->GetSource(), EScenarioNameSource::ServerAction);
    }

    Y_UNIT_TEST(TestLoadFiltationModeContainsMediumByDefault) {
        NJson::TJsonValue request = BASE_REQUEST;

        const auto speechKitRequest = TSpeechKitRequestBuilder(request).Build();
        const auto req = CreateRequest(IEvent::CreateEvent(speechKitRequest.Event()), speechKitRequest);
        CheckRequestFromProto(req);

        UNIT_ASSERT_VALUES_EQUAL(req.GetContentRestrictionLevel(), EContentRestrictionLevel::Medium);
    }

    Y_UNIT_TEST(TestLoadFiltationModeFromAdditionOptions) {
        for (const auto [level, expected] : THashMap<int, EContentRestrictionLevel>{
                 {0, EContentRestrictionLevel::Without},
                 {1, EContentRestrictionLevel::Medium},
                 {2, EContentRestrictionLevel::Children},
                 {3, EContentRestrictionLevel::Safe},
             }) {
            NJson::TJsonValue request = BASE_REQUEST;
            request["request"]["additional_options"]["bass_options"]["filtration_level"] = level;

            const auto speechKitRequest = TSpeechKitRequestBuilder(request).Build();
            const auto req = CreateRequest(IEvent::CreateEvent(speechKitRequest.Event()), speechKitRequest);
            CheckRequestFromProto(req);

            UNIT_ASSERT_VALUES_EQUAL(req.GetContentRestrictionLevel(), expected);
        }
    }

    Y_UNIT_TEST(TestLoadFiltationModeFromContentSettings) {
        for (const auto [level, expected] : THashMap<TStringBuf, EContentRestrictionLevel>{
                 {TStringBuf("without"), EContentRestrictionLevel::Without},
                 {TStringBuf("medium"), EContentRestrictionLevel::Medium},
                 {TStringBuf("children"), EContentRestrictionLevel::Children},
                 {TStringBuf("safe"), EContentRestrictionLevel::Safe},
             }) {
            NJson::TJsonValue request = BASE_REQUEST;
            request["request"]["device_state"]["device_config"]["content_settings"] = level;

            const auto speechKitRequest = TSpeechKitRequestBuilder(request).Build();
            const auto req = CreateRequest(IEvent::CreateEvent(speechKitRequest.Event()), speechKitRequest);
            CheckRequestFromProto(req);

            UNIT_ASSERT_VALUES_EQUAL(req.GetContentRestrictionLevel(), expected);
        }
    }

    Y_UNIT_TEST(TestLoadFiltrationModeFromChildContentSettings) {
        NJson::TJsonValue request = BASE_REQUEST;
        auto& requestJson = request["request"];
        auto& deviceConfigJson = requestJson["device_state"]["device_config"];
        deviceConfigJson["content_settings"] = TStringBuf("without");
        deviceConfigJson["child_content_settings"] = TStringBuf("safe");

        {
            const auto speechKitRequest = TSpeechKitRequestBuilder(request).Build();
            const auto req = CreateRequest(IEvent::CreateEvent(speechKitRequest.Event()), speechKitRequest);
            CheckRequestFromProto(req);

            UNIT_ASSERT_VALUES_EQUAL(req.GetContentRestrictionLevel(), EContentRestrictionLevel::Without);
        }

        auto& classification = requestJson["event"]["biometry_classification"]["simple"][0];
        classification["classname"] = TStringBuf("child");
        classification["tag"] = TStringBuf("children");

        {
            const auto speechKitRequest = TSpeechKitRequestBuilder(request).Build();
            const auto req = CreateRequest(IEvent::CreateEvent(speechKitRequest.Event()), speechKitRequest);
            CheckRequestFromProto(req);

            UNIT_ASSERT_VALUES_EQUAL(req.GetContentRestrictionLevel(), EContentRestrictionLevel::Safe);
        }

        deviceConfigJson.EraseValue("child_content_settings");

        {
            const auto speechKitRequest = TSpeechKitRequestBuilder(request).Build();
            const auto req = CreateRequest(IEvent::CreateEvent(speechKitRequest.Event()), speechKitRequest);
            CheckRequestFromProto(req);

            UNIT_ASSERT_VALUES_EQUAL(req.GetContentRestrictionLevel(), EContentRestrictionLevel::Children);
        }
    }

    Y_UNIT_TEST(TestLoadGenderSettings) {
        const auto male = "male";
        const auto female = "female";

        for (const auto& gender : {"unknown", male, female}) {
            NJson::TJsonValue request = BASE_REQUEST;
            auto& requestJson = request["request"];

            auto& classification = requestJson["event"]["biometry_classification"]["simple"][0];
            classification["classname"] = TStringBuf(gender);
            classification["tag"] = TStringBuf("gender");

            const auto speechKitRequest = TSpeechKitRequestBuilder(request).Build();
            const auto req = CreateRequest(IEvent::CreateEvent(speechKitRequest.Event()), speechKitRequest);
            CheckRequestFromProto(req);

            UNIT_ASSERT_VALUES_EQUAL(req.GetIsClassifiedAsMaleRequest(), (gender == male ? true : false));
            UNIT_ASSERT_VALUES_EQUAL(req.GetIsClassifiedAsFemaleRequest(), (gender == female ? true : false));
        }
    }

    Y_UNIT_TEST(TestNewDialogSessionWithoutDialogId) {
        auto onProto = [](NMegamind::TSpeechKitInitContext& initCtx) {
            auto& event = *initCtx.EventProtoPtr;
            event.SetType(EEventType::server_action);
            event.SetName("new_dialog_session");
        };
        auto speechKitRequest = CreateSpeechKitRequest(onProto);
        const auto actual = CreateRequest(IEvent::CreateEvent(speechKitRequest.Event()), speechKitRequest);
        CheckRequestFromProto(actual);

        UNIT_ASSERT(actual.GetDialogId().Empty());
        UNIT_ASSERT(actual.GetScenarioDialogId().Empty());

        const auto& scenario = actual.GetScenario();
        UNIT_ASSERT(scenario.Defined());
        UNIT_ASSERT_VALUES_EQUAL(scenario->GetName(), MM_DIALOGOVO_SCENARIO);
        UNIT_ASSERT_VALUES_EQUAL(scenario->GetSource(), EScenarioNameSource::VinsServerAction);
    }

    Y_UNIT_TEST(TestDontUseEventFromSpeechKitRequest) {
        auto onProto = [](NMegamind::TSpeechKitInitContext& initCtx) {
            auto& event = *initCtx.EventProtoPtr;
            event.SetType(EEventType::text_input);
            event.SetText("text");
        };
        auto speechKitRequest = CreateSpeechKitRequest(onProto);
        TEvent event{};
        event.SetType(EEventType::server_action);
        event.SetName("callback_name");
        const auto actual = CreateRequest(IEvent::CreateEvent(event), speechKitRequest);
        CheckRequestFromProto(actual);
        UNIT_ASSERT(actual.GetScenario().Defined());
    }

    Y_UNIT_TEST(TestCallbackForHollywoodMusicNegative) {
        auto onProto = [](NMegamind::TSpeechKitInitContext& initCtx) {
            auto& event = *initCtx.EventProtoPtr;
            event.SetType(EEventType::server_action);
            event.SetName("bass_action");
            (*event.MutablePayload()->mutable_fields())["name"].set_string_value(TString{NMusic::MUSIC_PLAY_OBJECT});
        };
        auto speechKitRequest = CreateSpeechKitRequest(onProto);
        const auto actual = CreateRequest(IEvent::CreateEvent(speechKitRequest.Event()), speechKitRequest);
        CheckRequestFromProto(actual);

        const auto& scenario = actual.GetScenario();
        UNIT_ASSERT(scenario.Defined());
        UNIT_ASSERT_VALUES_EQUAL(scenario->GetName(), MM_PROTO_VINS_SCENARIO);
        UNIT_ASSERT_VALUES_EQUAL(scenario->GetSource(), EScenarioNameSource::VinsServerAction);
    }

    Y_UNIT_TEST(TestCallbackForHollywoodMusicPositive) {
        auto onProto = [](NMegamind::TSpeechKitInitContext& initCtx) {
            auto& event = *initCtx.EventProtoPtr;
            event.SetType(EEventType::server_action);
            event.SetName("bass_action");
            (*event.MutablePayload()->mutable_fields())["name"].set_string_value(TString{NMusic::MUSIC_PLAY_OBJECT});

            auto& experiments = *initCtx.Proto->MutableRequest()->MutableExperiments()->MutableStorage();
            experiments[TString{NExperiments::EXP_HOLLYWOOD_MUSIC_SERVER_ACTION}].SetString(TString{"1"});
        };
        auto speechKitRequest = CreateSpeechKitRequest(onProto);
        const auto actual = CreateRequest(IEvent::CreateEvent(speechKitRequest.Event()), speechKitRequest);
        CheckRequestFromProto(actual);

        const auto& scenario = actual.GetScenario();
        UNIT_ASSERT(scenario.Defined());
        UNIT_ASSERT_VALUES_EQUAL(scenario->GetName(), HOLLYWOOD_MUSIC_SCENARIO);
        UNIT_ASSERT_VALUES_EQUAL(scenario->GetSource(), EScenarioNameSource::ServerAction);
    }

    Y_UNIT_TEST(TestLegacyWeatherForwarding) {
        const NJson::TJsonValue request = JsonFromString(R"({
            "request": {
                "event": {
                    "name": "update_form",
                    "payload": {
                        "form_update": {
                            "push_id": "weather_today",
                            "name": "personal_assistant.scenarios.get_weather",
                            "slots": [
                              {
                                "optional": true,
                                "name": "when",
                                "value": {
                                    "seconds": 0,
                                    "seconds_relative": true
                                },
                                "type": "datetime",
                                "source_text": "сейчас"
                              }
                            ]
                        },
                        "resubmit": true
                    },
                    "type": "server_action"
                }
            }
        })");

        const auto speechKitRequest = TSpeechKitRequestBuilder(request).Build();
        const auto actual = CreateRequest(IEvent::CreateEvent(speechKitRequest.Event()), speechKitRequest);
        CheckRequestFromProto(actual);
        const auto& scenario = actual.GetScenario();

        UNIT_ASSERT(scenario.Defined());
        UNIT_ASSERT_VALUES_EQUAL(scenario->GetName(), HOLLYWOOD_WEATHER_SCENARIO);
        UNIT_ASSERT_VALUES_EQUAL(scenario->GetSource(), EScenarioNameSource::ServerAction);
    }

    Y_UNIT_TEST(TestMegamindCookies) {
        NJson::TJsonValue request = BASE_REQUEST;
        auto cookiesValue = "{\"uaas_tests\":[247071]}";
        request["request"]["megamind_cookies"] = cookiesValue;
        google::protobuf::Struct cookiesStruct;
        google::protobuf::util::JsonStringToMessage(cookiesValue, &cookiesStruct);
        const auto speechKitRequest = TSpeechKitRequestBuilder(request).Build();
        const auto actual = CreateRequest(IEvent::CreateEvent(speechKitRequest.Event()), speechKitRequest);
        CheckRequestFromProto(actual);

        UNIT_ASSERT_MESSAGES_EQUAL(cookiesStruct, actual.GetOptions().GetMegamindCookies());
    }

    Y_UNIT_TEST(TestDoNotUseUserLogs) {
        NJson::TJsonValue request = BASE_REQUEST;
        request["request"]["additional_options"]["do_not_use_user_logs"] = false;
        {
            const auto speechKitRequest = TSpeechKitRequestBuilder(request).Build();
            const auto actual = CreateRequest(IEvent::CreateEvent(speechKitRequest.Event()), speechKitRequest);
            CheckRequestFromProto(actual);
            UNIT_ASSERT_C(actual.GetOptions().GetCanUseUserLogs(), "Options.CanUseUserLogs should be true");
        }
        request["request"]["additional_options"]["do_not_use_user_logs"] = true;
        {
            const auto speechKitRequest = TSpeechKitRequestBuilder(request).Build();
            const auto actual = CreateRequest(IEvent::CreateEvent(speechKitRequest.Event()), speechKitRequest);
            CheckRequestFromProto(actual);
            UNIT_ASSERT_C(!actual.GetOptions().GetCanUseUserLogs(), "Options.CanUseUserLogs should be false");
        }
    }

    Y_UNIT_TEST(TestColoredMicroPromoType) {
        NJson::TJsonValue request = BASE_REQUEST;
        request["application"]["device_model"] = "yandexmicro";
        request["application"]["device_manufacturer"] = "Yandex";
        request["application"]["app_id"] = "aliced";
        request["application"]["device_color"] = "purple";

        const auto speechKitRequest = TSpeechKitRequestBuilder(request).Build();
        const auto actual = CreateRequest(IEvent::CreateEvent(speechKitRequest.Event()), speechKitRequest);
        CheckRequestFromProto(actual);
        UNIT_ASSERT_EQUAL_C(actual.GetOptions().GetPromoType(), NClient::PT_PURPLE_PERSONALITY,
                            ", actual is " << NClient::EPromoType_Name(actual.GetOptions().GetPromoType()));
    }

    Y_UNIT_TEST(TestRequestToProto) {
        const auto request = CreateTestRequest(/* forced_should_listen= */ true,
                                               /* channel */ TDirectiveChannel_EDirectiveChannel_Content);
        CheckRequestFromProto(request);

        const auto requestJson = JsonFromProto(request.ToProto());

        const NJson::TJsonValue expected = JsonFromString(R"({
            "parameters": {
                "forced_sould_listen": true,
                "channel": "Content"
            },
            "content_restriction_level": "Medium",
            "stack_engine_core":{
                "session_id": "stack_session"
            },
            "recognized_action_effect_frames": [{"name":"3_SF"}],
            "user_location": {
                "user_time_zone": "time_zone",
                "user_tld": "tld",
                "user_country": 2,
                "user_region": 1
            },
            "iot_user_info": {
                "raw_user_info": "raw_info"
            },
            "contacts_list": {
                "is_known_uuid": true,
                "contacts": [
                    {
                        "lookup_key": "abc",
                        "_id": 43,
                        "account_name": "test@gmail.com",
                        "first_name": "Test",
                        "contact_id": "123",
                        "account_type": "com.google",
                        "display_name": "Test"
                    }
                ],
                "phones": [
                    {
                        "lookup_key": "abc",
                        "_id": 44,
                        "account_type": "com.google",
                        "phone": "+79121234567",
                        "type": "mobile"
                    }
                ]
            },
            "options": {
                "user_agent": "user_agnt"
            },
            "request_source": "GetNext",
            "scenario_dialog_id": "test_scenario_dialog_id",
            "user_classification":{},
            "semantic_frames": [
                {"name":"1_SF"},
                {"name":"2_SF"}
            ],
            "all_parsed_semantic_frames": [
                {"name":"3_SF"}
            ],
            "interfaces": {
                "has_screen": true
            },
            "dialog_id": "test_dialog_id",
            "user_preferences": {
                "filtration_mode": "Moderate"
            },
            "server_time_ms": "12345",
            "event": {
                "name": "",
                "type": "text_input",
                "text": "давай поиграем в города"
            },
            "location": {
                "speed": 5,
                "longitude": 2,
                "latitude": 1,
                "recency": 4,
                "accuracy": 3
            },
            "scenario": {
                "name": "scenario_name",
                "source": "DialogId"
            },
            "origin": {
                "device_id": "another-device-id",
                "uuid": "another-uuid"
            },
            "is_warm_up": true,
            "disable_voice_session": true,
            "disable_should_listen": true
        })");
        UNIT_ASSERT_VALUES_EQUAL(expected, requestJson);
    }

    Y_UNIT_TEST(TestRequestToProtoParameters) {
        /* testEmptyForcedShouldListen */ {
            const auto request = CreateTestRequest(/* forced_should_listen= */ Nothing(),
                                                   /* channel */ TDirectiveChannel_EDirectiveChannel_Content);
            CheckRequestFromProto(request);
        }
        /* testEmptyChannel */ {
            const auto request = CreateTestRequest(/* forced_should_listen= */ true,
                                                   /* channel */ Nothing());
            CheckRequestFromProto(request);
        }
        /* testEmptyParameters */ {
            const auto request = CreateTestRequest(/* forced_should_listen= */ Nothing(),
                                                   /* channel */ Nothing());
            CheckRequestFromProto(request);
        }
        /* testDefaults */ {
            const auto request = CreateTestRequest(/* forced_should_listen= */ false,
                                                   /* channel */ TDirectiveChannel_EDirectiveChannel{});
            CheckRequestFromProto(request);
        }
    }
}

} // namespace NAlice
