#include "modifier_fixture.h"

#include "fake_guid_generator.h"
#include "mock_context.h"
#include "mock_responses.h"
#include "speechkit.h"

#include <alice/megamind/library/models/directives/get_next_callback_directive_model.h>
#include <alice/megamind/library/proactivity/common/common.h>
#include <alice/megamind/library/response/response.h>

#include <alice/library/blackbox/blackbox.h>
#include <alice/library/frame/builder.h>
#include <alice/library/geo/user_location.h>
#include <alice/library/json/json.h>
#include <alice/library/metrics/names.h>
#include <alice/library/unittest/message_diff.h>
#include <alice/library/unittest/mock_sensors.h>

#include <kernel/geodb/countries.h>

#include <library/cpp/json/json_reader.h>
#include <library/cpp/testing/gmock_in_unittest/gmock.h>

namespace NAlice::NMegamind {

using namespace testing;
using TTestInput = TModifierTestFixture::TInput;

namespace {

constexpr TStringBuf DEFAULT_PHRASE = "Таков путь.";
constexpr ui64 PROACTIVITY_RANDOM_SEED = 6924683208236895711;


// Wrappers for tests ----------------------------------------------------------
template<typename T>
T MakeProto(const TStringBuf data) {
    return data ? JsonToProto<T>(NJson::ReadJsonFastTree(data)) : T{};
}

THolder<ISession> MakeSession(const TTestInput& input, TStringBuf actionName) {
    ::google::protobuf::Map<TString, NScenarios::TFrameAction> prevActions;
    prevActions[TString{actionName}] = NScenarios::TFrameAction{};
    TSessionProto::TScenarioSession prevScenario;
    *prevScenario.MutableState() = TState{};
    return MakeSessionBuilder()
        ->SetActions(prevActions)
        .SetPreviousScenarioName("prev")
        .SetScenarioSession("prev", prevScenario)
        .SetIntentName(input.IntentFromSession
            ? TString{input.Intent}
            : Default<TString>())
        .SetModifiersStorage(MakeProto<TModifiersStorage>(input.Storage))
        .Build();
}

NMementoApi::TRespGetAllObjects MakeMementoAllObjects(const TTestInput& input) {
    NMementoApi::TRespGetAllObjects result;
    *result.MutableUserConfigs() = MakeProto<NMementoApi::TUserConfigs>(input.UserConfigs);
    return result;
}

class TMockLightCtxWrapper {
public:
    TMockLightCtxWrapper(const TTestInput& input, TTestSpeechKitRequest skr)
        : SpeechKitRequest(skr)
        , UserLocation("ru", NGeoDB::MOSCOW_ID, "Europe/Moscow", NGeoDB::RUSSIA_ID)
        , Session(MakeSession(input, input.Action.Name))
        , MementoData(MakeMementoAllObjects(input))
    {
        Responses.SetPersonalIntentsResponse(
            NKvSaaS::TPersonalIntentsResponse(MakeProto<TPersonalIntentsRecord::TPersonalIntents>(input.PersonalIntents))
        );

        TBlackBoxFullUserInfoProto bbProto;
        TBlackBoxApi{}.ParseFullInfo(input.BlackBoxJson).MoveTo(bbProto);
        Responses.SetBlackBoxResponse(std::move(bbProto));

        EXPECT_CALL(Ctx, SpeechKitRequest()).WillRepeatedly(Return(SpeechKitRequest));
        EXPECT_CALL(Ctx, Responses()).WillRepeatedly(ReturnRef(Responses));
        EXPECT_CALL(Ctx, Session()).WillRepeatedly(Return(Session.Get()));
        EXPECT_CALL(Ctx, MementoData()).WillRepeatedly(ReturnRef(MementoData));
        EXPECT_CALL(Ctx, Sensors()).WillRepeatedly(ReturnRef(Sensors));
        EXPECT_CALL(Sensors, IncRate(_)).Times(AnyNumber());
    }

