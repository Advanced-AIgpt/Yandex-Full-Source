#include "pre.h"

#include <alice/megamind/library/classifiers/formulas/formulas_description.h>
#include <alice/megamind/library/classifiers/formulas/formulas_storage.h>

#include <alice/megamind/library/common/defs.h>
#include <alice/megamind/library/config/protos/config.pb.h>
#include <alice/megamind/library/context/responses.h>
#include <alice/megamind/library/experiments/flags.h>
#include <alice/megamind/library/request/builder.h>
#include <alice/megamind/library/request/event/text_input_event.h>
#include <alice/megamind/library/response/response.h>
#include <alice/megamind/library/scenarios/defs/names.h>
#include <alice/megamind/library/scenarios/helpers/interface/scenario_ref.h>
#include <alice/megamind/library/scenarios/protocol/protocol_scenario.h>
#include <alice/megamind/library/scenarios/registry/registry.h>
#include <alice/megamind/library/testing/mock_context.h>
#include <alice/megamind/library/testing/mock_responses.h>
#include <alice/megamind/library/testing/speechkit.h>
#include <alice/megamind/library/testing/speechkit_api_builder.h>
#include <alice/megamind/library/testing/utils.h>
#include <alice/megamind/protos/common/device_state.pb.h>
#include <alice/protos/data/language/language.pb.h>

#include <alice/library/experiments/flags.h>
#include <alice/library/frame/builder.h>
#include <alice/library/json/json.h>
#include <alice/library/metrics/util.h>

#include <kernel/factor_storage/factor_storage.h>
#include <kernel/formula_storage/formula_storage.h>
#include <kernel/geodb/countries.h>

#include <library/cpp/testing/gmock_in_unittest/gmock.h>
#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/testing/unittest/tests_data.h>

#include <util/generic/algorithm.h>
#include <util/generic/ptr.h>
#include <util/generic/set.h>

using namespace ::testing;
using namespace NAlice;
using TLocalExpFlags = NMegamind::TClientComponent::TExpFlags;

namespace {

const TClientInfo EMPTY_CLIENT_INFO{TClientInfoProto{}};

TScenarioConfig ProtocolVinsScenarioConfig() {
    TScenarioConfig config;
    config.SetName(TString{MM_PROTO_VINS_SCENARIO});
    config.SetEnabled(true);
    config.AddLanguages(ELang(ELanguage::LANG_RUS));
    config.AddLanguages(ELang(ELanguage::LANG_TUR));
    config.SetAcceptsAnyUtterance(true);
    config.SetAcceptsImageInput(true);
    config.SetAcceptsMusicInput(true);
    return config;
}

constexpr TStringBuf DEFAULT_FRAME_NAME = "another_intent";
constexpr TStringBuf FAST_FRAME_NAME = "fast_frame";
constexpr TStringBuf ACTION_FRAME_NAME = "action_frame";

TScenarioConfig ProtocolTestScenarioConfig(TStringBuf scenarioName, const TVector<TStringBuf>& frameNames = {DEFAULT_FRAME_NAME}) {
    TScenarioConfig config;
    config.SetName(TString{scenarioName});
    for (const TStringBuf frameName : frameNames) {
        *config.AddAcceptedFrames() = frameName;
    }
    config.SetEnabled(true);
    config.AddLanguages(ELang(ELanguage::LANG_RUS));
    return config;
}

void RegisterPolyglotProtocolScenario(TScenarioRegistry& registry, TStringBuf scenarioName, const TVector<ELang>& langs) {
    auto config = ProtocolTestScenarioConfig(scenarioName);
    *config.MutableLanguages() = {langs.cbegin(), langs.cend()};
    registry.RegisterConfigBasedAppHostPureProtocolScenario(MakeHolder<TConfigBasedAppHostPureProtocolScenario>(config));
}

TWizardResponse ParseBegemotResponseJsonToWizardResponse(const TStringBuf begemotResponseJsonStr) {
    const auto begemotResponseJson = JsonFromString(begemotResponseJsonStr);
    auto begemotResponse = JsonToProto<NBg::NProto::TAlicePolyglotMergeResponseResult>(begemotResponseJson);
    return TWizardResponse(std::move(begemotResponse));
}

TWizardResponse PrepareMusicWizardResponse() {
    return ParseBegemotResponseJsonToWizardResponse(R"({
        "AliceResponse": {
            "CustomEntities": {
                "Occurrences": {
                "Tokens": ["включи","мою","музыку"],
                "Ranges": [{"Begin":0,"End":1},{"Begin":1,"End":2}]
                },
                "Values": [
                {"CustomEntityValues":[{"Type":"action_request","Value":"autoplay"}]},
                {"CustomEntityValues":[{"Type":"personality","Value":"is_personal"}]},
                ]
            },
            "AliceTagger": {
                "Predictions": {
                    "personal_assistant.scenarios.music_play": {
                        "Prediction": [
                            {
                                "Token":[
                                    {"Text":"включи", "Tag":"B-action_request"},
                                    {"Text":"мою", "Tag":"B-personality"},
                                    {"Text":"музыку", "Tag":"O"},
                                ],
                                "Probability":0.95
                            }
                        ]
                    }
                }
            },
            "AliceParsedFrames": {
                "Frames": [
                {
                    "Name": "personal_assistant.scenarios.music_play",
                    "Slots": [
                    {
                        "Name": "action_request",
                        "Type": "string",
                        "Value": "включи",
                        "AcceptedTypes": [
                        "string"
                        ],
                        "IsFilled": true
                    },
                    {
                        "Name": "search_text",
                        "Type": "string",
                        "Value": "радио европа плюс",
                        "AcceptedTypes": [
                        "string"
                        ],
                        "IsFilled": true
                    }
                    ]
                }
                ],
                "Confidences": [
                0.5
                ],
                "Sources": [
                "AliceTagger",
                ]
            }
        }
    })");
}

TWizardResponse PrepareNewsWizardResponse(TString request, float binaryClassifierProba) {
    return ParseBegemotResponseJsonToWizardResponse(R"({
        "AliceResponse": {
            "Text":{
                "Request":")" + request + R"(",
                "UserRequest":")" + request + R"(",
                "RequestLenTruncated":"0"
            },
            "AliceParsedFrames": {
                "Frames": [
                    {
                        "Name": "personal_assistant.scenarios.get_news"
                    }
                ],
                "Confidences": [
                    )" + ToString(binaryClassifierProba) + R"(
                ],
                "Sources": [
                    "Granet"
                ]
            }
        }
    })");
}

TVector<TSemanticFrame> PrepareNewsFrames(bool withPreciseFrame) {
    TVector<TSemanticFrame> vector;
    if (withPreciseFrame) {
        TSemanticFrame frame;
        frame.SetName(TString(MM_NEWS_FRAME_NAME));
        vector.push_back(frame);
    }
    TSemanticFrame freeFrame;
    freeFrame.SetName(TString(MM_NEWS_FREE_FRAME_NAME));
    vector.push_back(freeFrame);
    return vector;
}

constexpr TStringBuf TEST_SCENARIO_NAME = "TestScenario";

struct TFixture : public NUnitTest::TBaseFixture {
    TFixture()
        : SpeechKitRequest(TSpeechKitRequestBuilder(TSpeechKitApiRequestBuilder().SetTextInput("hello").BuildJson())
                .Build())
        , Request(CreateRequest(IEvent::CreateEvent(SpeechKitRequest.Event()), SpeechKitRequest))
        , FormulasStorage{RawFormulasStorage, FormulasDescription}
    {
        Registry.RegisterConfigBasedAppHostPureProtocolScenario(
            MakeHolder<TConfigBasedAppHostPureProtocolScenario>(ProtocolVinsScenarioConfig()));

        Session = MakeSessionBuilder()
            ->SetPreviousScenarioName("")
            .SetScenarioSession("", NewScenarioSession(TState{}))
            .Build();
    }

    void AddProtocolScenario(TStringBuf scenarioName, const TVector<TStringBuf>& frameNames = {DEFAULT_FRAME_NAME}) {
        Registry.RegisterConfigBasedAppHostPureProtocolScenario(
            MakeHolder<TConfigBasedAppHostPureProtocolScenario>(ProtocolTestScenarioConfig(scenarioName, frameNames)));
    }

    void InitDefaultExpects() const {
        EXPECT_CALL(Ctx, Language()).WillRepeatedly(Return(ELanguage::LANG_RUS));
        EXPECT_CALL(Ctx, HasExpFlag(_)).WillRepeatedly(Return(false));

        static THashMap<TString, TMaybe<TString>> expFlags;
        EXPECT_CALL(Ctx, ExpFlags()).WillRepeatedly(ReturnRef(expFlags));

        static TMockResponses responses;
        auto wizardResponse = ParseBegemotResponseJsonToWizardResponse(R"({
            "AliceResponse": {
                "AliceFixlist": {
                    "Matches": {
                        "general_fixlist": {}
                    }
                }
            }
        })");
        responses.SetWizardResponse(std::move(wizardResponse));
        EXPECT_CALL(Ctx, Responses()).WillRepeatedly(ReturnRef(responses));

        static auto speechKitRequest = TSpeechKitRequestBuilder(TSpeechKitRequestBuilder::EPredefined::MinimalWithTextEvent).Build();
        EXPECT_CALL(Ctx, SpeechKitRequest()).WillRepeatedly(Return(speechKitRequest));

        static TClientInfo clientInfo{TClientInfoProto{}};
        EXPECT_CALL(Ctx, ClientInfo()).WillRepeatedly(ReturnRef(clientInfo));

        static TClientFeatures clientFeatures{TClientInfoProto{}, {}};
        EXPECT_CALL(Ctx, ClientFeatures()).WillRepeatedly(ReturnRef(clientFeatures));

        EXPECT_CALL(Ctx, Logger()).WillRepeatedly(ReturnRef(TRTLogger::NullLogger()));

        EXPECT_CALL(Ctx, Session()).WillRepeatedly(Return(Session.Get()));

        static NAlice::TConfig::TScenarios::TConfig scenarioConfig;
        EXPECT_CALL(Ctx, ScenarioConfig(_)).WillRepeatedly(ReturnRef(scenarioConfig));

        static NMegamind::TClassificationConfig classificationConfig;
        EXPECT_CALL(Ctx, ClassificationConfig()).WillRepeatedly(ReturnRef(classificationConfig));

        static TMockSensors sensors;
        EXPECT_CALL(Ctx, Sensors()).WillRepeatedly(ReturnRef(sensors));

        EXPECT_CALL(Ctx, IsProtoVinsEnabled()).WillRepeatedly(Return(true));
    }

    NiceMock<TMockContext> Ctx;
    TTestSpeechKitRequest SpeechKitRequest;
    TRequest Request;
    TFactorStorage FactorStorage;
    ::TFormulasStorage RawFormulasStorage;
    TFormulasDescription FormulasDescription;
    NAlice::TFormulasStorage FormulasStorage;
    TQualityStorage QualityStorage;
    TScenarioRegistry Registry;
    THolder<ISession> Session;
};

class TTestScenario : public TScenario {
public:
    explicit TTestScenario(TStringBuf name)
        : TScenario(name)
    {
    }

    TVector<TString> GetAcceptedFrames() const override {
        return {};
    }
};

class TTestProtocolScenario : public TConfigBasedAppHostPureProtocolScenario {
public:
    TTestProtocolScenario(const TString& name)
        : TConfigBasedAppHostPureProtocolScenario(ProtocolTestScenarioConfig(name))
    {
    }

    MOCK_METHOD(bool, IsEnabled, (const IContext& ctx), (const, override));
};

TScenarioToRequestFrames MakeMapping(
    const TScenarioRefs& refs,
    const TSet<TStringBuf>& expectedScenarios = {}
) {
    TScenarioToRequestFrames result;
    for (const auto& ref : refs) {
        if (expectedScenarios.empty() || expectedScenarios.contains(ref->GetScenario().GetName())) {
            if (ref->GetScenario().GetAcceptedFrames().empty()) {
                result[ref] = {TSemanticFrameBuilder{"some_intent"}.Build()};
            } else {
                for (const auto& frame : ref->GetScenario().GetAcceptedFrames()) {
                    result[ref].push_back(TSemanticFrameBuilder{frame}.Build());
                }
            }
        } else {
            result[ref] = {};
        }
    }
    return result;
}

void CheckOnlyAllowed(
    const TSet<TStringBuf>& expectedScenarioNames,
    const TScenarioToRequestFrames& gotScenarioRefToFrames,
    const TStringBuf message = TStringBuf("")
) {
    TSet<TStringBuf> gotScenarioNames;
    for (const auto& [ ref, frames ] : gotScenarioRefToFrames) {
        UNIT_ASSERT(ref);
        gotScenarioNames.insert(ref->GetScenario().GetName());
    }
    UNIT_ASSERT_VALUES_EQUAL_C(expectedScenarioNames, gotScenarioNames, message);
}

TString GetThresholdsFlag(const THashMap<TStringBuf, double>& scenarioToThreshold) {
    TStringBuilder result;
    result << EXP_PREFIX_MM_PRECLASSIFIER_THRESHOLDS;
    bool isFirst = true;
    for (const auto& [scenario, threshold] : scenarioToThreshold) {
        if (isFirst) {
            isFirst = false;
        } else {
            result << ';';
        }
        result << scenario << ':' << threshold;
    }
    return result;
}

