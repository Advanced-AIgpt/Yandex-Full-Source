#include "modifier_request_factory.h"

#include "utils.h"

#include <alice/megamind/library/apphost_request/item_adapter.h>
#include <alice/megamind/library/apphost_request/item_names.h>
#include <alice/megamind/library/experiments/flags.h>
#include <alice/megamind/library/request/request.h>
#include <alice/megamind/library/testing/apphost_helpers.h>
#include <alice/megamind/library/testing/mock_context.h>
#include <alice/megamind/library/testing/mock_session.h>

#include <alice/protos/data/language/language.pb.h>
#include <alice/megamind/protos/modifiers/modifier_body.pb.h>
#include <alice/megamind/protos/modifiers/modifier_response.pb.h>
#include <alice/megamind/protos/scenarios/response.pb.h>

#include <alice/library/client/client_info.h>
#include <alice/library/client/protos/client_info.pb.h>

#include <library/cpp/langs/langs.h>
#include <library/cpp/testing/unittest/registar.h>

namespace {

using namespace testing;
using namespace NAlice;
using namespace NAlice::NMegamind;
using namespace NAlice::NMegamind::NModifiers;
using namespace NAlice::NScenarios;
using namespace NAlice::NData;

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
                        "is_whisper":true
                    }
                ],
        },
        "device_state": {
            "sound_level": 33212,
            "multiroom": {
                "multiroom_session_id": "kek_session_id"
            }
        }
    }
}
)");

const TString TEST_WINNER_SCENARIO_NAME = "_test_winner_scenario_name_";

std::unique_ptr<NiceMock<TMockGlobalContext>> GetGlobalCtxMock() {
    auto result = std::make_unique<NiceMock<TMockGlobalContext>>();
    result->GenericInit();
    return result;
}

TClientInfo GetNotYandexMicroClientInfo() {
    return TClientInfo{TClientInfoProto{}};
}

TScenarioResponseBody CreateScenarioResponseBody() {
    TScenarioResponseBody result;
    result.MutableLayout()->SetOutputSpeech("kek");
    return result;
}

TScenarioResponse CreateScenarioResponse() {
    auto result = TScenarioResponse(TEST_WINNER_SCENARIO_NAME, {}, false);
    result.SetResponseBody(CreateScenarioResponseBody());
    return result;
}

TModifierResponse CreateModifierResponse() {
    TModifierResponse result;
    result.MutableModifierBody()->MutableLayout()->SetOutputSpeech("lol");
    return result;
}

class TFixture : public NUnitTest::TBaseFixture {
public:
    TFixture()
        : GlobalCtx_(GetGlobalCtxMock())
        , ApphostCtx_(*GlobalCtx_)
        , ClientInfo_{GetNotYandexMicroClientInfo()}
        , ClientFeatures_(TClientInfoProto(), TRawExpFlags())
    {
    }

    TMockContext& CreateCtx(const ELanguage language) {
        EXPECT_CALL(Ctx_, ClientInfo()).WillRepeatedly(ReturnRef(ClientInfo_));
        EXPECT_CALL(Ctx_, ClientFeatures()).WillRepeatedly(ReturnRef(ClientFeatures_));
        EXPECT_CALL(Ctx_, HasExpFlag(_)).WillRepeatedly(Return(false));
        EXPECT_CALL(Ctx_, Session()).WillRepeatedly(Return(&Session_));
        EXPECT_CALL(Ctx_, ExpFlags()).WillRepeatedly(ReturnRef(Default<THashMap<TString, TMaybe<TString>>>()));
        EXPECT_CALL(Ctx_, HasExpFlag(_)).WillRepeatedly(Return(false));
        EXPECT_CALL(Ctx_, Language()).WillRepeatedly(Return(language));
        EXPECT_CALL(Ctx_, ScenarioConfig(_)).WillRepeatedly(ReturnRef(Default<TScenarioInfraConfig>()));
        EXPECT_CALL(Ctx_, Logger()).WillRepeatedly(ReturnRef(TRTLogger::NullLogger()));

        auto speechKitRequest = TSpeechKitRequestBuilder{JsonFromString(TEST_SKR)}.Build();
        EXPECT_CALL(Ctx_, SpeechKitRequest()).WillRepeatedly(Return(speechKitRequest));
        return Ctx_;
    }

