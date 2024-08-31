#include "conjugator_modifier.h"

#include <library/cpp/testing/gmock_in_unittest/gmock.h>
#include <library/cpp/testing/unittest/registar.h>

#include <alice/hollywood/library/modifiers/testing/mock_external_source_request_collector.h>
#include <alice/hollywood/library/modifiers/testing/mock_modifier_context.h>
#include <alice/library/proto/proto.h>
#include <alice/megamind/protos/scenarios/request.pb.h>
#include <alice/megamind/protos/scenarios/response.pb.h>
#include <alice/protos/api/conjugator/api.pb.h>
#include <alice/protos/data/contextual_data.pb.h>
#include <alice/megamind/protos/analytics/modifiers/conjugator/conjugator.pb.h>

#include <alice/library/unittest/message_diff.h>

#include <apphost/lib/service_testing/service_testing.h>


namespace {

using namespace NAlice::NConjugator;
using namespace NAlice::NHollywood::NModifiers;
using namespace NAlice::NHollywood;
using namespace NAlice::NScenarios;
using namespace NAlice::NMegamind;
using namespace NAlice;
using namespace testing;

TConjugatorModifier CreateConjugatorModifier() {
    auto config = TConjugatableScenariosConfig();

    auto& languageConfig = *config.AddLanguageConfigs();
    languageConfig.SetLanguage(ELang::L_ARA);
    *languageConfig.AddProductScenarioNames() = "test_scenario";

    auto result = TConjugatorModifier();
    result.Configure(config);
    return result;
}

TConjugateRequest CreateConjugatorRequest() {
    return ParseProtoText<TConjugateRequest>(R"(
    Language: L_ARA
    UnconjugatedPhrases: [
        "first unconjugated test phrase",
        "second unconjugated test phrase"
    ]
)");
}

TConjugateResponse CreateConjugatorResponse() {
    return ParseProtoText<TConjugateResponse>(R"(
    ConjugatedPhrases: [
        "first conjugated test phrase",
        "second conjugated test phrase"
    ]
)");
}

TModifierBody CreateRequestModifierBody() {
    return ParseProtoText<TModifierBody>(R"(
    Layout {
        OutputSpeech: "<speaker lang=\"ar\" voice=\"test_voice\">first unconjugated test phrase"
        Cards: {
            Text: "first unconjugated test phrase"
        }
        Cards: {
            DivCard: { }
        }
        Cards: {
            TextWithButtons {
                Text: "second unconjugated test phrase"
            }
        }
    }
)");
}

TModifierBody CreateResponseModifierBody() {
    return ParseProtoText<TModifierBody>(R"(
    Layout {
        OutputSpeech: "<speaker lang=\"ar\" voice=\"test_voice\">first conjugated test phrase"
        Cards: {
            Text: "first conjugated test phrase"
        }
        Cards: {
            DivCard: { }
        }
        Cards: {
            TextWithButtons {
                Text: "second conjugated test phrase"
            }
        }
    }
)");
}

::NAlice::NModifiers::TAnalyticsInfo CreateResponseAnalyticsInfo() {
    return ParseProtoText<::NAlice::NModifiers::TAnalyticsInfo>(R"(
    Conjugator {
        ConjugatedPhrasesCount: 2
        IsOutputSpeechConjugated: 1
        ConjugatedCardsCount: 2
    }
)");
}

class TConjugatorModifierFixtureBase : public NUnitTest::TBaseFixture {
public:
    TConjugatorModifierFixtureBase() {
        EXPECT_CALL(ModifierCtx_, Logger()).WillRepeatedly(ReturnRef(TRTLogger::NullLogger()));
        EXPECT_CALL(ModifierCtx_, HasExpFlag(_)).WillRepeatedly(Return(false));
        EXPECT_CALL(ModifierCtx_, ExpFlags()).WillRepeatedly(ReturnRef(Default<TExpFlags>()));

        ModifierBody_ = CreateRequestModifierBody();
    }

    TMockModifierContext& ModifierCtx() {
        return ModifierCtx_;
    }

    TModifierBody& ModifierBody() {
        return ModifierBody_;
    }
private:
    StrictMock<TMockModifierContext> ModifierCtx_;
    TModifierBody ModifierBody_;
};

class TConjugatorModifierPrepareFixture : public TConjugatorModifierFixtureBase {
public:
    TConjugatorModifierPrepareFixture() {
        Features_.SetScenarioLanguage(ELang::L_ARA);
        Features_.SetProductScenarioName("test_scenario");
        EXPECT_CALL(ModifierCtx(), GetFeatures()).WillRepeatedly(ReturnRef(Features_));
    }