void CheckWithExpThresholds(const THashMap<TStringBuf, double>& scenarioToThreshold,
                            const TSet<TStringBuf>& expectedScenarios, TFixture& fixture,
                            TMaybe<TWizardResponse> wizardResponse = {},
                            const TString& confidentScenarioThresholdFlag = {})
{
    fixture.InitDefaultExpects();
    fixture.AddProtocolScenario(HOLLYWOOD_COMMANDS_SCENARIO);

    const THashMap<TString, TMaybe<TString>> expFlags = {
        {GetThresholdsFlag(scenarioToThreshold), "1"},
        {confidentScenarioThresholdFlag, "1"},
    };
    EXPECT_CALL(fixture.Ctx, ExpFlags()).WillRepeatedly(ReturnRef(expFlags));
    if (wizardResponse.Defined()) {
        static TMockResponses responses;
        responses.SetWizardResponse(std::move(wizardResponse.GetRef()));
        EXPECT_CALL(fixture.Ctx, Responses()).WillRepeatedly(ReturnRef(responses));
    }

    auto candidateToRequestFrames = MakeMapping(fixture.Registry.GetScenarioRefs());
    PreClassify(candidateToRequestFrames, fixture.Request, fixture.Ctx, fixture.FormulasStorage, fixture.FactorStorage,
                fixture.QualityStorage);
    CheckOnlyAllowed(expectedScenarios, candidateToRequestFrames);
}

void CheckHollywoodWithThreshold(TFixture& fixture, const bool hasExp) {
    fixture.AddProtocolScenario(HOLLYWOOD_MUSIC_SCENARIO);

    TMockResponses responses;
    responses.SetWizardResponse(PrepareMusicWizardResponse());
    EXPECT_CALL(fixture.Ctx, Responses()).WillRepeatedly(ReturnRef(responses));

    THashMap<TString, TMaybe<TString>> expFlags;
    EXPECT_CALL(fixture.Ctx, ExpFlags()).WillRepeatedly(ReturnRef(expFlags));

    TClientInfoProto clientInfoProto;
    clientInfoProto.SetAppId("something_else");
    TClientFeatures clientFeatures{clientInfoProto, expFlags};
    EXPECT_CALL(fixture.Ctx, ClientFeatures()).WillRepeatedly(ReturnRef(clientFeatures));

    TSet<TStringBuf> expectedScenarios = {
        MM_PROTO_VINS_SCENARIO,
    };

    if (hasExp) {
        expFlags[TString{EXP_MUSIC_PLAY_CONFIDENCE_THRESHOLD_PREFIX} + "0.0"] = "1";
        expectedScenarios.insert(HOLLYWOOD_MUSIC_SCENARIO);
    }
    auto candidateToRequestFrames = MakeMapping(fixture.Registry.GetScenarioRefs(), expectedScenarios);
    PreClassify(candidateToRequestFrames, fixture.Request, fixture.Ctx, fixture.FormulasStorage, fixture.FactorStorage,
                fixture.QualityStorage);
    CheckOnlyAllowed(expectedScenarios, candidateToRequestFrames);
}

void CheckMediaInput(TFixture& fixture, const EEventType eventType) {
    fixture.AddProtocolScenario("CustomScenario");

    const auto speechKitRequest =
        TSpeechKitRequestBuilder{TSpeechKitRequestBuilder::EPredefined::MinimalWithTextEvent}
            .SetProtoPatcher([&](NMegamind::TSpeechKitInitContext& ctx) { ctx.EventProtoPtr->SetType(eventType); })
            .Build();
    EXPECT_CALL(fixture.Ctx, SpeechKitRequest()).WillRepeatedly(Return(speechKitRequest));
    const auto request = CreateRequest(IEvent::CreateEvent(speechKitRequest.Event()), speechKitRequest);
    const auto process = [&] {
        auto candidateToRequestFrames = MakeMapping(fixture.Registry.GetScenarioRefs());
        PreClassify(candidateToRequestFrames, request,
                    fixture.Ctx, fixture.FormulasStorage, fixture.FactorStorage,fixture.QualityStorage);
        return candidateToRequestFrames;
    };
    CheckOnlyAllowed({MM_PROTO_VINS_SCENARIO}, process());

    fixture.Registry.RegisterConfigBasedAppHostPureProtocolScenario(MakeHolder<TConfigBasedAppHostPureProtocolScenario>(
        [eventType] {
            auto config = ProtocolTestScenarioConfig(TEST_SCENARIO_NAME);
            switch (eventType) {
                case EEventType::music_input:
                    config.SetAcceptsMusicInput(true);
                    break;
                case EEventType::image_input:
                    config.SetAcceptsImageInput(true);
                    break;
                default:
                    break;
            }
            return config;
        }()
    ));

    CheckOnlyAllowed({TEST_SCENARIO_NAME, MM_PROTO_VINS_SCENARIO}, process());

    auto scenarioSession = NewScenarioSession(TState{});
    scenarioSession.SetActivityTurn(1);
    const auto session = MakeSessionBuilder()
        ->SetPreviousScenarioName("CustomScenario")
        .SetScenarioSession("CustomScenario", scenarioSession)
        .Build();
    EXPECT_CALL(fixture.Ctx, Session()).WillRepeatedly(Return(session.Get()));
    CheckOnlyAllowed({TEST_SCENARIO_NAME, MM_PROTO_VINS_SCENARIO, "CustomScenario"}, process());
}

void CheckPreclassifyKeepsFixlist(TFixture& fixture, const TWizardResponse& wizardResponseNoFl, const TWizardResponse& wizardResponseFl){
    fixture.AddProtocolScenario("TestScenario1");
    fixture.AddProtocolScenario("TestScenario2");

    NMegamind::TClassificationConfig::TScenarioConfig scenarioConfig;
    scenarioConfig.AddPreclassifierHint(TString{FAST_FRAME_NAME});

    NMegamind::TClassificationConfig configWithFastFrame;
    auto& mp = *configWithFastFrame.MutableScenarioClassificationConfigs();
    mp["TestScenario1"] = scenarioConfig;

    EXPECT_CALL(fixture.Ctx, ClassificationConfig()).WillRepeatedly(ReturnRef(configWithFastFrame));

    const TMockResponses responses_nofl;
    EXPECT_CALL(responses_nofl, WizardResponse(_)).WillRepeatedly(ReturnRef(wizardResponseNoFl));
    EXPECT_CALL(fixture.Ctx, Responses()).WillRepeatedly(ReturnRef(responses_nofl));
    {
        auto candidateToRequestFrames = MakeMapping(fixture.Registry.GetScenarioRefs());
        PreClassify(
            candidateToRequestFrames,
            fixture.Request,
            fixture.Ctx,
            fixture.FormulasStorage,
            fixture.FactorStorage,
            fixture.QualityStorage
        );
        CheckOnlyAllowed({"TestScenario1"}, candidateToRequestFrames);
    }
    const TMockResponses responses_fl;
    EXPECT_CALL(responses_fl, WizardResponse(_)).WillRepeatedly(ReturnRef(wizardResponseFl));
    EXPECT_CALL(fixture.Ctx, Responses()).WillRepeatedly(ReturnRef(responses_fl));

    {
        auto candidateToRequestFrames = MakeMapping(fixture.Registry.GetScenarioRefs());
        PreClassify(
            candidateToRequestFrames,
            fixture.Request,
            fixture.Ctx,
            fixture.FormulasStorage,
            fixture.FactorStorage,
            fixture.QualityStorage
        );
        CheckOnlyAllowed({"TestScenario1", "TestScenario2"}, candidateToRequestFrames);
    }

}

TClientFeatures GetClientFeaturesFor(TStringBuf appId) {
    TClientInfoProto clientInfoProto;
    clientInfoProto.SetAppId(appId.data());
    return TClientFeatures {clientInfoProto, {}};
}

void TestKeepsVideoOnHints(TFixture& fixture, const TWizardResponse& wizardResponseEmpty, const TWizardResponse& wizardResponseNoSelected,
                           const TWizardResponse& wizardResponseItemSelector) {
    fixture.AddProtocolScenario("TestScenario1");
    fixture.AddProtocolScenario("Video");

    NMegamind::TClassificationConfig::TScenarioConfig scenarioConfig;
    scenarioConfig.AddPreclassifierHint(TString{FAST_FRAME_NAME});

    NMegamind::TClassificationConfig configWithFastFrame;
    auto& mp = *configWithFastFrame.MutableScenarioClassificationConfigs();
    mp["TestScenario1"] = scenarioConfig;

    EXPECT_CALL(fixture.Ctx, ClassificationConfig()).WillRepeatedly(ReturnRef(configWithFastFrame));

    const auto clientFeatures = GetClientFeaturesFor("com.yandex.tv.alice");
    EXPECT_CALL(fixture.Ctx, ClientFeatures()).WillRepeatedly(ReturnRef(clientFeatures));

    const TMockResponses responses;
    EXPECT_CALL(responses, WizardResponse(_)).WillRepeatedly(ReturnRef(wizardResponseEmpty));
    EXPECT_CALL(fixture.Ctx, Responses()).WillRepeatedly(ReturnRef(responses));
    {
        auto candidateToRequestFrames = MakeMapping(fixture.Registry.GetScenarioRefs());
        PreClassify(
            candidateToRequestFrames,
            fixture.Request,
            fixture.Ctx,
            fixture.FormulasStorage,
            fixture.FactorStorage,
            fixture.QualityStorage
        );
        CheckOnlyAllowed({"TestScenario1"}, candidateToRequestFrames);
    }

    const TMockResponses responsesNS;
    EXPECT_CALL(responsesNS, WizardResponse(_)).WillRepeatedly(ReturnRef(wizardResponseNoSelected));
    EXPECT_CALL(fixture.Ctx, Responses()).WillRepeatedly(ReturnRef(responsesNS));
    {
        auto candidateToRequestFrames = MakeMapping(fixture.Registry.GetScenarioRefs());
        PreClassify(
            candidateToRequestFrames,
            fixture.Request,
            fixture.Ctx,
            fixture.FormulasStorage,
            fixture.FactorStorage,
            fixture.QualityStorage
        );
        CheckOnlyAllowed({"TestScenario1"}, candidateToRequestFrames);
    }

    const TMockResponses responsesIS;
    EXPECT_CALL(responsesIS, WizardResponse(_)).WillRepeatedly(ReturnRef(wizardResponseItemSelector));
    EXPECT_CALL(fixture.Ctx, Responses()).WillRepeatedly(ReturnRef(responsesIS));
    {
        auto candidateToRequestFrames = MakeMapping(fixture.Registry.GetScenarioRefs());
        PreClassify(
            candidateToRequestFrames,
            fixture.Request,
            fixture.Ctx,
            fixture.FormulasStorage,
            fixture.FactorStorage,
            fixture.QualityStorage
        );
        CheckOnlyAllowed({"TestScenario1", "Video"}, candidateToRequestFrames);
    }
}

void TestBoostScenarioByPreclassifierHint(TFixture& fixture, const TWizardResponse& wizardResponse) {
    fixture.AddProtocolScenario("TestScenario1", {DEFAULT_FRAME_NAME, ACTION_FRAME_NAME});
    fixture.AddProtocolScenario("TestScenario2", {DEFAULT_FRAME_NAME, ACTION_FRAME_NAME});
    fixture.AddProtocolScenario("TestScenario3");

    NMegamind::TClassificationConfig::TScenarioConfig scenarioConfig;
    scenarioConfig.AddPreclassifierHint(TString{FAST_FRAME_NAME});

    NMegamind::TClassificationConfig configWithFastFrame;
    auto& mp = *configWithFastFrame.MutableScenarioClassificationConfigs();
    mp["TestScenario1"] = scenarioConfig;
    mp["TestScenario2"] = scenarioConfig;

    EXPECT_CALL(fixture.Ctx, ClassificationConfig()).WillRepeatedly(ReturnRef(configWithFastFrame));

    const TMockResponses responses;
    EXPECT_CALL(responses, WizardResponse(_)).WillRepeatedly(ReturnRef(wizardResponse));
    EXPECT_CALL(fixture.Ctx, Responses()).WillRepeatedly(ReturnRef(responses));

    {
        auto candidateToRequestFrames = MakeMapping(fixture.Registry.GetScenarioRefs());
        PreClassify(
            candidateToRequestFrames,
            fixture.Request,
            fixture.Ctx,
            fixture.FormulasStorage,
            fixture.FactorStorage,
            fixture.QualityStorage
        );
        CheckOnlyAllowed({"TestScenario1", "TestScenario2"}, candidateToRequestFrames);
    }

    {
        auto candidateToRequestFrames = MakeMapping(fixture.Registry.GetScenarioRefs());
        const auto sessionExpectsRequest = MakeSessionBuilder()
            ->SetPreviousScenarioName("")
            .SetScenarioSession("", NewScenarioSession(TState{}))
            .SetRequestIsExpected(true)
            .Build();
        EXPECT_CALL(fixture.Ctx, Session()).WillRepeatedly(Return(sessionExpectsRequest.Get()));
        PreClassify(
            candidateToRequestFrames,
            fixture.Request,
            fixture.Ctx,
            fixture.FormulasStorage,
            fixture.FactorStorage,
            fixture.QualityStorage
        );
        CheckOnlyAllowed(
            {"TestScenario1", "TestScenario2", "TestScenario3", MM_PROTO_VINS_SCENARIO},
            candidateToRequestFrames
        );
    }

    {
        fixture.AddProtocolScenario(TEST_SCENARIO_NAME);
        auto candidateToRequestFrames = MakeMapping(fixture.Registry.GetScenarioRefs());
        auto scenarioSession = NewScenarioSession(TState{});
        scenarioSession.SetActivityTurn(1);
        const auto session = MakeSessionBuilder()
            ->SetPreviousScenarioName(TString{TEST_SCENARIO_NAME})
            .SetScenarioSession(TString{TEST_SCENARIO_NAME}, scenarioSession)
            .Build();
        EXPECT_CALL(fixture.Ctx, Session()).WillRepeatedly(Return(session.Get()));
        PreClassify(
            candidateToRequestFrames,
            fixture.Request,
            fixture.Ctx,
            fixture.FormulasStorage,
            fixture.FactorStorage,
            fixture.QualityStorage
        );
        CheckOnlyAllowed(
            {"TestScenario1", "TestScenario2", "TestScenario3", MM_PROTO_VINS_SCENARIO, TEST_SCENARIO_NAME},
            candidateToRequestFrames
        );
    }
}