    NAlice::NMegamind::NTesting::TTestAppHostCtx& ApphostCtx() {
        return ApphostCtx_;
    }

    TScenarioConfigRegistry& CreateScenarioConfigRegistry(const TVector<::NAlice::ELang>& languages) {
        ScenarioConfigRegistry_ = TScenarioConfigRegistry();
        auto scenarioConfig = TScenarioConfig();
        scenarioConfig.SetName(TEST_WINNER_SCENARIO_NAME);
        *scenarioConfig.MutableLanguages() = {languages.cbegin(), languages.cend()};
        ScenarioConfigRegistry_.AddScenarioConfig(scenarioConfig);
        return ScenarioConfigRegistry_;
    }

private:
    std::unique_ptr<NiceMock<TMockGlobalContext>> GlobalCtx_;
    NAlice::NMegamind::NTesting::TTestAppHostCtx ApphostCtx_;
    TClientInfo ClientInfo_;
    TClientFeatures ClientFeatures_;
    TMockSession Session_;
    TMockContext Ctx_;
    TScenarioConfigRegistry ScenarioConfigRegistry_;
};

Y_UNIT_TEST_SUITE_F(ModifierRequestFactorySetupRequest, TFixture) {
    Y_UNIT_TEST(TestSetupRequestModifier) {
        auto& ctx = CreateCtx(ELanguage::LANG_RUS);

        auto requestFactory = TAppHostModifierRequestFactory(
            ApphostCtx().ItemProxyAdapter(), ctx, CreateScenarioConfigRegistry({::NAlice::ELang::L_RUS, ::NAlice::ELang::L_ARA}));

        requestFactory.SetupModifierRequest(
            CreateRequest(IEvent::CreateEvent(ctx.SpeechKitRequest().Event()), ctx.SpeechKitRequest()),
            CreateScenarioResponseBody(),
            TEST_WINNER_SCENARIO_NAME);

        const auto modifierRequest = ApphostCtx().TestCtx().GetOnlyProtobufItem<TModifierRequest>(AH_ITEM_MODIFIER_REQUEST);
        UNIT_ASSERT_EQUAL(modifierRequest.GetBaseRequest().GetUserLanguage(), ELang::L_RUS);
        UNIT_ASSERT_EQUAL(modifierRequest.GetFeatures().GetScenarioLanguage(), ELang::L_RUS);
    }
    Y_UNIT_TEST(TestSetupPolyglotRequestModifierRuOnly) {
        auto& ctx = CreateCtx(ELanguage::LANG_ARA);

        auto requestFactory = TAppHostModifierRequestFactory(
            ApphostCtx().ItemProxyAdapter(), ctx, CreateScenarioConfigRegistry({::NAlice::ELang::L_RUS}));

        requestFactory.SetupModifierRequest(
            CreateRequest(IEvent::CreateEvent(ctx.SpeechKitRequest().Event()), ctx.SpeechKitRequest()),
            CreateScenarioResponseBody(),
            TEST_WINNER_SCENARIO_NAME);

        const auto modifierRequest = ApphostCtx().TestCtx().GetOnlyProtobufItem<TModifierRequest>(AH_ITEM_MODIFIER_REQUEST);
        UNIT_ASSERT_EQUAL(modifierRequest.GetBaseRequest().GetUserLanguage(), ELang::L_ARA);
        UNIT_ASSERT_EQUAL(modifierRequest.GetFeatures().GetScenarioLanguage(), ELang::L_RUS);
    }
    Y_UNIT_TEST(TestSetupPolyglotRequestModifierRuAr) {
        auto& ctx = CreateCtx(ELanguage::LANG_ARA);

        auto requestFactory = TAppHostModifierRequestFactory(
            ApphostCtx().ItemProxyAdapter(), ctx, CreateScenarioConfigRegistry({::NAlice::ELang::L_RUS, ::NAlice::ELang::L_ARA}));

        requestFactory.SetupModifierRequest(
            CreateRequest(IEvent::CreateEvent(ctx.SpeechKitRequest().Event()), ctx.SpeechKitRequest()),
            CreateScenarioResponseBody(),
            TEST_WINNER_SCENARIO_NAME);

        const auto modifierRequest = ApphostCtx().TestCtx().GetOnlyProtobufItem<TModifierRequest>(AH_ITEM_MODIFIER_REQUEST);
        UNIT_ASSERT_EQUAL(modifierRequest.GetBaseRequest().GetUserLanguage(), ELang::L_ARA);
        UNIT_ASSERT_EQUAL(modifierRequest.GetFeatures().GetScenarioLanguage(), ELang::L_ARA);
    }
}