    TMockContext& Get() {
        return Ctx;
    }

    StrictMock<TMockSensors>& GetSensors() {
        return Sensors;
    }

private:
    TMockContext Ctx;
    TTestSpeechKitRequest SpeechKitRequest;
    TUserLocation UserLocation;
    TMockResponses Responses;
    THolder<ISession> Session;
    TMementoData MementoData;
    StrictMock<TMockSensors> Sensors;
};

TVector<TSemanticFrame> CreateFrames(TStringBuf name) {
    if (name.empty()) {
        return {};
    }
    return {TSemanticFrameBuilder{TString{name}}.Build()};
}

TVector<TSemanticFrame> CreateFrames(const TVector<TStringBuf>& framesJsons) {
    TVector<TSemanticFrame> frames;
    for (const auto frame : framesJsons) {
        frames.push_back(MakeProto<TSemanticFrame>(frame));
    }
    return frames;
}

TProactivityAnswer MakeProactivityAnswer(const TTestInput& input) {
    TProactivityAnswer result;
    for (const auto& skillRec : input.SkillRecs) {
        result.emplace_back(MakeProto<NDJ::NAS::TProactivityRecommendation>(skillRec));
    }
    return result;
}

class TModifierCtxWrapper {
public:
    TModifierCtxWrapper(const TTestInput& input, TTestSpeechKitRequest skr)
        : LightCtxWrapper(input, skr)
        , Storage(LightCtxWrapper.Get().Session()->GetModifiersStorage().GetRef())
        , SemanticFrames(CreateFrames(input.SemanticFrames))
        , RecognizedActionEffectFrames(CreateFrames(input.Action.FrameName))
        , SkillRec(MakeProactivityAnswer(input))
        , ModCtx(LightCtxWrapper.Get(), Storage, Info, LogStorage, SemanticFrames, RecognizedActionEffectFrames,
                 MegamindAnalyticsInfoBuilder, PROACTIVITY_RANDOM_SEED, SkillRec)
    {
    }

    TResponseModifierContext& Get() {
        return ModCtx;
    }