TWizardResponse CreateWhitelistWizardResponse() {
    return ParseBegemotResponseJsonToWizardResponse(R"({
        "AliceResponse": {
            "AliceParsedFrames": {
                "Frames": [
                    {
                        "Name": "whitelist1"
                    }
                ],
                "Sources": [
                    "Granet"
                ],
                "Confidences": [
                    1.0
                ]
            }
        }
    })");
}

Y_UNIT_TEST_SUITE_F(PreClassify, TFixture) {
    Y_UNIT_TEST(ForceVins) {
        InitDefaultExpects();

        static THashMap<TString, TMaybe<TString>> expFlags = {
            {TString{EXP_PREFIX_MM_FORCE_SCENARIO} + MM_PROTO_VINS_SCENARIO, ""}};
        EXPECT_CALL(Ctx, ExpFlags()).WillRepeatedly(ReturnRef(expFlags));

        auto candidateToRequestFrames = MakeMapping(Registry.GetScenarioRefs());
        PreClassify(candidateToRequestFrames, Request, Ctx, FormulasStorage, FactorStorage, QualityStorage);
        CheckOnlyAllowed({MM_PROTO_VINS_SCENARIO}, candidateToRequestFrames);
    }

    Y_UNIT_TEST(GeneralCase) {
        InitDefaultExpects();

        const TSet<TStringBuf> expectedScenarios = {
            MM_PROTO_VINS_SCENARIO,
        };

        auto candidateToRequestFrames = MakeMapping(Registry.GetScenarioRefs());
        PreClassify(candidateToRequestFrames, Request, Ctx, FormulasStorage, FactorStorage, QualityStorage);
        CheckOnlyAllowed(expectedScenarios, candidateToRequestFrames);
    }

    Y_UNIT_TEST(LanguageFilter) {
        InitDefaultExpects();
        EXPECT_CALL(Ctx, Language()).WillRepeatedly(Return(ELanguage::LANG_ARA));

        RegisterPolyglotProtocolScenario(Registry, "RuOnly", {::NAlice::ELang::L_RUS});
        RegisterPolyglotProtocolScenario(Registry, "ArOnly", {::NAlice::ELang::L_ARA});
        RegisterPolyglotProtocolScenario(Registry, "TrOnly", {::NAlice::ELang::L_TUR});
        RegisterPolyglotProtocolScenario(Registry, "RuAr", {::NAlice::ELang::L_RUS, ::NAlice::ELang::L_ARA});

        const TSet<TStringBuf> expectedScenarios = {
            MM_PROTO_VINS_SCENARIO, "RuOnly", "ArOnly", "RuAr"
        };

        auto candidateToRequestFrames = MakeMapping(Registry.GetScenarioRefs());
        PreClassify(candidateToRequestFrames, Request, Ctx, FormulasStorage, FactorStorage, QualityStorage);
        CheckOnlyAllowed(expectedScenarios, candidateToRequestFrames);
    }

    Y_UNIT_TEST(LanguageFilterExpForceAllScenarioLanguages) {
        InitDefaultExpects();
        EXPECT_CALL(Ctx, Language()).WillRepeatedly(Return(ELanguage::LANG_ARA));
        EXPECT_CALL(Ctx, HasExpFlag(EXP_FORCE_TRANSLATED_LANGUAGE_FOR_ALL_SCENARIOS)).WillRepeatedly(Return(true));
        EXPECT_CALL(Ctx, HasExpFlag("mm_force_polyglot_language_for_scenario=ArOnlyWithFlag")).WillRepeatedly(Return(true));

        RegisterPolyglotProtocolScenario(Registry, "RuOnly", {::NAlice::ELang::L_RUS});
        RegisterPolyglotProtocolScenario(Registry, "ArOnly", {::NAlice::ELang::L_ARA});
        RegisterPolyglotProtocolScenario(Registry, "ArOnlyWithFlag", {::NAlice::ELang::L_ARA});
        RegisterPolyglotProtocolScenario(Registry, "TrOnly", {::NAlice::ELang::L_TUR});
        RegisterPolyglotProtocolScenario(Registry, "RuAr", {::NAlice::ELang::L_RUS, ::NAlice::ELang::L_ARA});

        const TSet<TStringBuf> expectedScenarios = {
            MM_PROTO_VINS_SCENARIO, "RuOnly", "ArOnlyWithFlag", "RuAr"
        };

        auto candidateToRequestFrames = MakeMapping(Registry.GetScenarioRefs());
        PreClassify(candidateToRequestFrames, Request, Ctx, FormulasStorage, FactorStorage, QualityStorage);
        CheckOnlyAllowed(expectedScenarios, candidateToRequestFrames);
    }

    Y_UNIT_TEST(ScenarioAcceptsNamedFrameAndHasIt) {
        InitDefaultExpects();
        AddProtocolScenario(HOLLYWOOD_COMMANDS_SCENARIO);

        const TSet<TStringBuf> expectedScenarios = {
            MM_PROTO_VINS_SCENARIO,
            HOLLYWOOD_COMMANDS_SCENARIO,
        };

        auto candidateToRequestFrames = MakeMapping(Registry.GetScenarioRefs());
        PreClassify(candidateToRequestFrames, Request, Ctx, FormulasStorage, FactorStorage, QualityStorage);
        CheckOnlyAllowed(expectedScenarios, candidateToRequestFrames);
    }

    Y_UNIT_TEST(ScenarioAcceptsNamedFrameAndDoesNotHaveIt) {
        InitDefaultExpects();
        AddProtocolScenario(HOLLYWOOD_COMMANDS_SCENARIO);

        const TSet<TStringBuf> expectedScenarios = {
            MM_PROTO_VINS_SCENARIO,
        };

        auto candidateToRequestFrames = MakeMapping(Registry.GetScenarioRefs(), expectedScenarios);
        PreClassify(candidateToRequestFrames, Request, Ctx, FormulasStorage, FactorStorage, QualityStorage);
        CheckOnlyAllowed(expectedScenarios, candidateToRequestFrames);
    }

    Y_UNIT_TEST(ScenarioAcceptsAnyFramesAndHasNamedOne) {
        InitDefaultExpects();

        const TSet<TStringBuf> expectedScenarios = {
            MM_PROTO_VINS_SCENARIO,
        };

        auto candidateToRequestFrames = MakeMapping(Registry.GetScenarioRefs());
        PreClassify(candidateToRequestFrames, Request, Ctx, FormulasStorage, FactorStorage, QualityStorage);
        CheckOnlyAllowed(expectedScenarios, candidateToRequestFrames);
    }

    Y_UNIT_TEST(ScenarioAcceptsAnyFramesAndDoesNotHaveNamedOne) {
        InitDefaultExpects();

        const TSet<TStringBuf> expectedScenarios = {
            MM_PROTO_VINS_SCENARIO,
        };

        auto candidateToRequestFrames = MakeMapping(Registry.GetScenarioRefs(), {MM_PROTO_VINS_SCENARIO});
        PreClassify(candidateToRequestFrames, Request, Ctx, FormulasStorage, FactorStorage, QualityStorage);
        CheckOnlyAllowed(expectedScenarios, candidateToRequestFrames);
    }

    Y_UNIT_TEST(FixlistTur) {
        InitDefaultExpects();

        static TMockResponses responses;
        auto wizardResponse = ParseBegemotResponseJsonToWizardResponse(R"({
            "AliceResponse": {
                "AliceFixlist": {
                    "Matches": {
                        "general_fixlist": {
                            "Intents" : [
                                "Vins"
                            ]
                        }
                    }
                }
            }
        })");
        responses.SetWizardResponse(std::move(wizardResponse));
        EXPECT_CALL(Ctx, Responses()).WillRepeatedly(ReturnRef(responses));

        EXPECT_CALL(Ctx, Language()).WillRepeatedly(Return(ELanguage::LANG_TUR));

        const TSet<TStringBuf> expectedScenarios = {
            MM_PROTO_VINS_SCENARIO,
        };

        auto candidateToRequestFrames = MakeMapping(Registry.GetScenarioRefs());
        PreClassify(candidateToRequestFrames, Request, Ctx, FormulasStorage, FactorStorage, QualityStorage);
        CheckOnlyAllowed(expectedScenarios, candidateToRequestFrames);
    }

    Y_UNIT_TEST(BoostedScenarioLeaveOne) {
        InitDefaultExpects();

        const TMaybe<TString> scenarioName = TString{MM_PROTO_VINS_SCENARIO};
        Request = std::move(NMegamind::TRequestBuilder(IEvent::CreateEvent(Request.GetEvent().SpeechKitEvent()))
            .SetScenario(TRequest::TScenarioInfo{*scenarioName, EScenarioNameSource::ServerAction}))
            .Build();

        auto candidateToRequestFrames = MakeMapping(Registry.GetScenarioRefs());
        PreClassify(candidateToRequestFrames, Request, Ctx, FormulasStorage, FactorStorage, QualityStorage);
        CheckOnlyAllowed({MM_PROTO_VINS_SCENARIO}, candidateToRequestFrames);
    }

    Y_UNIT_TEST(BoostedScenarioLeaveOneOnDisabledScenarios) {
        InitDefaultExpects();

        const TString scenarioName = "SN";
        {
            TScenarioConfig config;
            config.SetName(scenarioName);
            config.SetEnabled(false);
            config.MutableLanguages()->Add(ELang::L_RUS);
            Registry.RegisterConfigBasedAppHostPureProtocolScenario(
                MakeHolder<TConfigBasedAppHostPureProtocolScenario>(config));
        }

        auto candidateToRequestFrames = MakeMapping(Registry.GetScenarioRefs());
        PreClassify(candidateToRequestFrames, Request, Ctx, FormulasStorage, FactorStorage, QualityStorage);
        for (const auto& [candidate, _] : candidateToRequestFrames) {
            UNIT_ASSERT_VALUES_UNEQUAL(candidate->GetScenario().GetName(), scenarioName);
        }

        const TMaybe<TString> boostedScenario = scenarioName;
        Request = std::move(NMegamind::TRequestBuilder(IEvent::CreateEvent(Request.GetEvent().SpeechKitEvent()))
            .SetScenario(TRequest::TScenarioInfo{*boostedScenario, EScenarioNameSource::ServerAction}))
            .Build();

        candidateToRequestFrames = MakeMapping(Registry.GetScenarioRefs());
        PreClassify(candidateToRequestFrames, Request, Ctx, FormulasStorage, FactorStorage, QualityStorage);
        CheckOnlyAllowed({scenarioName}, candidateToRequestFrames);

        const TString disableFlag = EXP_PREFIX_MM_DISABLE_PROTOCOL_SCENARIO + scenarioName;
        EXPECT_CALL(Ctx, HasExpFlag(disableFlag)).WillRepeatedly(Return(true));
        candidateToRequestFrames = MakeMapping(Registry.GetScenarioRefs());
        PreClassify(candidateToRequestFrames, Request, Ctx, FormulasStorage, FactorStorage, QualityStorage);
        CheckOnlyAllowed({}, candidateToRequestFrames);
    }

    Y_UNIT_TEST(ForcedScenarioWithBoostedScenarioLeaveNothingOnDifferentScenarios) {
        InitDefaultExpects();
        AddProtocolScenario(HOLLYWOOD_COMMANDS_SCENARIO);

        THashMap<TString, TMaybe<TString>> expFlags = {
            {TString{EXP_PREFIX_MM_FORCE_SCENARIO} + HOLLYWOOD_COMMANDS_SCENARIO, ""}};
        EXPECT_CALL(Ctx, ExpFlags()).WillRepeatedly(ReturnRef(expFlags));

        Request = std::move(NMegamind::TRequestBuilder(IEvent::CreateEvent(Request.GetEvent().SpeechKitEvent()))
            .SetScenario(TRequest::TScenarioInfo{TString{MM_PROTO_VINS_SCENARIO}, EScenarioNameSource::ServerAction}))
            .Build();

        auto candidateToRequestFrames = MakeMapping(Registry.GetScenarioRefs());
        PreClassify(candidateToRequestFrames, Request, Ctx, FormulasStorage, FactorStorage, QualityStorage);
        CheckOnlyAllowed({}, candidateToRequestFrames);
    }

    Y_UNIT_TEST(ForcedScenarioWithBoostedScenarioLeaveOneOnSameScenario) {
        InitDefaultExpects();
        AddProtocolScenario(HOLLYWOOD_COMMANDS_SCENARIO);

        const TMaybe<TString> scenarioName = TString{HOLLYWOOD_COMMANDS_SCENARIO};
        THashMap<TString, TMaybe<TString>> expFlags = {
            {TString{EXP_PREFIX_MM_FORCE_SCENARIO} + scenarioName.GetRef(), ""}};
        EXPECT_CALL(Ctx, ExpFlags()).WillRepeatedly(ReturnRef(expFlags));

        Request = std::move(NMegamind::TRequestBuilder(IEvent::CreateEvent(Request.GetEvent().SpeechKitEvent()))
            .SetScenario(TRequest::TScenarioInfo{*scenarioName, EScenarioNameSource::ServerAction}))
            .Build();

        auto candidateToRequestFrames = MakeMapping(Registry.GetScenarioRefs());
        PreClassify(candidateToRequestFrames, Request, Ctx, FormulasStorage, FactorStorage, QualityStorage);
        CheckOnlyAllowed({HOLLYWOOD_COMMANDS_SCENARIO}, candidateToRequestFrames);
    }

    Y_UNIT_TEST(BoostInvalidScenarioLeaveNothing) {
        InitDefaultExpects();

        const TMaybe<TString> scenarioName = "!@#";
        Request = std::move(NMegamind::TRequestBuilder(IEvent::CreateEvent(Request.GetEvent().SpeechKitEvent()))
            .SetScenario(TRequest::TScenarioInfo{*scenarioName, EScenarioNameSource::ServerAction}))
            .Build();

        auto candidateToRequestFrames = MakeMapping(Registry.GetScenarioRefs());
        PreClassify(candidateToRequestFrames, Request, Ctx, FormulasStorage, FactorStorage, QualityStorage);
        CheckOnlyAllowed({}, candidateToRequestFrames);
    }

    Y_UNIT_TEST(ForceInvalidScenarioLeaveNothing) {
        InitDefaultExpects();

        THashMap<TString, TMaybe<TString>> expFlags = {{TString{EXP_PREFIX_MM_FORCE_SCENARIO} + "!@#", ""}};
        EXPECT_CALL(Ctx, ExpFlags()).WillRepeatedly(ReturnRef(expFlags));

        auto candidateToRequestFrames = MakeMapping(Registry.GetScenarioRefs());
        PreClassify(candidateToRequestFrames, Request, Ctx, FormulasStorage, FactorStorage, QualityStorage);
        CheckOnlyAllowed({}, candidateToRequestFrames);
    }

    Y_UNIT_TEST(EnableOnlyOneScenarioWithSemanticFrames) {
        InitDefaultExpects();
        AddProtocolScenario(HOLLYWOOD_COMMANDS_SCENARIO);

        THashMap<TString, TMaybe<TString>> expFlags = {{TString{EXP_PREFIX_MM_SCENARIO} + HOLLYWOOD_COMMANDS_SCENARIO, ""}};
        EXPECT_CALL(Ctx, ExpFlags()).WillRepeatedly(ReturnRef(expFlags));

        auto candidateToRequestFrames = MakeMapping(Registry.GetScenarioRefs());
        PreClassify(candidateToRequestFrames, Request, Ctx, FormulasStorage, FactorStorage, QualityStorage);
        CheckOnlyAllowed({HOLLYWOOD_COMMANDS_SCENARIO}, candidateToRequestFrames);
    }

    Y_UNIT_TEST(EnableOnlyOneScenarioWithoutSemanticFrames) {
        InitDefaultExpects();
        AddProtocolScenario(HOLLYWOOD_COMMANDS_SCENARIO);

        THashMap<TString, TMaybe<TString>> expFlags = {{TString{EXP_PREFIX_MM_SCENARIO} + HOLLYWOOD_COMMANDS_SCENARIO, ""}};
        EXPECT_CALL(Ctx, ExpFlags()).WillRepeatedly(ReturnRef(expFlags));

        auto candidateToRequestFrames = MakeMapping(Registry.GetScenarioRefs(), {TEST_SCENARIO_NAME});
        PreClassify(candidateToRequestFrames, Request, Ctx, FormulasStorage, FactorStorage, QualityStorage);
        CheckOnlyAllowed({}, candidateToRequestFrames);
    }

    Y_UNIT_TEST(EnableOnlyOneScenarioWithSemanticFramesAndForceIt) {
        InitDefaultExpects();
        AddProtocolScenario(HOLLYWOOD_COMMANDS_SCENARIO);

        THashMap<TString, TMaybe<TString>> expFlags = {{TString{EXP_PREFIX_MM_FORCE_SCENARIO} + HOLLYWOOD_COMMANDS_SCENARIO, ""}};
        EXPECT_CALL(Ctx, ExpFlags()).WillRepeatedly(ReturnRef(expFlags));

        auto candidateToRequestFrames = MakeMapping(Registry.GetScenarioRefs());
        PreClassify(candidateToRequestFrames, Request, Ctx, FormulasStorage, FactorStorage, QualityStorage);
        CheckOnlyAllowed({HOLLYWOOD_COMMANDS_SCENARIO}, candidateToRequestFrames);
    }

    Y_UNIT_TEST(EnableOnlyOneScenarioWithoutSemanticFramesAndForceIt) {
        InitDefaultExpects();
        AddProtocolScenario(HOLLYWOOD_COMMANDS_SCENARIO);

        THashMap<TString, TMaybe<TString>> expFlags = {{TString{EXP_PREFIX_MM_FORCE_SCENARIO} + HOLLYWOOD_COMMANDS_SCENARIO, ""}};
        EXPECT_CALL(Ctx, ExpFlags()).WillRepeatedly(ReturnRef(expFlags));

        auto candidateToRequestFrames = MakeMapping(Registry.GetScenarioRefs(), {TEST_SCENARIO_NAME});
        PreClassify(candidateToRequestFrames, Request, Ctx, FormulasStorage, FactorStorage, QualityStorage);
        CheckOnlyAllowed({HOLLYWOOD_COMMANDS_SCENARIO}, candidateToRequestFrames);
    }

    Y_UNIT_TEST(ForceActiveScenario) {
        InitDefaultExpects();
        AddProtocolScenario(TEST_SCENARIO_NAME);

        {
            auto scenarioSession = NewScenarioSession(TState{});
            scenarioSession.SetActivityTurn(1);
            const auto session = MakeSessionBuilder()
                ->SetPreviousScenarioName(TString{TEST_SCENARIO_NAME})
                .SetScenarioSession(TString{TEST_SCENARIO_NAME}, scenarioSession)
                .Build();
            EXPECT_CALL(Ctx, Session()).WillRepeatedly(Return(session.Get()));

            auto candidateToRequestFrames = MakeMapping(Registry.GetScenarioRefs(), {MM_PROTO_VINS_SCENARIO});
            PreClassify(candidateToRequestFrames, Request, Ctx, FormulasStorage, FactorStorage, QualityStorage);
            CheckOnlyAllowed({TEST_SCENARIO_NAME, MM_PROTO_VINS_SCENARIO}, candidateToRequestFrames);
        }
    }

    Y_UNIT_TEST(InvokesPerSecondSensor) {
        InitDefaultExpects();

        const TString TEST_SCENARIO_NAME_1 = "TestScenario1";
        const TString TEST_SCENARIO_NAME_2 = "TestScenario2";
        const TString TEST_SCENARIO_NAME_3 = "TestScenario3";

        Registry.RegisterConfigBasedAppHostProxyProtocolScenario(
                MakeHolder<TConfigBasedAppHostProxyProtocolScenario>(ProtocolTestScenarioConfig(TEST_SCENARIO_NAME_1)));
        Registry.RegisterConfigBasedAppHostProxyProtocolScenario(
                MakeHolder<TConfigBasedAppHostProxyProtocolScenario>(ProtocolTestScenarioConfig(TEST_SCENARIO_NAME_2)));
        Registry.RegisterConfigBasedAppHostProxyProtocolScenario(
                MakeHolder<TConfigBasedAppHostProxyProtocolScenario>(ProtocolTestScenarioConfig(TEST_SCENARIO_NAME_3)));

        const TSet<TString> expectedScenarios{TEST_SCENARIO_NAME_1, TEST_SCENARIO_NAME_2};
        TScenarioToRequestFrames candidates;
        for (const auto& ref : Registry.GetScenarioRefs()) {
            if (expectedScenarios.contains(ref->GetScenario().GetName())) {
                TSemanticFrame frame;
                frame.SetName(TString(DEFAULT_FRAME_NAME));
                candidates[ref] = {frame};
            }
        }

        StrictMock<TMockSensors> sensors;
        EXPECT_CALL(sensors, IncRate(NMonitoring::TLabels{
            {NAlice::NSignal::SCENARIO_NAME, TEST_SCENARIO_NAME_1},
            {NAlice::NSignal::NAME, "invokes_per_second"}
        }));
        EXPECT_CALL(sensors, IncRate(NMonitoring::TLabels{
            {NAlice::NSignal::SCENARIO_NAME, TEST_SCENARIO_NAME_2},
            {NAlice::NSignal::NAME, "invokes_per_second"}
        }));
        EXPECT_CALL(sensors, IncRate(NMonitoring::TLabels{
            {NAlice::NSignal::SCENARIO_NAME, TEST_SCENARIO_NAME_3},
            {NAlice::NSignal::NAME, "invokes_per_second"}
        })).WillRepeatedly(Invoke([](){
            UNIT_ASSERT(false);
        }));
        EXPECT_CALL(Ctx, Sensors()).WillRepeatedly(ReturnRef(sensors));
        PreClassify(candidates, Request, Ctx, FormulasStorage, FactorStorage, QualityStorage);
    }

    Y_UNIT_TEST(ActiveScenarioWithTimeouts) {
        InitDefaultExpects();
        NAlice::TConfig::TScenarios::TConfig scenarioConfig;
        scenarioConfig.MutableDialogManagerParams()->SetActiveScenarioTimeoutSeconds(1'000'000);

        Registry.RegisterConfigBasedAppHostPureProtocolScenario(MakeHolder<TConfigBasedAppHostPureProtocolScenario>(
            ProtocolTestScenarioConfig(TEST_SCENARIO_NAME)));
        const auto test = [&](const auto timestamp, const TSet<TStringBuf>& expectedScenarioNames) {
            auto scenarioSession = NewScenarioSession(TState{});
            scenarioSession.SetActivityTurn(1);
            scenarioSession.SetTimestamp(timestamp);
            const auto session = MakeSessionBuilder()
                                     ->SetPreviousScenarioName(TString{TEST_SCENARIO_NAME})
                                     .SetScenarioSession(TString{TEST_SCENARIO_NAME}, scenarioSession)
                                     .Build();
            EXPECT_CALL(Ctx, Session()).WillRepeatedly(Return(session.Get()));
            EXPECT_CALL(Ctx, ScenarioConfig(_)).WillRepeatedly(ReturnRef(scenarioConfig));

            auto candidateToRequestFrames = MakeMapping(Registry.GetScenarioRefs(), {MM_PROTO_VINS_SCENARIO});
            PreClassify(candidateToRequestFrames, Request, Ctx, FormulasStorage, FactorStorage, QualityStorage);
            CheckOnlyAllowed(expectedScenarioNames, candidateToRequestFrames);
        };
        test(/* timestamp= */ 0, /* expectedScenarioNames= */ {MM_PROTO_VINS_SCENARIO});
        test(/* timestamp= */ TInstant::Now().MicroSeconds(),
             /* expectedScenarioNames= */ {TEST_SCENARIO_NAME, MM_PROTO_VINS_SCENARIO});
    }

    Y_UNIT_TEST(ForcePlayerOwnerScenario) {
        InitDefaultExpects();

        NAlice::TConfig::TScenarios::TConfig scenarioConfig;
        scenarioConfig.MutableDialogManagerParams()->SetIsPlayerOwnerPriorityAllowed(true);
        EXPECT_CALL(Ctx, ScenarioConfig(TString{TEST_SCENARIO_NAME})).WillRepeatedly(ReturnRef(scenarioConfig));

        auto speechKitRequest = TSpeechKitRequestBuilder(TSpeechKitRequestBuilder::EPredefined::MinimalWithTextEvent)
            .SetProtoPatcher([&](NMegamind::TSpeechKitInitContext& ctx) {
                auto* deviceState = ctx.Proto->MutableRequest()->MutableDeviceState();
                auto& playerMeta = *deviceState->MutableAudioPlayer()->MutableScenarioMeta();
                playerMeta[TString{NMegamind::SCENARIO_NAME_JSON_KEY}] = TEST_SCENARIO_NAME;
            })
            .Build();
        EXPECT_CALL(Ctx, SpeechKitRequest()).WillRepeatedly(Return(speechKitRequest));

        AddProtocolScenario(TEST_SCENARIO_NAME);

        auto candidateToRequestFrames = MakeMapping(Registry.GetScenarioRefs(), {MM_PROTO_VINS_SCENARIO});
        PreClassify(candidateToRequestFrames, Request, Ctx, FormulasStorage, FactorStorage, QualityStorage);
        CheckOnlyAllowed({TEST_SCENARIO_NAME, MM_PROTO_VINS_SCENARIO}, candidateToRequestFrames);
    }

    Y_UNIT_TEST(SearchappHollywoodMusicNoExp) {
        InitDefaultExpects();
        CheckHollywoodWithThreshold(*this, /* hasExp= */ false);
    }

    Y_UNIT_TEST(SearchappHollywoodMusicExp) {
        InitDefaultExpects();
        CheckHollywoodWithThreshold(*this, /* hasExp= */ true);
    }

    Y_UNIT_TEST(ImageInput) {
        InitDefaultExpects();
        CheckMediaInput(*this, EEventType::image_input);
    }

    Y_UNIT_TEST(MusicInput) {
        InitDefaultExpects();
        CheckMediaInput(*this, EEventType::music_input);
    }

    Y_UNIT_TEST(WithExpThresholds) {
        const THashMap<TStringBuf, double> scenarioToThreshold = {
            {MM_PROTO_VINS_SCENARIO, 1.0},
        };
        const TSet<TStringBuf> expectedScenarios = {
            HOLLYWOOD_COMMANDS_SCENARIO,
        };

        CheckWithExpThresholds(scenarioToThreshold, expectedScenarios, *this, {},
                               "mm_pre_confident_scenario_threshold__ECT_UNKNOWN__Vins=-5.0");
    }

    Y_UNIT_TEST(WithExpThresholds2) {
        const THashMap<TStringBuf, double> scenarioToThreshold = {
            {MM_PROTO_VINS_SCENARIO, 1.0},
            {HOLLYWOOD_COMMANDS_SCENARIO, 1.0},
        };

        CheckWithExpThresholds(scenarioToThreshold, {}, *this, {},
                               "mm_pre_confident_scenario_threshold__ECT_UNKNOWN__Vins=-5.0");
    }

    const TString ACTION_SCENARIO = "ActionScenario";
    Y_UNIT_TEST(KeepRecognizedAction) {
        AddProtocolScenario(ACTION_SCENARIO, {DEFAULT_FRAME_NAME, ACTION_FRAME_NAME});

        const THashMap<TStringBuf, double> scenarioToThreshold = {
            {MM_PROTO_VINS_SCENARIO, 1.0},
            {HOLLYWOOD_COMMANDS_SCENARIO, 1.0},
            {ACTION_SCENARIO, 1.0}
        };
        const TSet<TStringBuf> expectedScenarios = {
            ACTION_SCENARIO,
        };
        Request = std::move(NMegamind::TRequestBuilder{Request}
            .SetRecognizedActionEffectFrames({TSemanticFrameBuilder{TString{ACTION_FRAME_NAME}}.Build()}))
            .Build();

        CheckWithExpThresholds(scenarioToThreshold, expectedScenarios, *this, {},
                               "mm_pre_confident_scenario_threshold__ECT_UNKNOWN__Vins=-5.0");
    }

    Y_UNIT_TEST(KeepRecognizedActionAndMakeConfident) {
        AddProtocolScenario(ACTION_SCENARIO, {DEFAULT_FRAME_NAME, ACTION_FRAME_NAME});

        const THashMap<TStringBuf, double> scenarioToThreshold = {
            {MM_PROTO_VINS_SCENARIO, 1.0},
            {HOLLYWOOD_COMMANDS_SCENARIO, 1.0},
            {ACTION_SCENARIO, 1.0}
        };
        const TSet<TStringBuf> expectedScenarios = {
            ACTION_SCENARIO,
        };
        Request = std::move(NMegamind::TRequestBuilder{Request}
            .SetRecognizedActionEffectFrames({TSemanticFrameBuilder{TString{ACTION_FRAME_NAME}}.Build()}))
            .Build();

        CheckWithExpThresholds(scenarioToThreshold, expectedScenarios, *this, {});
    }

    Y_UNIT_TEST(WithExpThresholds3) {
        const THashMap<TStringBuf, double> scenarioToThreshold = {
            {MM_PROTO_VINS_SCENARIO, 1.0},
        };
        const TSet<TStringBuf> expectedScenarios = {
            HOLLYWOOD_COMMANDS_SCENARIO,
            MM_PROTO_VINS_SCENARIO,
        };

        auto wizardResponse = ParseBegemotResponseJsonToWizardResponse(R"({
            "AliceResponse": {
                "Granet": {
                    "Forms": [
                        {
                            "Name": "alice.meditation",
                            "LogProbability": -0.1
                        }
                    ]
                }
            }
        })");

        CheckWithExpThresholds(scenarioToThreshold, expectedScenarios, *this, std::move(wizardResponse));
    }

    Y_UNIT_TEST(KeepScenarioByFixList) {
        InitDefaultExpects();
        const auto wizardResponseFl = ParseBegemotResponseJsonToWizardResponse(TStringBuilder{} << R"({
            "AliceResponse": {
                "AliceFixlist": {
                    "Matches": {
                        "general_fixlist": {
                            "Intents" : [
                                "TestScenario2"
                            ]
                        }
                    }
                },
                "AliceParsedFrames": {
                    "Frames": [
                        {
                            "Name": ")" << FAST_FRAME_NAME << R"("
                        }
                    ],
                    "Sources": [
                        "Granet"
                    ],
                    "Confidences": [
                        1.0
                    ]
                }
            }
        })");

        const auto wizardResponseNoFl = ParseBegemotResponseJsonToWizardResponse(TStringBuilder{} << R"({
            "AliceResponse": {
                "AliceParsedFrames": {
                    "Frames": [
                        {
                            "Name": ")" << FAST_FRAME_NAME << R"("
                        }
                    ],
                    "Sources": [
                        "Granet"
                    ],
                    "Confidences": [
                        1.0
                    ]
                }
            }
        })");

        CheckPreclassifyKeepsFixlist(*this, wizardResponseNoFl, wizardResponseFl);
    }

    Y_UNIT_TEST(KeepVideoWithItemSelector) {
        InitDefaultExpects();
        const auto wizardResponseEmpty = ParseBegemotResponseJsonToWizardResponse(TStringBuilder() << R"({
            "AliceResponse": {
                "AliceParsedFrames": {
                    "Frames": [
                        {
                            "Name": ")" << FAST_FRAME_NAME << R"("
                        }
                    ],
                    "Sources": [
                        "Granet"
                    ],
                    "Confidences": [
                        1.0
                    ]
                }
            }
        })");

        const auto wizardResponseNoSelected = ParseBegemotResponseJsonToWizardResponse(TStringBuilder{} << R"({
            "AliceResponse": {
                "AliceItemSelector": {
                    "Galleries": [
                        {
                            "GalleryName": "TestGallery",
                            "Items": [
                                {
                                    "Score": 0.1,
                                    "IsSelected": false,
                                    "Alias": "asdawd"
                                }
                            ]
                        }
                    ]
                },
                "AliceParsedFrames": {
                    "Frames": [
                        {
                            "Name": ")" << FAST_FRAME_NAME << R"("
                        }
                    ],
                    "Sources": [
                        "Granet"
                    ],
                    "Confidences": [
                        1.0
                    ]
                }
            }
        })");

        const auto wizardResponseItemSelected = ParseBegemotResponseJsonToWizardResponse(TStringBuilder{} << R"({
            "AliceResponse": {
                "AliceItemSelector": {
                    "Galleries": [
                        {
                            "GalleryName": "TestGallery",
                            "Items": [
                                {
                                    "Score": 0.1,
                                    "IsSelected": true,
                                    "Alias": "asdawd"
                                }
                            ]
                        }
                    ]
                },
                "AliceParsedFrames": {
                    "Frames": [
                        {
                            "Name": ")" << FAST_FRAME_NAME << R"("
                        }
                    ],
                    "Sources": [
                        "Granet"
                    ],
                    "Confidences": [
                        1.0
                    ]
                }
            }
        })");

        TestKeepsVideoOnHints(*this, wizardResponseEmpty, wizardResponseNoSelected, wizardResponseItemSelected);
    }

    Y_UNIT_TEST(BoostScenarioByPreclassifierHintByFrame) {
        InitDefaultExpects();

        const auto wizardResponse = ParseBegemotResponseJsonToWizardResponse(TStringBuilder() << R"({
            "AliceResponse": {
                "AliceParsedFrames": {
                    "Frames": [
                        {
                            "Name": ")" << FAST_FRAME_NAME << R"("
                        }
                    ],
                    "Sources": [
                        "Granet"
                    ],
                    "Confidences": [
                        1.0
                    ]
                }
            }
        })");
        TestBoostScenarioByPreclassifierHint(*this, wizardResponse);
    }

    Y_UNIT_TEST(BoostScenarioByPreclassifierHintByAction) {
        InitDefaultExpects();

        Request = std::move(NMegamind::TRequestBuilder{Request}
            .SetRecognizedActionEffectFrames({TSemanticFrameBuilder{TString{ACTION_FRAME_NAME}}.Build()}))
            .Build();
        TestBoostScenarioByPreclassifierHint(*this, TWizardResponse());
    }

    Y_UNIT_TEST(PreclassifierWithConfidentScenario) {
        const THashMap<TStringBuf, double> scenarioToThreshold = {
            {MM_PROTO_VINS_SCENARIO, 1.0},
        };
        const TSet<TStringBuf> expectedScenarios = {
            HOLLYWOOD_COMMANDS_SCENARIO,
        };

        CheckWithExpThresholds(scenarioToThreshold, expectedScenarios, *this, {},
                               "mm_pre_confident_scenario_threshold__ECT_UNKNOWN__Vins=-5.0");
    }

    Y_UNIT_TEST(PreclassifierWithoutConfidentScenario) {
        const THashMap<TStringBuf, double> scenarioToThreshold = {
            {MM_PROTO_VINS_SCENARIO, 1.0},
        };
        const TSet<TStringBuf> expectedScenarios = {
            HOLLYWOOD_COMMANDS_SCENARIO,
            MM_PROTO_VINS_SCENARIO,
        };

        CheckWithExpThresholds(scenarioToThreshold, expectedScenarios, *this, {},
                               "mm_pre_confident_scenario_threshold__ECT_UNKNOWN__Vins=5.0");
    }

    Y_UNIT_TEST(LogScenarioToRequestFramesEmpty) {
        Registry.RegisterConfigBasedAppHostPureProtocolScenario(MakeHolder<TTestProtocolScenario>("qwe"));
        auto candidateToRequestFrames = MakeMapping({});

        const auto actual = NImpl::LogScenarioToRequestFrames(candidateToRequestFrames);

        UNIT_ASSERT_EQUAL("-", actual);
    }

    Y_UNIT_TEST(LogScenarioToRequestFramesOne) {
        TScenarioRefs scenarioRefs;
        TTestProtocolScenario scenario1("lol");
        scenarioRefs.push_back(MakeIntrusive<TScenarioRef<TTestProtocolScenario>>(scenario1));

        auto candidateToRequestFrames = MakeMapping(scenarioRefs);
        const auto actual = NImpl::LogScenarioToRequestFrames(candidateToRequestFrames);

        UNIT_ASSERT_EQUAL("lol", actual);
    }

    Y_UNIT_TEST(LogScenarioToRequestFramesTwo) {
        TScenarioRefs scenarioRefs;
        TTestProtocolScenario scenario1("lol");
        TTestProtocolScenario scenario2("kek");
        scenarioRefs.push_back(MakeIntrusive<TScenarioRef<TTestProtocolScenario>>(scenario1));
        scenarioRefs.push_back(MakeIntrusive<TScenarioRef<TTestProtocolScenario>>(scenario2));

        auto candidateToRequestFrames = MakeMapping(scenarioRefs);
        const auto actual = NImpl::LogScenarioToRequestFrames(candidateToRequestFrames);

        UNIT_ASSERT(("lol, kek" == actual) || ("kek, lol" == actual));
    }

    Y_UNIT_TEST(LeaveOnly) {
        TScenarioRefs scenarioRefs;
        TTestProtocolScenario scenario1("lol");
        TTestProtocolScenario scenario2("kek");
        TTestProtocolScenario scenario3("123");
        EXPECT_CALL(scenario1, IsEnabled(_)).WillOnce(Return(true));
        EXPECT_CALL(Ctx, HasExpFlag(EXP_PREFIX_MM_DISABLE_PROTOCOL_SCENARIO + scenario1.GetName())).WillOnce(Return(false));
        scenarioRefs.push_back(MakeIntrusive<TScenarioRef<TTestProtocolScenario>>(scenario1));
        scenarioRefs.push_back(MakeIntrusive<TScenarioRef<TTestProtocolScenario>>(scenario2));
        scenarioRefs.push_back(MakeIntrusive<TScenarioRef<TTestProtocolScenario>>(scenario3));

        auto candidateToRequestFrames = MakeMapping(scenarioRefs);
        NImpl::LeaveOnly({"lol"}, candidateToRequestFrames, Ctx, QualityStorage, LR_FORMULA);

        CheckOnlyAllowed({"lol"}, candidateToRequestFrames);
    }

    Y_UNIT_TEST(LeaveOnlyEmpty) {
        TScenarioRefs scenarioRefs;
        TTestProtocolScenario scenario1("lol");
        TTestProtocolScenario scenario2("kek");
        TTestProtocolScenario scenario3("123");
        scenarioRefs.push_back(MakeIntrusive<TScenarioRef<TTestProtocolScenario>>(scenario1));
        scenarioRefs.push_back(MakeIntrusive<TScenarioRef<TTestProtocolScenario>>(scenario2));
        scenarioRefs.push_back(MakeIntrusive<TScenarioRef<TTestProtocolScenario>>(scenario3));

        auto candidateToRequestFrames = MakeMapping(scenarioRefs);
        NImpl::LeaveOnly({"321"}, candidateToRequestFrames, Ctx, QualityStorage, LR_FORMULA);

        UNIT_ASSERT(candidateToRequestFrames.empty());
    }

    Y_UNIT_TEST(LeaveOnlyTwo) {
        TScenarioRefs scenarioRefs;
        TTestProtocolScenario scenario1("lol");
        TTestProtocolScenario scenario2("kek");
        TTestProtocolScenario scenario3("123");
        EXPECT_CALL(scenario1, IsEnabled(_)).WillOnce(Return(true));
        EXPECT_CALL(scenario2, IsEnabled(_)).WillOnce(Return(true));
        EXPECT_CALL(Ctx, HasExpFlag(EXP_PREFIX_MM_DISABLE_PROTOCOL_SCENARIO + scenario1.GetName())).WillOnce(Return(false));
        EXPECT_CALL(Ctx, HasExpFlag(EXP_PREFIX_MM_DISABLE_PROTOCOL_SCENARIO + scenario2.GetName())).WillOnce(Return(false));
        scenarioRefs.push_back(MakeIntrusive<TScenarioRef<TTestProtocolScenario>>(scenario1));
        scenarioRefs.push_back(MakeIntrusive<TScenarioRef<TTestProtocolScenario>>(scenario2));
        scenarioRefs.push_back(MakeIntrusive<TScenarioRef<TTestProtocolScenario>>(scenario3));

        auto candidateToRequestFrames = MakeMapping(scenarioRefs);
        NImpl::LeaveOnly({"lol", "kek"}, candidateToRequestFrames, Ctx, QualityStorage, LR_FORMULA);

        CheckOnlyAllowed({"lol", "kek"}, candidateToRequestFrames);
    }

    Y_UNIT_TEST(TryEraseIf) {
        TScenarioRefs scenarioRefs;
        TTestProtocolScenario scenario1("1");
        TTestProtocolScenario scenario2("22");
        TTestProtocolScenario scenario3("333");
        TTestProtocolScenario scenario4("4444");
        scenarioRefs.push_back(MakeIntrusive<TScenarioRef<TTestProtocolScenario>>(scenario1));
        scenarioRefs.push_back(MakeIntrusive<TScenarioRef<TTestProtocolScenario>>(scenario2));
        scenarioRefs.push_back(MakeIntrusive<TScenarioRef<TTestProtocolScenario>>(scenario3));
        scenarioRefs.push_back(MakeIntrusive<TScenarioRef<TTestProtocolScenario>>(scenario4));
        auto candidateToRequestFrames = MakeMapping(scenarioRefs);

        NImpl::TryEraseIf(candidateToRequestFrames, [](const TScenarioToRequestFrames::value_type& candidateFrames) {
            return candidateFrames.first->GetScenario().GetName().size() > 2;
        }, QualityStorage, LR_FORMULA);

        CheckOnlyAllowed({"1", "22"}, candidateToRequestFrames);
    }

    Y_UNIT_TEST(EmptyUtteranceIsNotAnyUtterance) {
        InitDefaultExpects();

        const TString semanticFrameName = "search_semantic_frame";

        const auto& constructFrame = [&semanticFrameName](){
            TSemanticFrame frame;
            frame.SetName(semanticFrameName);
            return frame;
        };

        auto event = std::make_unique<TTextInputEvent>(Default<TString>(), /* isUserGenerated= */ false);

        const auto request = CreateRequest(std::move(event), SpeechKitRequest,
                                           /* iotUserInfo= */ Nothing(),
                                           /* requestSource= */ {}, {constructFrame()});

        Registry.RegisterConfigBasedAppHostPureProtocolScenario(
            MakeHolder<TConfigBasedAppHostPureProtocolScenario>(
                ProtocolTestScenarioConfig(TEST_SCENARIO_NAME, {semanticFrameName})));

        TScenarioToRequestFrames candidateToRequestFrames;
        for (const auto& ref : Registry.GetScenarioRefs()) {
            if (ref->GetScenario().GetName() == TEST_SCENARIO_NAME) {
                candidateToRequestFrames[ref] = {constructFrame()};
            } else {
                candidateToRequestFrames[ref] = {};
            }
        }

        PreClassify(candidateToRequestFrames, request, Ctx, FormulasStorage, FactorStorage, QualityStorage);
        CheckOnlyAllowed({TEST_SCENARIO_NAME}, candidateToRequestFrames);
    }
}