    TModifierFeatures& Features() {
        return Features_;
    }

    TMockExternalSourceRequestCollector& ExternalSourceRequestCollector() {
        return ExternalSourceRequestCollector_;
    }

    void RunPrepare() {
        CreateConjugatorModifier().Prepare(TModifierPrepareContext {
            ModifierCtx(),
            ModifierBody(),
            ExternalSourceRequestCollector_
        });
    }
private:
    TModifierFeatures Features_;
    StrictMock<TMockExternalSourceRequestCollector> ExternalSourceRequestCollector_;
};

class TConjugatorModifierApplyFixture : public TConjugatorModifierFixtureBase {
public:
    NAppHost::NService::TTestContext& AppHostCtx() {
        return AppHostCtx_;
    }

    std::tuple<TApplyResult, TModifierBody, ::NAlice::NModifiers::TAnalyticsInfo> RunApply() {
        auto bodyBuilder = TResponseBodyBuilder(TModifierBody(ModifierBody()));
        auto analyticsInfo = TModifierAnalyticsInfoBuilder();

        auto applyError = CreateConjugatorModifier().TryApply(TModifierApplyContext {
            ModifierCtx(),
            bodyBuilder,
            analyticsInfo,
            TExternalSourcesResponseRetriever(AppHostCtx())
        });

        return std::make_tuple(std::move(applyError), std::move(bodyBuilder).MoveProto(), std::move(analyticsInfo).MoveProto());
    }
private:
    NAppHost::NService::TTestContext AppHostCtx_;
};

Y_UNIT_TEST_SUITE_F(ConjugatorModifierPrepare, TConjugatorModifierPrepareFixture) {
    Y_UNIT_TEST(NonMatchingScenario) {
        Features().SetProductScenarioName("non_matching_scenario");
        EXPECT_CALL(ExternalSourceRequestCollector(), AddRequest(_, _)).Times(0);

        RunPrepare();
    }

    Y_UNIT_TEST(ResponseAlreadyConjugated) {
        Features().MutableContextualData()->MutableConjugator()->SetResponseConjugationStatus(
            ::NAlice::NData::TContextualData_TConjugator_EResponseConjugationStatus_Conjugated);
        EXPECT_CALL(ExternalSourceRequestCollector(), AddRequest(_, _)).Times(0);

        RunPrepare();
    }

    Y_UNIT_TEST(ResponseConjugationStatusFeaturePriority) {
        Features().MutableContextualData()->MutableConjugator()->SetResponseConjugationStatus(
            ::NAlice::NData::TContextualData_TConjugator_EResponseConjugationStatus_Unconjugated);
        EXPECT_CALL(ExternalSourceRequestCollector(), AddRequest(_, _)).Times(1);

        RunPrepare();
    }

    Y_UNIT_TEST(NonMatchingScenarioLanguage) {
        Features().SetScenarioLanguage(ELang::L_RUS);
        EXPECT_CALL(ExternalSourceRequestCollector(), AddRequest(_, _)).Times(0);

        RunPrepare();
    }

    Y_UNIT_TEST(NonMatchingResponseFeatureLanguage) {
        Features().MutableContextualData()->SetResponseLanguage(ELang::L_RUS);
        EXPECT_CALL(ExternalSourceRequestCollector(), AddRequest(_, _)).Times(0);

        RunPrepare();
    }

    Y_UNIT_TEST(ResponseFeatureLanguagePriority) {
        Features().SetScenarioLanguage(ELang::L_RUS);
        Features().MutableContextualData()->SetResponseLanguage(ELang::L_ARA);

        EXPECT_CALL(ExternalSourceRequestCollector(), AddRequest(_, _)).Times(1);

        RunPrepare();
    }

    Y_UNIT_TEST(EmptyPhrases) {
        ModifierBody().Clear();
        EXPECT_CALL(ExternalSourceRequestCollector(), AddRequest(_, _)).Times(0);

        RunPrepare();

        ModifierBody().MutableLayout()->SetOutputSpeech("");
        ModifierBody().MutableLayout()->AddCards()->SetText("");
        ModifierBody().MutableLayout()->AddCards();

        EXPECT_CALL(ExternalSourceRequestCollector(), AddRequest(_, _)).Times(0);

        RunPrepare();
    }

    Y_UNIT_TEST(DuplicatedPhrase) {
        EXPECT_CALL(ExternalSourceRequestCollector(), AddRequest(_, _)).WillOnce([](const google::protobuf::Message& item, const TStringBuf type) {
            UNIT_ASSERT_VALUES_EQUAL(type, AR_CONJUGATOR_REQUEST_ITEM_NAME);
            UNIT_ASSERT_MESSAGES_EQUAL(item, CreateConjugatorRequest());
        });

        RunPrepare();
    }

    Y_UNIT_TEST(DifferentSpeechAndCards) {
        ModifierBody().MutableLayout()->MutableCards(0)->Clear();

        EXPECT_CALL(ExternalSourceRequestCollector(), AddRequest(_, _)).WillOnce([](const google::protobuf::Message& item, const TStringBuf type) {
            UNIT_ASSERT_VALUES_EQUAL(type, AR_CONJUGATOR_REQUEST_ITEM_NAME);
            UNIT_ASSERT_MESSAGES_EQUAL(item, CreateConjugatorRequest());
        });

        RunPrepare();
    }
}

Y_UNIT_TEST_SUITE_F(ConjugatorModifierApply, TConjugatorModifierApplyFixture) {
    Y_UNIT_TEST(NoConjugatorRequest) {
        const auto [applyError, modifierBody, analyticsInfo] = RunApply();
        UNIT_ASSERT_C(applyError.Defined(), "Conjugator modifier must not be applied without conjugator request in context");
    }

    Y_UNIT_TEST(NoConjugatorResponse) {
        AppHostCtx().AddProtobufItem(CreateConjugatorRequest(), CONJUGATOR_REQUEST_ITEM_NAME);
        UNIT_ASSERT_EXCEPTION_C(RunApply(), yexception, "Conjugator modifier must throw without conjugator response in context");
    }

    Y_UNIT_TEST(WrongConjugatorResponse) {
        AppHostCtx().AddProtobufItem(CreateConjugatorRequest(), CONJUGATOR_REQUEST_ITEM_NAME);

        auto conjugatorResponse = CreateConjugatorResponse();
        conjugatorResponse.MutableConjugatedPhrases()->RemoveLast();
        AppHostCtx().AddProtobufItem(conjugatorResponse, CONJUGATOR_RESPONSE_ITEM_NAME);
        UNIT_ASSERT_EXCEPTION_C(RunApply(), yexception, "Conjugator modifier must throw on wrong conjugator response");
    }

    Y_UNIT_TEST(SimpleApply) {
        AppHostCtx().AddProtobufItem(CreateConjugatorRequest(), CONJUGATOR_REQUEST_ITEM_NAME);
        AppHostCtx().AddProtobufItem(CreateConjugatorResponse(), CONJUGATOR_RESPONSE_ITEM_NAME);

        const auto [applyError, modifierBody, analyticsInfo] = RunApply();

        UNIT_ASSERT(applyError.Empty());
        UNIT_ASSERT_MESSAGES_EQUAL(modifierBody, CreateResponseModifierBody());
        UNIT_ASSERT_MESSAGES_EQUAL(analyticsInfo, CreateResponseAnalyticsInfo());
    }

    Y_UNIT_TEST(OnlyCards) {
        AppHostCtx().AddProtobufItem(CreateConjugatorRequest(), CONJUGATOR_REQUEST_ITEM_NAME);
        AppHostCtx().AddProtobufItem(CreateConjugatorResponse(), CONJUGATOR_RESPONSE_ITEM_NAME);

        ModifierBody().MutableLayout()->ClearOutputSpeech();

        const auto [applyError, modifierBody, analyticsInfo] = RunApply();

        UNIT_ASSERT(applyError.Empty());

        auto expectedModiferBody = CreateResponseModifierBody();
        expectedModiferBody.MutableLayout()->ClearOutputSpeech();
        UNIT_ASSERT_MESSAGES_EQUAL(modifierBody, expectedModiferBody);

        auto expectedAnalyticsInfo = CreateResponseAnalyticsInfo();
        expectedAnalyticsInfo.MutableConjugator()->SetIsOutputSpeechConjugated(false);
        UNIT_ASSERT_MESSAGES_EQUAL(analyticsInfo, expectedAnalyticsInfo);
    }
}

}