    TMockLightCtxWrapper& GetMockCtxWrapper() {
        return LightCtxWrapper;
    }

private:
    NMegamind::TMegamindAnalyticsInfoBuilder MegamindAnalyticsInfoBuilder;
    TMockLightCtxWrapper LightCtxWrapper;
    TModifiersStorage Storage;
    TModifiersInfo Info;
    TProactivityLogStorage LogStorage;
    TVector<TSemanticFrame> SemanticFrames;
    TVector<TSemanticFrame> RecognizedActionEffectFrames;
    TProactivityAnswer SkillRec;
    TResponseModifierContext ModCtx;
};

void SetIntent(const TTestInput& input, TScenarioResponse& response) {
    if (!input.IntentFromSession && !input.Intent.Empty()) {
        TFeatures features;
        features.MutableScenarioFeatures()->SetIntent(TString{input.Intent});
        response.SetFeatures(features);
    }
}

void AddStackEngineGetNext(NAlice::IResponseBuilder& builder) {
    const NAlice::NMegamind::TGetNextCallbackDirectiveModel directive(
        /* ignoreAnswer = */ false,
        /* isLedSilent = */ true,
        /* sessionId = */ "mega-session-id",
        /* productScenarioName = */ "ultra-scenario"
    );
    builder.AddDirective(directive);
}

TTestSpeechKitRequest BuildSpeechKitRequestWrapper(const TTestInput& input) {
    NJson::TJsonValue skrJson = NJson::ReadJsonFastTree(input.SkrJson);

    if (input.PersonalData.empty()) {
        return TSpeechKitRequestBuilder(skrJson).Build();
    }

    NJson::TJsonValue personalDataJson;
    personalDataJson[PROACTIVITY_HISTORY] = input.PersonalData;
    skrJson["request"]["raw_personal_data"] = JsonToString(personalDataJson);

    return TSpeechKitRequestBuilder(skrJson).Build();
}

// Default TScenarioResponse ---------------------------------------------------
TScenarioResponse DefaultResponse(const TStringBuf scenario,
                                  const TVector<TStringBuf>& framesJsons)
{
    const auto& frames = CreateFrames(framesJsons);
    TScenarioResponse response(TString{scenario}, frames, /* scenarioAcceptsAnyUtterance= */ true);
    return response;
}

TScenarioResponse DefaultResponseMM(const TTestInput& input, TTestSpeechKitRequest skr) {
    // If scenario is not specified, use intent as scenario
    const auto scenario = input.Scenario.Empty() ? input.Intent : input.Scenario;
    auto response = DefaultResponse(scenario, input.FramesJsons);
    auto& builder = response.ForceBuilder(skr, CreateRequest(IEvent::CreateEvent(skr.Event()), skr),
                          NMegamind::TFakeGuidGenerator{"GUID"})
                        .AddSimpleText(input.AddTextReponse ? TString{DEFAULT_PHRASE} : "",
                                       input.AddVoiceReponse ? TString{DEFAULT_PHRASE} : "")
                        .ShouldListen(input.ShouldListen);
    SetIntent(input, response);
    if (input.AddStackEngineGetNext) {
        AddStackEngineGetNext(builder);
    }
    return response;
}

// TestBasic -------------------------------------------------------------------
TMaybeNonApply TestBasic(
    TModifierCreator creator,
    TModifierCtxWrapper& ctx,
    TScenarioResponse& response
) {
    const TModifierPtr modifier = creator();
    const auto applyResult = modifier->TryApply(ctx.Get(), response);
    UNIT_ASSERT_C(applyResult.IsSuccess(), applyResult.Error()->ErrorMsg);
    return applyResult.Value();
}

TScenarioResponse TestBasicNonApply(
    TModifierCreator creator,
    TModifierCtxWrapper& ctx,
    const TNonApply::EType expectedType,
    const TStringBuf expectedReason,
    TScenarioResponse&& scenarioResponse
) {
    auto response = std::move(scenarioResponse);
    const auto nonApply = TestBasic(creator, ctx, response);
    UNIT_ASSERT_C(nonApply, "Apply is successful unexpectedly");
    UNIT_ASSERT_VALUES_EQUAL(nonApply->Type(), expectedType);
    if (expectedReason) {
        UNIT_ASSERT_STRINGS_EQUAL(nonApply->Reason(), TString{expectedReason});
    }
    return response;
}

TScenarioResponse TestBasicNonApplyMM(
    TModifierCreator creator,
    const TTestInput& input,
    TTestSpeechKitRequest skr,
    const TNonApply::EType expectedType,
    const TStringBuf expectedReason
) {
    TModifierCtxWrapper ctx(input, skr);
    return TestBasicNonApply(creator, ctx, expectedType, expectedReason, DefaultResponseMM(input, skr));
}

TScenarioResponse TestBasicApply(
    TModifierCreator creator,
    TModifierCtxWrapper& ctx,
    TScenarioResponse&& scenarioResponse
) {
    auto response = std::move(scenarioResponse);
    const auto nonApply = TestBasic(creator, ctx, response);
    UNIT_ASSERT_C(!nonApply, nonApply->Reason());
    return response;
}

TScenarioResponse TestBasicApplyMM(
    TModifierCreator creator,
    const TTestInput& input,
    TTestSpeechKitRequest skr
) {
    TModifierCtxWrapper ctx(input, skr);
    return TestBasicApply(creator, ctx, DefaultResponseMM(input, skr));
}

} // namespace

// TestExpected ----------------------------------------------------------------
void TModifierTestFixture::TestExpectedNonApply(
    TModifierCreator creator,
    const TTestInput& input,
    const TNonApply::EType expectedType,
    const TStringBuf expectedReason
) {
    auto skr = BuildSpeechKitRequestWrapper(input);
    TModifierCtxWrapper ctx(input, skr);

    TestBasicNonApply(creator, ctx, expectedType, expectedReason,
                      DefaultResponseMM(input, skr));
    UNIT_ASSERT_MESSAGES_EQUAL(ctx.Get().ProactivityLogStorage(), TProactivityLogStorage{});
}

void TModifierTestFixture::TestExpectedAction(
    TModifierCreator creator,
    const TTestInput& input,
    const TStringBuf expectedName,
    const TStringBuf expectedAction
) {
    auto skr = BuildSpeechKitRequestWrapper(input);
    const auto expectedJson = NJson::ReadJsonFastTree(expectedAction);

    NScenarios::TFrameAction expectedProto;
    const auto status = JsonToProto(expectedJson, expectedProto);
    UNIT_ASSERT_C(status.ok(), status.ToString());
    const auto response = TestBasicApplyMM(creator, input, skr);
    UNIT_ASSERT_MESSAGES_EQUAL(
        response.BuilderIfExists()->GetActions()[TString(expectedName)],
        expectedProto
    );
}

void TModifierTestFixture::TestExpectedResponse(
    TModifierCreator creator,
    const TTestInput& input,
    const TStringBuf expected
) {
    auto skr = BuildSpeechKitRequestWrapper(input);

    TSpeechKitResponseProto skExpected;
    const auto status = JsonToProto(NJson::ReadJsonFastTree(expected), skExpected);
    UNIT_ASSERT_C(status.ok(), status.ToString());
    const auto response = TestBasicApplyMM(creator, input, skr);
    UNIT_ASSERT_MESSAGES_EQUAL(
        response.BuilderIfExists()->GetSKRProto().GetResponse(),
        skExpected.GetResponse()
    );
    UNIT_ASSERT_MESSAGES_EQUAL(
        response.BuilderIfExists()->GetSKRProto().GetVoiceResponse(),
        skExpected.GetVoiceResponse()
    );
}

void TModifierTestFixture::TestExpectedResponseNonApply(
    TModifierCreator creator,
    const TTestInput& input,
    const TStringBuf expected,
    const TNonApply::EType expectedNonApplyType,
    const TStringBuf expectedNonApplyReason
) {
    auto skr = BuildSpeechKitRequestWrapper(input);

    TSpeechKitResponseProto skExpected;
    const auto status = JsonToProto(NJson::ReadJsonFastTree(expected), skExpected);
    UNIT_ASSERT_C(status.ok(), status.ToString());
    const auto response = TestBasicNonApplyMM(creator, input, skr, expectedNonApplyType, expectedNonApplyReason);
    UNIT_ASSERT_MESSAGES_EQUAL(
        response.BuilderIfExists()->GetSKRProto().GetResponse(),
        skExpected.GetResponse()
    );
    UNIT_ASSERT_MESSAGES_EQUAL(
        response.BuilderIfExists()->GetSKRProto().GetVoiceResponse(),
        skExpected.GetVoiceResponse()
    );
}

void TModifierTestFixture::TestExpectedTts(
    TModifierCreator creator,
    const TTestInput& input,
    const TStringBuf expected
) {
    auto skr = BuildSpeechKitRequestWrapper(input);
    UNIT_ASSERT_STRINGS_EQUAL(
        TestBasicApplyMM(creator, input, skr).BuilderIfExists()->GetRenderedSpeech(),
        expected
    );
}

void TModifierTestFixture::TestExpectedIncRateCall(
    TModifierCreator creator,
    const TTestInput& input,
    const TString expectedItemInfo,
    const TString expectedSource
) {
    auto skr = BuildSpeechKitRequestWrapper(input);
    TModifierCtxWrapper ctx(input, skr);
    EXPECT_CALL(ctx.GetMockCtxWrapper().GetSensors(), IncRate(NSignal::LabelsForPostrollItemInfo("View", expectedItemInfo)));
    EXPECT_CALL(ctx.GetMockCtxWrapper().GetSensors(), IncRate(NSignal::LabelsForPostrollSource("View", expectedSource)));
    TestBasicApply(creator, ctx, DefaultResponseMM(input, skr));
}

void TModifierTestFixture::TestExpectedTextAndVoice(
    TModifierCreator creator,
    const TTestInput& input,
    const TStringBuf expectedText,
    const TStringBuf expectedVoice
) {
    auto skr = BuildSpeechKitRequestWrapper(input);

    UNIT_ASSERT_STRINGS_EQUAL(
            TestBasicApplyMM(creator, input, skr).BuilderIfExists()->GetRenderedText(),
            expectedText
    );
    UNIT_ASSERT_STRINGS_EQUAL(
            TestBasicApplyMM(creator, input, skr).BuilderIfExists()->GetRenderedSpeech(),
            expectedVoice
    );
}

void TModifierTestFixture::TestExpectedLogStorage(
    TModifierCreator creator,
    const TTestInput& input,
    const TStringBuf expected
) {
    auto skr = BuildSpeechKitRequestWrapper(input);

    TModifierCtxWrapper ctx(input, skr);
    TestBasicApply(creator, ctx, DefaultResponseMM(input, skr));

    TProactivityLogStorage logStorageExpected;
    const auto status = ::google::protobuf::TextFormat::ParseFromString(ToString(expected), &logStorageExpected);
    UNIT_ASSERT_C(status, JsonStringFromProto(logStorageExpected));

    UNIT_ASSERT_MESSAGES_EQUAL(ctx.Get().ProactivityLogStorage(), logStorageExpected);
}

void TModifierTestFixture::TestExpectedLogStorageNonApply(
        TModifierCreator creator,
        const TTestInput& input,
        const TStringBuf expectedLog,
        const TNonApply::EType expectedNonApplyType,
        const TStringBuf expectedNonApplyReason
) {
    auto skr = BuildSpeechKitRequestWrapper(input);

    TModifierCtxWrapper ctx(input, skr);
    TestBasicNonApply(creator, ctx, expectedNonApplyType, expectedNonApplyReason, DefaultResponseMM(input, skr));

    TProactivityLogStorage logStorageExpected;
    const auto status = ::google::protobuf::TextFormat::ParseFromString(ToString(expectedLog), &logStorageExpected);
    UNIT_ASSERT_C(status, JsonStringFromProto(logStorageExpected));

    UNIT_ASSERT_MESSAGES_EQUAL(ctx.Get().ProactivityLogStorage(), logStorageExpected);
}

void TModifierTestFixture::TestPostrollAnalytics(
    TModifierCreator creator,
    const TInput& input,
    const TStringBuf expectedAnalyticsJson
) {
    auto skr = BuildSpeechKitRequestWrapper(input);
    TModifierCtxWrapper ctx(input, skr);
    TestBasicApply(creator, ctx, DefaultResponseMM(input, skr));
    UNIT_ASSERT_VALUES_EQUAL(ctx.Get().MegamindAnalyticsInfoBuilder().BuildJson(),
                             NJson::ReadJsonFastTree(expectedAnalyticsJson));
}

void TModifierTestFixture::TestExpectedProactivityInfo(
    TModifierCreator creator,
    const TInput& input,
    const TStringBuf expectedProactivityInfoStr
) {
    auto skr = BuildSpeechKitRequestWrapper(input);

    TModifierCtxWrapper ctx(input, skr);
    TestBasicApply(creator, ctx, DefaultResponseMM(input, skr));

    TProactivityInfo expectedProactivityInfo;
    const auto status = ::google::protobuf::TextFormat::ParseFromString(ToString(expectedProactivityInfoStr), &expectedProactivityInfo);
    UNIT_ASSERT_C(status, JsonStringFromProto(expectedProactivityInfo));

    UNIT_ASSERT_MESSAGES_EQUAL(ctx.Get().ModifiersInfo().GetProactivity(), expectedProactivityInfo);
}

} // NAlice::NMegamind