Y_UNIT_TEST_SUITE(ShouldFilterMiscHollywoodMusic) {
    Y_UNIT_TEST(AnotherScenario) {
        TMockContext ctx;
        TTestScenario scenario(MM_SEARCH_PROTOCOL_SCENARIO);

        UNIT_ASSERT(!NImpl::ShouldFilterMiscHollywoodMusic(ctx, scenario));
    }

    Y_UNIT_TEST(Default) {
        TMockContext ctx;
        TLocalExpFlags expFlags;
        TClientInfoProto clientInfoProto;
        clientInfoProto.SetAppId("ru.yandex.iosdk.elariwatch");
        TClientInfo clientInfo{clientInfoProto};
        EXPECT_CALL(ctx, ExpFlags()).WillRepeatedly(ReturnRef(expFlags));

        EXPECT_CALL(ctx, Logger()).WillRepeatedly(ReturnRef(TRTLogger::NullLogger()));
        EXPECT_CALL(ctx, ClientInfo()).WillRepeatedly(ReturnRef(clientInfo));

        TMockResponses responses;
        responses.SetWizardResponse(PrepareMusicWizardResponse());
        EXPECT_CALL(ctx, Responses()).WillRepeatedly(ReturnRef(responses));

        TTestScenario scenario(HOLLYWOOD_MUSIC_SCENARIO);

        UNIT_ASSERT(!NImpl::ShouldFilterMiscHollywoodMusic(ctx, scenario));
    }

    Y_UNIT_TEST(Filter) {
        TMockContext ctx;
        TLocalExpFlags expFlags = {
            {TString{EXP_MUSIC_PLAY_CONFIDENCE_THRESHOLD_PREFIX} + "0.6", "1"},
        };
        TClientInfoProto clientInfoProto;
        clientInfoProto.SetAppId("ru.yandex.iosdk.elariwatch");
        TClientInfo clientInfo{clientInfoProto};
        EXPECT_CALL(ctx, ExpFlags()).WillRepeatedly(ReturnRef(expFlags));

        EXPECT_CALL(ctx, Logger()).WillRepeatedly(ReturnRef(TRTLogger::NullLogger()));
        EXPECT_CALL(ctx, ClientInfo()).WillRepeatedly(ReturnRef(clientInfo));

        TMockResponses responses;
        responses.SetWizardResponse(PrepareMusicWizardResponse());
        EXPECT_CALL(ctx, Responses()).WillRepeatedly(ReturnRef(responses));

        TTestScenario scenario(HOLLYWOOD_MUSIC_SCENARIO);
        UNIT_ASSERT(NImpl::ShouldFilterMiscHollywoodMusic(ctx, scenario));
    }

    Y_UNIT_TEST(NotFilterExp) {
        TMockContext ctx;
        TLocalExpFlags expFlags = {
            {TString{EXP_MUSIC_PLAY_CONFIDENCE_THRESHOLD_PREFIX} + "0.6", "1"},
            {TString{EXP_MUSIC_PLAY_DISABLE_CONFIDENCE_THRESHOLD}, "1"},
        };
        TClientInfo clientInfo{TClientInfoProto{}};
        EXPECT_CALL(ctx, ExpFlags()).WillRepeatedly(ReturnRef(expFlags));

        EXPECT_CALL(ctx, Logger()).WillRepeatedly(ReturnRef(TRTLogger::NullLogger()));
        EXPECT_CALL(ctx, ClientInfo()).WillRepeatedly(ReturnRef(clientInfo));

        TMockResponses responses;
        responses.SetWizardResponse(PrepareMusicWizardResponse());
        EXPECT_CALL(ctx, Responses()).WillRepeatedly(ReturnRef(responses));

        TTestScenario scenario(HOLLYWOOD_MUSIC_SCENARIO);
        UNIT_ASSERT(!NImpl::ShouldFilterMiscHollywoodMusic(ctx, scenario));
    }

    Y_UNIT_TEST(NotFilter) {
        TMockContext ctx;
        TLocalExpFlags expFlags = {
            {TString{EXP_MUSIC_PLAY_CONFIDENCE_THRESHOLD_PREFIX} + "0.4", "1"},
        };
        TClientInfo clientInfo{TClientInfoProto{}};
        EXPECT_CALL(ctx, ExpFlags()).WillRepeatedly(ReturnRef(expFlags));

        EXPECT_CALL(ctx, Logger()).WillRepeatedly(ReturnRef(TRTLogger::NullLogger()));
        EXPECT_CALL(ctx, ClientInfo()).WillRepeatedly(ReturnRef(clientInfo));

        TMockResponses responses;
        responses.SetWizardResponse(PrepareMusicWizardResponse());
        EXPECT_CALL(ctx, Responses()).WillRepeatedly(ReturnRef(responses));

        TTestScenario scenario(HOLLYWOOD_MUSIC_SCENARIO);
        UNIT_ASSERT(!NImpl::ShouldFilterMiscHollywoodMusic(ctx, scenario));
    }

    Y_UNIT_TEST(BadExp) {
        TMockContext ctx;
        TLocalExpFlags expFlags = {
            {TString{EXP_MUSIC_PLAY_CONFIDENCE_THRESHOLD_PREFIX} + "huge", "1"},
        };
        TClientInfoProto clientInfoProto;
        clientInfoProto.SetAppId("ru.yandex.iosdk.elariwatch");
        TClientInfo clientInfo{clientInfoProto};
        EXPECT_CALL(ctx, ExpFlags()).WillRepeatedly(ReturnRef(expFlags));

        EXPECT_CALL(ctx, Logger()).WillRepeatedly(ReturnRef(TRTLogger::NullLogger()));
        EXPECT_CALL(ctx, ClientInfo()).WillRepeatedly(ReturnRef(clientInfo));

        TMockResponses responses;
        responses.SetWizardResponse(PrepareMusicWizardResponse());
        EXPECT_CALL(ctx, Responses()).WillRepeatedly(ReturnRef(responses));

        TTestScenario scenario(HOLLYWOOD_MUSIC_SCENARIO);
        UNIT_ASSERT(NImpl::ShouldFilterMiscHollywoodMusic(ctx, scenario));
    }
}