Y_UNIT_TEST_SUITE_F(ModifierRequestFactoryApplyResponse, TFixture) {
    Y_UNIT_TEST(TestApplyResponse) {
        auto requestFactory = TAppHostModifierRequestFactory(
            ApphostCtx().ItemProxyAdapter(),
            CreateCtx(ELanguage::LANG_RUS),
            CreateScenarioConfigRegistry({::NAlice::ELang::L_RUS, ::NAlice::ELang::L_ARA}));

        {
            auto response = CreateScenarioResponse();
            TMegamindAnalyticsInfoBuilder analyticsInfo;
            const auto error = requestFactory.ApplyModifierResponse(response, analyticsInfo);
            UNIT_ASSERT(!error);
            UNIT_ASSERT(response.ResponseBodyIfExists());
            UNIT_ASSERT_EQUAL(response.ResponseBodyIfExists()->GetLayout().GetOutputSpeech(), "kek");
        }

        ApphostCtx().TestCtx().AddProtobufItem(CreateModifierResponse(), AH_ITEM_MODIFIER_RESPONSE, NAppHost::EContextItemKind::Input);

        {
            auto response = CreateScenarioResponse();
            TMegamindAnalyticsInfoBuilder analyticsInfo;
            const auto error = requestFactory.ApplyModifierResponse(response, analyticsInfo);
            UNIT_ASSERT(!error);
            UNIT_ASSERT(response.ResponseBodyIfExists());
            UNIT_ASSERT_EQUAL(response.ResponseBodyIfExists()->GetLayout().GetOutputSpeech(), "lol");
        }
    }
    Y_UNIT_TEST(TestApplyPolyglotResponse) {
        auto requestFactory = TAppHostModifierRequestFactory(
                ApphostCtx().ItemProxyAdapter(),
                CreateCtx(ELanguage::LANG_ARA),
                CreateScenarioConfigRegistry({::NAlice::ELang::L_RUS, ::NAlice::ELang::L_ARA}));

        {
            auto response = CreateScenarioResponse();
            TMegamindAnalyticsInfoBuilder analyticsInfo;
            const auto error = requestFactory.ApplyModifierResponse(response, analyticsInfo);
            UNIT_ASSERT(error);
        }

        ApphostCtx().TestCtx().AddProtobufItem(CreateModifierResponse(), AH_ITEM_MODIFIER_RESPONSE, NAppHost::EContextItemKind::Input);

        {
            auto response = CreateScenarioResponse();
            TMegamindAnalyticsInfoBuilder analyticsInfo;
            const auto error = requestFactory.ApplyModifierResponse(response, analyticsInfo);
            UNIT_ASSERT(!error);
            UNIT_ASSERT(response.ResponseBodyIfExists());
            UNIT_ASSERT_EQUAL(response.ResponseBodyIfExists()->GetLayout().GetOutputSpeech(), "lol");
        }
    }
}

} // namespace