Y_UNIT_TEST_SUITE(GetExpThresholds) {
    Y_UNIT_TEST(WithPresentExp) {
        TLocalExpFlags expFlags = {{"mm_preclassifier_thresholds=Vins:-4.2", "1"}};
        const THashMap<TStringBuf, double> expected = {{"Vins", -4.2}};
        const auto actual = NImpl::GetExpThresholds(expFlags, TRTLogger::NullLogger());
        UNIT_ASSERT_VALUES_EQUAL(actual, expected);
    }

    Y_UNIT_TEST(WithPresentExp2) {
        TLocalExpFlags expFlags = {
            {"mm_preclassifier_thresholds=Vins:-4.2;Vins:-4.2;"
             "HollywoodMusic:-1.1;Video:-2.1", "1"},
        };
        const THashMap<TStringBuf, double> expected = {
            {"Vins", -4.2},
            {"Vins", -4.2},
            {"HollywoodMusic", -1.1},
            {"Video", -2.1},
        };
        const auto actual = NImpl::GetExpThresholds(expFlags, TRTLogger::NullLogger());
        UNIT_ASSERT_VALUES_EQUAL(actual, expected);
    }

    Y_UNIT_TEST(WithAbsentExp) {
        TLocalExpFlags expFlags = {};
        const THashMap<TStringBuf, double> expected = {};
        const auto actual = NImpl::GetExpThresholds(expFlags, TRTLogger::NullLogger());
        UNIT_ASSERT_VALUES_EQUAL(actual, expected);
    }
}

Y_UNIT_TEST_SUITE(GetThresholdsFlag) {
    Y_UNIT_TEST(OneScenario) {
        const THashMap<TStringBuf, double> thresholds = {{"Vins", -4.2}};
        const TStringBuf expected = "mm_preclassifier_thresholds=Vins:-4.2";
        const auto actual = GetThresholdsFlag(thresholds);
        UNIT_ASSERT_VALUES_EQUAL(actual, expected);
    }

    Y_UNIT_TEST(TwoScenarios) {
        const THashMap<TStringBuf, double> thresholds = {
            {"HollywoodMusic", -1.8},
            {"Video", -2.1},
        };
        const TVector<TString> expectedVariants = {
            "mm_preclassifier_thresholds=HollywoodMusic:-1.8;Video:-2.1",
            "mm_preclassifier_thresholds=Video:-2.1;HollywoodMusic:-1.8"
        };
        const auto actual = GetThresholdsFlag(thresholds);
        UNIT_ASSERT(Find(expectedVariants, actual) != expectedVariants.end());
    }
}

Y_UNIT_TEST_SUITE(ShouldFilterProtocolVideoScenario) {
    Y_UNIT_TEST(AnotherScenario) {
        TMockContext ctx;
        const auto clientFeatures = GetClientFeaturesFor("ru.yandex.quasar");
        EXPECT_CALL(ctx, ClientFeatures()).WillRepeatedly(ReturnRef(clientFeatures));
        TTestScenario scenario(MM_SEARCH_PROTOCOL_SCENARIO);

        UNIT_ASSERT(!NImpl::ShouldFilterProtocolVideoScenario(ctx, scenario));
    }

    Y_UNIT_TEST(Filter) {
        TMockContext ctx;
        const auto clientFeatures = GetClientFeaturesFor("ru.yandex.mobile");
        EXPECT_CALL(ctx, ClientFeatures()).WillRepeatedly(ReturnRef(clientFeatures));
        TTestScenario scenario(MM_VIDEO_PROTOCOL_SCENARIO);

        UNIT_ASSERT(NImpl::ShouldFilterProtocolVideoScenario(ctx, scenario));
    }

    Y_UNIT_TEST(NotFilterSmartSpeaker) {
        TMockContext ctx;
        const auto clientFeatures = GetClientFeaturesFor("ru.yandex.quasar");
        EXPECT_CALL(ctx, ClientFeatures()).WillRepeatedly(ReturnRef(clientFeatures));
        TTestScenario scenario(MM_VIDEO_PROTOCOL_SCENARIO);

        UNIT_ASSERT(!NImpl::ShouldFilterProtocolVideoScenario(ctx, scenario));
    }

    Y_UNIT_TEST(NotFilterTvDevice) {
        TMockContext ctx;
        const auto clientFeatures = GetClientFeaturesFor("com.yandex.tv.alice");
        EXPECT_CALL(ctx, ClientFeatures()).WillRepeatedly(ReturnRef(clientFeatures));
        TTestScenario scenario(MM_VIDEO_PROTOCOL_SCENARIO);

        UNIT_ASSERT(!NImpl::ShouldFilterProtocolVideoScenario(ctx, scenario));
    }
}

Y_UNIT_TEST_SUITE(ShouldFilterNewsScenario) {
    TTestScenario scenarioNews(MM_NEWS_PROTOCOL_SCENARIO);

    Y_UNIT_TEST(NotFilterPreciseGrammar) {
        TMockContext ctx;

        for (const auto client: {"ru.yandex.quasar", "ru.yandex.searchplugin"}){
            const auto clientFeatures = GetClientFeaturesFor(client);
            const TLocalExpFlags expFlags;
            EXPECT_CALL(ctx, ClientFeatures()).WillRepeatedly(ReturnRef(clientFeatures));
            EXPECT_CALL(ctx, ExpFlags()).WillRepeatedly(ReturnRef(expFlags));

            auto wizardResponse = PrepareNewsWizardResponse("новости", 0);
            auto frames = PrepareNewsFrames(true);

            UNIT_ASSERT(!NImpl::ShouldFilterNewsScenario(ctx, scenarioNews, frames, wizardResponse));
        }
    }

    Y_UNIT_TEST(NotFilterFreeGrammarByClassifier) {
        TMockContext ctx;

        for (const auto client: {"ru.yandex.quasar", "ru.yandex.searchplugin"}){
            const auto clientFeatures = GetClientFeaturesFor(client);
            const TLocalExpFlags expFlags;
            EXPECT_CALL(ctx, ClientFeatures()).WillRepeatedly(ReturnRef(clientFeatures));
            EXPECT_CALL(ctx, ExpFlags()).WillRepeatedly(ReturnRef(expFlags));

            auto wizardResponse = PrepareNewsWizardResponse("теракт в бостоне", 0.97316);
            auto frames = PrepareNewsFrames(false);

            UNIT_ASSERT(!NImpl::ShouldFilterNewsScenario(ctx, scenarioNews, frames, wizardResponse));
        }
    }

    Y_UNIT_TEST(FilterFreeGrammarByClassifier) {
        TMockContext ctx;

        for (const auto client: {"ru.yandex.quasar", "ru.yandex.searchplugin"}){
            const auto clientFeatures = GetClientFeaturesFor(client);
            const TLocalExpFlags expFlags;
            EXPECT_CALL(ctx, ClientFeatures()).WillRepeatedly(ReturnRef(clientFeatures));
            EXPECT_CALL(ctx, ExpFlags()).WillRepeatedly(ReturnRef(expFlags));

            auto wizardResponse = PrepareNewsWizardResponse("танцы со звездами", 0.35152);
            auto frames = PrepareNewsFrames(false);

            UNIT_ASSERT(NImpl::ShouldFilterNewsScenario(ctx, scenarioNews, frames, wizardResponse));
        }
    }

    Y_UNIT_TEST(NotFilterFreeGrammarByClassifierWExperiment) {
        TMockContext ctx;

        for (const auto client: {"ru.yandex.quasar", "ru.yandex.searchplugin"}){
            const auto clientFeatures = GetClientFeaturesFor(client);
            const TLocalExpFlags expFlags {
                {TString{EXP_NEWS_DISABLE_CONFIDENCE_THRESHOLD}, "1"},
            };
            EXPECT_CALL(ctx, ClientFeatures()).WillRepeatedly(ReturnRef(clientFeatures));
            EXPECT_CALL(ctx, ExpFlags()).WillRepeatedly(ReturnRef(expFlags));

            auto wizardResponse = PrepareNewsWizardResponse("танцы со звездами", 0.35152);
            auto frames = PrepareNewsFrames(false);

            UNIT_ASSERT(!NImpl::ShouldFilterNewsScenario(ctx, scenarioNews, frames, wizardResponse));
        }
    }
}

Y_UNIT_TEST_SUITE(ShouldFilterSideSpeechScenario) {
    TTestScenario scenarioSideSpeech(SIDE_SPEECH_SCENARIO);

    Y_UNIT_TEST(AnotherScenario) {
        TMockContext ctx;
        const auto clientFeatures = GetClientFeaturesFor("ru.yandex.quasar");
        EXPECT_CALL(ctx, ClientFeatures()).WillRepeatedly(ReturnRef(clientFeatures));
        TTestScenario scenario(HARDCODED_RESPONSE_SCENARIO);

        UNIT_ASSERT(!NImpl::ShouldFilterSideSpeechScenario(ctx, scenario));
    }

    Y_UNIT_TEST(FilterSearchApp) {
        TMockContext ctx;
        const auto clientFeatures = GetClientFeaturesFor("ru.yandex.searchplugin");
        EXPECT_CALL(ctx, ClientFeatures()).WillRepeatedly(ReturnRef(clientFeatures));

        UNIT_ASSERT(NImpl::ShouldFilterSideSpeechScenario(ctx, scenarioSideSpeech));
    }

    Y_UNIT_TEST(FilterNavi) {
        TMockContext ctx;
        const auto clientFeatures = GetClientFeaturesFor("ru.yandex.mobile.navigator");
        EXPECT_CALL(ctx, ClientFeatures()).WillRepeatedly(ReturnRef(clientFeatures));

        UNIT_ASSERT(NImpl::ShouldFilterSideSpeechScenario(ctx, scenarioSideSpeech));
    }

    Y_UNIT_TEST(NotFilterQuasar) {
        TMockContext ctx;
        const auto clientFeatures = GetClientFeaturesFor("ru.yandex.quasar");
        EXPECT_CALL(ctx, ClientFeatures()).WillRepeatedly(ReturnRef(clientFeatures));

        UNIT_ASSERT(!NImpl::ShouldFilterSideSpeechScenario(ctx, scenarioSideSpeech));
    }
}

Y_UNIT_TEST_SUITE(CheckScenarioConfidentFrames) {
    using NAlice::NImpl::CheckScenarioConfidentFrames;
    const THashMap<TString, TMaybe<TString>> flags;

    Y_UNIT_TEST(NoConfig) {
        google::protobuf::Map<TString, NMegamind::TClassificationConfig::TScenarioConfig> configs;
        UNIT_ASSERT_VALUES_EQUAL(TString{}, CheckScenarioConfidentFrames(configs, CreateWhitelistWizardResponse(), "foo", flags));
    }

    Y_UNIT_TEST(NoMatchingFrames) {
        google::protobuf::Map<TString, NMegamind::TClassificationConfig::TScenarioConfig> configs;
        *configs["foo"].AddPreclassifierConfidentFrames() = "whitelist2";
        UNIT_ASSERT_VALUES_EQUAL(TString{}, CheckScenarioConfidentFrames(configs, CreateWhitelistWizardResponse(), "foo", flags));
    }

    Y_UNIT_TEST(MatchingFrames) {
        google::protobuf::Map<TString, NMegamind::TClassificationConfig::TScenarioConfig> configs;
        *configs["foo"].AddPreclassifierConfidentFrames() = "whitelist2";
        *configs["foo"].AddPreclassifierConfidentFrames() = "whitelist1";
        UNIT_ASSERT_VALUES_EQUAL("whitelist1", CheckScenarioConfidentFrames(configs, CreateWhitelistWizardResponse(), "foo", flags));
    }

    Y_UNIT_TEST(MatchedFrameFromExpFlag) {
        const THashMap<TString, TMaybe<TString>> overrideFlags = {{"mm_add_preclassifier_confident_frame_foo=whitelist1", "1"}};
        google::protobuf::Map<TString, NMegamind::TClassificationConfig::TScenarioConfig> configs;
        UNIT_ASSERT_VALUES_EQUAL("whitelist1", CheckScenarioConfidentFrames(configs, CreateWhitelistWizardResponse(), "foo", overrideFlags));
    }

    Y_UNIT_TEST(NoMatchedFrameFromExpFlag) {
        const THashMap<TString, TMaybe<TString>> overrideFlags = {{"mm_add_preclassifier_confident_frame_foo=whitelist2", "1"}};
        google::protobuf::Map<TString, NMegamind::TClassificationConfig::TScenarioConfig> configs;
        UNIT_ASSERT_VALUES_EQUAL(TString{}, CheckScenarioConfidentFrames(configs, CreateWhitelistWizardResponse(), "foo", overrideFlags));
    }
}

Y_UNIT_TEST_SUITE_F(ShouldSkipPreclassifierHint, TFixture) {
    const auto expBuilder = []() {
        return TStringBuilder{} << EXP_DISABLE_PRECLASSIFIER_HINT_PREFIX;
    };

    Y_UNIT_TEST(DisableHintForAllScenariosByExp) {
        InitDefaultExpects();
        EXPECT_CALL(Ctx, HasExpFlag(expBuilder() << "hint_name")).WillOnce(Return(true));
        EXPECT_CALL(Ctx, HasExpFlag(expBuilder() << "another_hint_name")).WillOnce(Return(false));
        EXPECT_CALL(Ctx, HasExpFlag(expBuilder() << "another_hint_name:any_scenario")).WillOnce(Return(false));
        UNIT_ASSERT(NImpl::ShouldSkipPreclassifierHint(Ctx, "any_scenario", "hint_name"));
        UNIT_ASSERT(!NImpl::ShouldSkipPreclassifierHint(Ctx, "any_scenario", "another_hint_name"));
    }
    Y_UNIT_TEST(DisableHintForOneScenarioByExp) {
        InitDefaultExpects();
        EXPECT_CALL(Ctx, HasExpFlag(expBuilder() << "hint_name")).Times(2).WillRepeatedly(Return(false));
        EXPECT_CALL(Ctx, HasExpFlag(expBuilder() << "hint_name:another_scenario_name")).WillOnce(Return(false));
        EXPECT_CALL(Ctx, HasExpFlag(expBuilder() << "another_hint_name")).WillOnce(Return(false));
        EXPECT_CALL(Ctx, HasExpFlag(expBuilder() << "another_hint_name:another_scenario_name")).WillOnce(Return(false));
        EXPECT_CALL(Ctx, HasExpFlag(expBuilder() << "hint_name:scenario_name")).WillOnce(Return(true));

        UNIT_ASSERT(!NImpl::ShouldSkipPreclassifierHint(Ctx, "another_scenario_name", "hint_name"));
        UNIT_ASSERT(!NImpl::ShouldSkipPreclassifierHint(Ctx, "another_scenario_name", "another_hint_name"));
        UNIT_ASSERT(NImpl::ShouldSkipPreclassifierHint(Ctx, "scenario_name", "hint_name"));
    }
}

Y_UNIT_TEST_SUITE_F(ShouldIgnorePreclassifierHint, TFixture) {
    Y_UNIT_TEST(HintIgnoredInConfig) {
        InitDefaultExpects();
        AddProtocolScenario("TestScenario1");
        AddProtocolScenario("TestScenario2");
        AddProtocolScenario("TestScenario3");

        NMegamind::TClassificationConfig::TScenarioConfig scenarioConfigWithHint;
        scenarioConfigWithHint.AddPreclassifierHint(TString{FAST_FRAME_NAME});

        NMegamind::TClassificationConfig::TScenarioConfig scenarioConfigIgnoreHint;
        scenarioConfigIgnoreHint.SetIgnorePreclassifierHints(true);

        NMegamind::TClassificationConfig config;
        auto& mp = *config.MutableScenarioClassificationConfigs();
        mp["TestScenario1"] = scenarioConfigWithHint;
        mp["TestScenario2"] = scenarioConfigIgnoreHint;

        EXPECT_CALL(Ctx, ClassificationConfig()).WillRepeatedly(ReturnRef(config));

        const TMockResponses responses;
        const auto wizardResponse = ParseBegemotResponseJsonToWizardResponse(TStringBuilder() << R"({
            "AliceResponse": {
                "AliceParsedFrames": {
                    "Frames": [
                        {
                            "Name": ")" << FAST_FRAME_NAME << R"("
                        }
                    ],
                    "Sources": [
                        "Granet"
                    ],
                    "Confidences": [
                        1.0
                    ]
                }
            }
        })");
        EXPECT_CALL(responses, WizardResponse(_)).WillRepeatedly(ReturnRef(wizardResponse));
        EXPECT_CALL(Ctx, Responses()).WillRepeatedly(ReturnRef(responses));

        {
            auto candidateToRequestFrames = MakeMapping(Registry.GetScenarioRefs());
            PreClassify(
                candidateToRequestFrames,
                Request,
                Ctx,
                FormulasStorage,
                FactorStorage,
                QualityStorage
            );
            CheckOnlyAllowed({"TestScenario1", "TestScenario2"}, candidateToRequestFrames);
        }
    }
}

Y_UNIT_TEST_SUITE(TestPreClassifyPartial) {
    Y_UNIT_TEST(TestDisabledWithoutFlag) {
        StrictMock<TMockContext> ctx;
        EXPECT_CALL(ctx, HasExpFlag(EXP_ENABLE_PARTIAL_PRECLASSIFIER)).WillOnce(Return(false));
        UNIT_ASSERT(!PreClassifyPartial(ctx, {}));
    }

    TFactorStorage GetFactorStorageExample() {
        // TODO(SEARCH-11113): this have to be changed once moved to preclassifier configs
        auto factorStorage = CreateStorage();
        TFactorView view = factorStorage.CreateViewFor(NFactorSlices::EFactorSlice::ALICE_BEGEMOT_QUERY_FACTORS);
        TVector<float> factorsExample = {
            0.000141292, 0.000409238, 4.44416e-06, 4.68631e-05, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0.711784, 0.366033, 8.81001e-07, 2.92859e-07, 0, 0,
            1.32602e-09, 0, 3.37644e-08, 4.6164e-07, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0
        };
        MemCopy(view.GetRawFactors(), factorsExample.data(), factorsExample.size());

        view = factorStorage.CreateViewFor(NFactorSlices::EFactorSlice::ALICE_DEVICE_STATE);
        factorsExample = {0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
        MemCopy(view.GetRawFactors(), factorsExample.data(), factorsExample.size());

        view = factorStorage.CreateViewFor(NFactorSlices::EFactorSlice::ALICE_ASR_FACTORS);
        factorsExample = {10, 2, 0, 0.6, 0.680557, 1, 7, 0, 2.75, 2.95359, 6, 10, 0.462963, 0.298418};
        MemCopy(view.GetRawFactors(), factorsExample.data(), factorsExample.size());
        return factorStorage;
    }

    TPartialPreCalcer GetCalcer() {
        TFileInput input(TFsPath(GetWorkPath()) / "partial_preclf_model.cbm");
        TFullModel model{};
        model.Load(&input);
        return TPartialPreCalcer(std::move(model));
    }

    Y_UNIT_TEST(TestWorkWithFlag) {
        StrictMock<TMockContext> ctx;

        EXPECT_CALL(ctx, Logger()).WillRepeatedly(ReturnRef(TRTLogger::NullLogger()));
        EXPECT_CALL(ctx, HasExpFlag(EXP_ENABLE_PARTIAL_PRECLASSIFIER)).WillRepeatedly(Return(true));

        const auto clientFeatures = GetClientFeaturesFor("aliced");
        EXPECT_CALL(ctx, ClientFeatures()).WillRepeatedly(ReturnRef(clientFeatures));

        const auto speechKitRequest =
            TSpeechKitRequestBuilder{TSpeechKitRequestBuilder::EPredefined::MinimalWithoutEvent}
                .SetProtoPatcher([&](NMegamind::TSpeechKitInitContext& ctx) {
                    ctx.EventProtoPtr->SetType(EEventType::voice_input);
                    ctx.EventProtoPtr->SetEndOfUtterance(false);
                }).Build();
        EXPECT_CALL(ctx, SpeechKitRequest()).WillRepeatedly(Return(speechKitRequest));

        const auto factorStorage = GetFactorStorageExample();

        auto begemotResponse = NBg::NProto::TAlicePolyglotMergeResponseResult();
        *begemotResponse.MutableAliceResponse()->MutableAlicePartialPreEmbedding()->MutableSerializedEmbedding() =
            "CuADjVsKvcDxOT3Y5Jk7ykkxvW2t8z39vpo9cSGyPaJ8kD0WZke9+mbdvGoFx721nLa979Mhu2BWrj2WR269hCsBPEgLnDzSkBM"
            "+4/YOPlW8hz2ZI0E+zUVHvVlsLL3PotW9JA8TvapyEz35qzO9vGOaPDZ1FT7UuOq8oxk6PKtWwj1VLdG8iFMOPF5GDDxBZAY9IR"
            "cMvRKYyj0F0MU9Alq0PQq8prx8f307DuYbPf/Hp7t3iOy8IdaSuhDDvjyu85k90utDvas2cjzexOc9G7ojvncx7D3R7Ym9Pgxnv"
            "b6YIjsFa8Q8zbjSPR88wjvOCpg9CLukvYzUZzwQxZQ9GyGavLqLgz1XvLw8JUg4Pfbv6zyljHM8BGhZu22KTjwGB9K8SxkZPcNO"
            "hTx2naM8IX2KvjhdpL39tYO9fQBgPQb0371laqw92aqVOvcOXD1OiRg+I2ccPKynm7zjwU+9ENKFPY0amry8pLi7M6X+vAlrID8"
            "HVhO9xyJrvJeedL3yhIy9Ote+OrhUcD2qdTy7qnXLvWlHyDy6WUm9jAvGvG86vT3bEhc9FbsvvU6igLwWv9g8QsvsvBNVgj0Lp1"
            "w+ezLEvFqNv7wkiqA90XQpu7mltb2jqLg9zNbrvFbXGz3J6c29";
        TMockResponses responses;
        responses.SetWizardResponse(TWizardResponse(std::move(begemotResponse)));
        EXPECT_CALL(ctx, Responses()).WillRepeatedly(ReturnRef(responses));

        const TPartialPreCalcer calcer = GetCalcer();
        EXPECT_CALL(ctx, GetPartialPreClassificationCalcer()).WillRepeatedly(ReturnRef(calcer));

        TMockSensors sensors{};
        EXPECT_CALL(ctx, Sensors()).WillRepeatedly(ReturnRef(sensors));

        {
            // set threshold so request will be classified as partial

            EXPECT_CALL(sensors, IncRate(NMonitoring::TLabels{
                {NSignal::CLIENT_TYPE, NSignal::CLIENT_TYPE_SMART_SPEAKER},
                {NSignal::NAME, "partial_preclassifier.preclassifications_per_second"},
                {NSignal::STATUS, "filtered"},
            })).Times(1);

            THashMap<TString, TMaybe<TString>> expFlags;
            expFlags[TString::Join(EXP_PARTIAL_PRECLASSIFIER_THRESHOLD_PREFIX, "0.5")] = "1";
            EXPECT_CALL(ctx, ExpFlags()).WillRepeatedly(ReturnRef(expFlags));

            UNIT_ASSERT(PreClassifyPartial(ctx, factorStorage));
        }
        {
            // set threshold so request will be classified as NOT partial

            EXPECT_CALL(sensors, IncRate(NMonitoring::TLabels{
                {NSignal::CLIENT_TYPE, NSignal::CLIENT_TYPE_SMART_SPEAKER},
                {NSignal::NAME, "partial_preclassifier.preclassifications_per_second"},
                {NSignal::STATUS, "left"},
            })).Times(1);

            THashMap<TString, TMaybe<TString>> expFlags;
            expFlags[TString::Join(EXP_PARTIAL_PRECLASSIFIER_THRESHOLD_PREFIX, "0.3")] = "1";
            EXPECT_CALL(ctx, ExpFlags()).WillRepeatedly(ReturnRef(expFlags));

            UNIT_ASSERT(!PreClassifyPartial(ctx, factorStorage));
        }
    }

    Y_UNIT_TEST(TestErrorHandling) {
        StrictMock<TMockContext> ctx;

        EXPECT_CALL(ctx, Logger()).WillRepeatedly(ReturnRef(TRTLogger::NullLogger()));
        EXPECT_CALL(ctx, HasExpFlag(EXP_ENABLE_PARTIAL_PRECLASSIFIER)).WillRepeatedly(Return(true));

        const auto clientFeatures = GetClientFeaturesFor("aliced");
        EXPECT_CALL(ctx, ClientFeatures()).WillRepeatedly(ReturnRef(clientFeatures));

        const auto speechKitRequest =
            TSpeechKitRequestBuilder{TSpeechKitRequestBuilder::EPredefined::MinimalWithoutEvent}
                .SetProtoPatcher([&](NMegamind::TSpeechKitInitContext& ctx) {
                    ctx.EventProtoPtr->SetType(EEventType::voice_input);
                    ctx.EventProtoPtr->SetEndOfUtterance(false);
                }).Build();
        EXPECT_CALL(ctx, SpeechKitRequest()).WillRepeatedly(Return(speechKitRequest));

        const auto factorStorage = GetFactorStorageExample();

        // No embedding from begemot. There will be an error
        TMockResponses responses;
        responses.SetWizardResponse(TWizardResponse());
        EXPECT_CALL(ctx, Responses()).WillRepeatedly(ReturnRef(responses));

        const TPartialPreCalcer calcer = GetCalcer();
        EXPECT_CALL(ctx, GetPartialPreClassificationCalcer()).WillRepeatedly(ReturnRef(calcer));

        TMockSensors sensors{};
        EXPECT_CALL(ctx, Sensors()).WillRepeatedly(ReturnRef(sensors));

        EXPECT_CALL(sensors, IncRate(NMonitoring::TLabels{
            {NSignal::CLIENT_TYPE, NSignal::CLIENT_TYPE_SMART_SPEAKER},
            {NSignal::NAME, "partial_preclassifier.errors"},
        })).Times(1);

        EXPECT_CALL(sensors, IncRate(NMonitoring::TLabels{
            {NSignal::CLIENT_TYPE, NSignal::CLIENT_TYPE_SMART_SPEAKER},
            {NSignal::NAME, "partial_preclassifier.preclassifications_per_second"},
            {NSignal::STATUS, "left"},
        })).Times(1);

        UNIT_ASSERT(!PreClassifyPartial(ctx, factorStorage));
    }

    Y_UNIT_TEST(TestWorkOnlyOnSupportedDevices) {
            StrictMock<TMockContext> ctx;

            EXPECT_CALL(ctx, Logger()).WillRepeatedly(ReturnRef(TRTLogger::NullLogger()));
            EXPECT_CALL(ctx, HasExpFlag(EXP_ENABLE_PARTIAL_PRECLASSIFIER)).WillOnce(Return(true));

            const auto clientFeatures = GetClientFeaturesFor("search_app_prod");
            EXPECT_CALL(ctx, ClientFeatures()).WillRepeatedly(ReturnRef(clientFeatures));

            const auto speechKitRequest =
                TSpeechKitRequestBuilder{TSpeechKitRequestBuilder::EPredefined::MinimalWithoutEvent}
                    .SetProtoPatcher([&](NMegamind::TSpeechKitInitContext& ctx) {
                        ctx.EventProtoPtr->SetType(EEventType::voice_input);
                        ctx.EventProtoPtr->SetEndOfUtterance(false);
                    }).Build();
            EXPECT_CALL(ctx, SpeechKitRequest()).WillRepeatedly(Return(speechKitRequest));

            TMockSensors sensors{};
            EXPECT_CALL(ctx, Sensors()).WillRepeatedly(ReturnRef(sensors));

            EXPECT_CALL(sensors, IncRate(NMonitoring::TLabels{
                {NSignal::CLIENT_TYPE, NSignal::CLIENT_TYPE_NON_SMART_SPEAKER},
                {NSignal::NAME, "partial_preclassifier.preclassifications_per_second"},
                {NSignal::STATUS, "skip"},
            })).Times(1);

            UNIT_ASSERT(!PreClassifyPartial(ctx, {}));
    }

    Y_UNIT_TEST(TestSkipPreclassificationForEOU) {
        StrictMock<TMockContext> ctx;

        EXPECT_CALL(ctx, Logger()).WillRepeatedly(ReturnRef(TRTLogger::NullLogger()));
        EXPECT_CALL(ctx, HasExpFlag(EXP_ENABLE_PARTIAL_PRECLASSIFIER)).WillOnce(Return(true));

        const auto clientFeatures = GetClientFeaturesFor("aliced");
        EXPECT_CALL(ctx, ClientFeatures()).WillRepeatedly(ReturnRef(clientFeatures));

        const auto speechKitRequest =
            TSpeechKitRequestBuilder{TSpeechKitRequestBuilder::EPredefined::MinimalWithoutEvent}
                .SetProtoPatcher([&](NMegamind::TSpeechKitInitContext& ctx) {
                    ctx.EventProtoPtr->SetType(EEventType::voice_input);
                    ctx.EventProtoPtr->SetEndOfUtterance(true);
                }).Build();
        EXPECT_CALL(ctx, SpeechKitRequest()).WillRepeatedly(Return(speechKitRequest));

        TMockSensors sensors{};
        EXPECT_CALL(ctx, Sensors()).WillRepeatedly(ReturnRef(sensors));

        EXPECT_CALL(sensors, IncRate(NMonitoring::TLabels{
            {NSignal::CLIENT_TYPE, NSignal::CLIENT_TYPE_SMART_SPEAKER},
            {NSignal::NAME, "partial_preclassifier.preclassifications_per_second"},
            {NSignal::STATUS, "skip"},
        })).Times(1);

        UNIT_ASSERT(!PreClassifyPartial(ctx, {}));
    }
}

} // namespace
