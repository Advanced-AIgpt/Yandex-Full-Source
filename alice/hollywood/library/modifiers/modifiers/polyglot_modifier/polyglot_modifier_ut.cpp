#include "polyglot_modifier.h"

#include <alice/hollywood/library/modifiers/modifiers/polyglot_modifier/proto/polyglot_translation_request.pb.h>
#include <alice/hollywood/library/modifiers/testing/mock_external_source_request_collector.h>
#include <alice/hollywood/library/modifiers/testing/mock_modifier_context.h>
#include <alice/protos/data/contextual_data.pb.h>
#include <alice/protos/data/language/language.pb.h>
#include <alice/library/unittest/message_diff.h>

#include <apphost/lib/proto_answers/http.pb.h>

#include <apphost/lib/service_testing/service_testing.h>

#include <library/cpp/testing/gmock_in_unittest/gmock.h>
#include <library/cpp/testing/unittest/registar.h>

using namespace testing;
using namespace NAlice;
using namespace NAlice::NHollywood;
using namespace NAlice::NHollywood::NModifiers;

namespace {

constexpr size_t ResponseBodyVoicePrefixSize = TStringBuf(R"(<speaker voice="arabic.gpu" lang="ar">)").size();

TModifierBody CreateRequestModifierBody() {
    return ParseProtoText<TModifierBody>(R"(
    Layout {
        OutputSpeech: "Привет, как дела?"
        Cards: {
            Text: "Привет, как дела?"
        }
    }
)");
}

TPolyglotTranslationRequest CreatePolyglotTranslationRequest() {
    return ParseProtoText<TPolyglotTranslationRequest>(R"(
    UntranslatedPhrases: "Привет, как дела?"
)");
}

NAppHostHttp::THttpResponse CreatePolyglotHttpResponse() {
    return ParseProtoText<NAppHostHttp::THttpResponse>(R"(
    StatusCode: 200
    Content: "{\"text\": [\"Hello, how are you?\"]}"
)");
}

TModifierBody CreateResponseModifierBody() {
    return ParseProtoText<TModifierBody>(R"(
    Layout {
        OutputSpeech: "<speaker voice=\"arabic.gpu\" lang=\"ar\">Hello, how are you?"
        Cards: {
            Text: "Hello, how are you?"
        }
    }
)");
}

::NAlice::NModifiers::TAnalyticsInfo CreateResponseAnalyticsInfo() {
    return ParseProtoText<::NAlice::NModifiers::TAnalyticsInfo>(R"(
    Polyglot {
        TranslatedPhrasesCount: 2
        UniqueTranslatedPhrasesCount: 1
    }
)");
}

void ValidatePolyglotHttpRequest(const google::protobuf::Message& item, const TStringBuf lang) {
    UNIT_ASSERT_VALUES_EQUAL(item.GetTypeName(), NAppHostHttp::THttpRequest().GetTypeName());
    if (typeid(item) != typeid(NAppHostHttp::THttpRequest)) {
        UNIT_ASSERT_C(false, "Expected to get THttpRequest for http request to translator");
    }

    const auto& httpRequest = dynamic_cast<const NAppHostHttp::THttpRequest&>(item);

    UNIT_ASSERT(httpRequest.GetPath().StartsWith('?'));

    const auto cgiParameters = TCgiParameters(httpRequest.GetPath().substr(1));
    UNIT_ASSERT_VALUES_EQUAL(cgiParameters.NumOfValues("lang"), 1);
    UNIT_ASSERT_VALUES_EQUAL(cgiParameters.Get("lang"), lang);
    UNIT_ASSERT_VALUES_EQUAL(cgiParameters.NumOfValues("text"), 1);
    UNIT_ASSERT_VALUES_EQUAL(cgiParameters.Get("text"), "Привет, как дела?");
}


class TPolyglotModifierFixtureBase : public NUnitTest::TBaseFixture {
public:
    TPolyglotModifierFixtureBase() {
        ModifierBaseRequest_.SetUserLanguage(ELang::L_ARA);

        EXPECT_CALL(ModifierCtx_, Logger()).WillRepeatedly(ReturnRef(TRTLogger::NullLogger()));
        EXPECT_CALL(ModifierCtx_, GetBaseRequest()).WillRepeatedly(ReturnRef(ModifierBaseRequest_));
        EXPECT_CALL(ModifierCtx_, ExpFlags()).WillRepeatedly(ReturnRef(ExpFlags_));
        EXPECT_CALL(ModifierCtx_, HasExpFlag(_)).WillRepeatedly([this](const TStringBuf key) {
            return ExpFlags_.contains(key);
        });

        ModifierBody_ = CreateRequestModifierBody();
    }

    TModifierBaseRequest& ModifierBaseRequest() {
        return ModifierBaseRequest_;
    }

    TExpFlags& ExpFlags() {
        return ExpFlags_;
    }

    TMockModifierContext& ModifierCtx() {
        return ModifierCtx_;
    }

    TModifierBody& ModifierBody() {
        return ModifierBody_;
    }
private:
    TModifierBaseRequest ModifierBaseRequest_;
    TExpFlags ExpFlags_;
    StrictMock<TMockModifierContext> ModifierCtx_;
    TModifierBody ModifierBody_;
};

class TPolyglotModifierPrepareFixture : public TPolyglotModifierFixtureBase {
public:
    TPolyglotModifierPrepareFixture() {
        Features_.SetScenarioLanguage(ELang::L_RUS);
        EXPECT_CALL(ModifierCtx(), GetFeatures()).WillRepeatedly(ReturnRef(Features_));
    }

    TModifierFeatures& Features() {
        return Features_;
    }

    TMockExternalSourceRequestCollector& ExternalSourceRequestCollector() {
        return ExternalSourceRequestCollector_;
    }

    void RunPrepare() {
        TPolyglotModifier().Prepare(TModifierPrepareContext {
            ModifierCtx(),
            ModifierBody(),
            ExternalSourceRequestCollector_
        });
    }
private:
    TModifierFeatures Features_;
    StrictMock<TMockExternalSourceRequestCollector> ExternalSourceRequestCollector_;
};


Y_UNIT_TEST_SUITE_F(PolyglotModifierPrepare, TPolyglotModifierPrepareFixture) {
    Y_UNIT_TEST(PrepareSkipByLangBothArOld) {
        Features().SetScenarioLanguage(ELang::L_ARA);

        EXPECT_CALL(ExternalSourceRequestCollector(), AddRequest(_, _)).Times(0);

        RunPrepare();
    }

    Y_UNIT_TEST(PrepareSkipByLangBothAr) {
        Features().MutableContextualData()->SetResponseLanguage(ELang::L_ARA);

        EXPECT_CALL(ExternalSourceRequestCollector(), AddRequest(_, _)).Times(0);

        RunPrepare();
    }

    Y_UNIT_TEST(ResponseFeaturePriority) {
        Features().SetScenarioLanguage(ELang::L_ARA);
        Features().MutableContextualData()->SetResponseLanguage(ELang::L_RUS);

        EXPECT_CALL(ExternalSourceRequestCollector(), AddRequest(_, AH_ITEM_POLYGLOT_REQUEST_NAME)).Times(1);
        EXPECT_CALL(ExternalSourceRequestCollector(), AddRequest(_, AH_ITEM_POLYGLOT_HTTP_REQUEST_NAME)).Times(1);

        RunPrepare();
    }

    Y_UNIT_TEST(PrepareSkipByLangBothRu) {
        ModifierBaseRequest().SetUserLanguage(ELang::L_RUS);

        EXPECT_CALL(ExternalSourceRequestCollector(), AddRequest(_, _)).Times(0);

        RunPrepare();
    }

    Y_UNIT_TEST(PrepareSkipByEmptyBody) {
        ModifierBody().MutableLayout()->SetOutputSpeech("");
        ModifierBody().MutableLayout()->MutableCards(0)->SetText("");

        EXPECT_CALL(ExternalSourceRequestCollector(), AddRequest(_, _)).Times(0);

        RunPrepare();
    }

    Y_UNIT_TEST(PrepareSkipByExp) {
        ExpFlags()["mm_disable_response_translation"] = "1";

        EXPECT_CALL(ExternalSourceRequestCollector(), AddRequest(_, _)).Times(0);

        RunPrepare();
    }

    Y_UNIT_TEST(PrepareByExp) {
        ExpFlags()["mm_response_translation=ru-en"] = "1";

        EXPECT_CALL(ExternalSourceRequestCollector(), AddRequest(_, AH_ITEM_POLYGLOT_REQUEST_NAME)).WillOnce(
            [](const google::protobuf::Message& item, const TStringBuf) {
            UNIT_ASSERT_MESSAGES_EQUAL(item, CreatePolyglotTranslationRequest());
        });
        EXPECT_CALL(ExternalSourceRequestCollector(), AddRequest(_, AH_ITEM_POLYGLOT_HTTP_REQUEST_NAME)).WillOnce(
            [](const google::protobuf::Message& item, const TStringBuf) {
            ValidatePolyglotHttpRequest(item, "ru-en");
        });

        RunPrepare();
    }

    Y_UNIT_TEST(PrepareSimple) {
        EXPECT_CALL(ExternalSourceRequestCollector(), AddRequest(_, AH_ITEM_POLYGLOT_REQUEST_NAME)).WillOnce(
            [](const google::protobuf::Message& item, const TStringBuf) {
            UNIT_ASSERT_MESSAGES_EQUAL(item, CreatePolyglotTranslationRequest());
        });
        EXPECT_CALL(ExternalSourceRequestCollector(), AddRequest(_, AH_ITEM_POLYGLOT_HTTP_REQUEST_NAME)).WillOnce(
            [](const google::protobuf::Message& item, const TStringBuf) {
            ValidatePolyglotHttpRequest(item, "ru-ar");
        });

        RunPrepare();
    }
}


class TPolyglotModifierApplyFixture : public TPolyglotModifierFixtureBase {
public:
    NAppHost::NService::TTestContext& AppHostCtx() {
        return AppHostCtx_;
    }

    std::tuple<TApplyResult, TModifierBody, ::NAlice::NModifiers::TAnalyticsInfo> RunApply() {
        auto bodyBuilder = TResponseBodyBuilder(TModifierBody(ModifierBody()));
        auto analyticsInfo = TModifierAnalyticsInfoBuilder();

        auto applyError = TPolyglotModifier().TryApply(TModifierApplyContext {
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


Y_UNIT_TEST_SUITE_F(PolyglotModifierApply, TPolyglotModifierApplyFixture) {
    Y_UNIT_TEST(ApplySkipWithoutRequest) {
        const auto [applyError, modifierBody, analyticsInfo] = RunApply();
        UNIT_ASSERT(applyError.Defined());
    }

    Y_UNIT_TEST(ApplyErrorWithoutHttpResponse) {
        AppHostCtx().AddProtobufItem(CreatePolyglotTranslationRequest(), AH_ITEM_POLYGLOT_REQUEST_NAME);
        UNIT_ASSERT_EXCEPTION(RunApply(), yexception);
    }

    Y_UNIT_TEST(ApplyErrorHttpResponseErrorCode) {
        AppHostCtx().AddProtobufItem(CreatePolyglotTranslationRequest(), AH_ITEM_POLYGLOT_REQUEST_NAME);

        auto httpResponse = CreatePolyglotHttpResponse();
        httpResponse.SetStatusCode(500);
        AppHostCtx().AddProtobufItem(httpResponse, AH_ITEM_POLYGLOT_HTTP_RESPONSE_NAME);

        UNIT_ASSERT_EXCEPTION(RunApply(), yexception);
    }

    Y_UNIT_TEST(ApplyWithVoiceExp) {
        ExpFlags()["mm_polyglot_voice_prefix=PHNwZWFrZXIgdm9pY2U9InRlc3Rfdm9pY2UiIGxhbmc9ImFyIj4="] = "1";

        AppHostCtx().AddProtobufItem(CreatePolyglotTranslationRequest(), AH_ITEM_POLYGLOT_REQUEST_NAME);
        AppHostCtx().AddProtobufItem(CreatePolyglotHttpResponse(), AH_ITEM_POLYGLOT_HTTP_RESPONSE_NAME);

        const auto [applyError, modifierBody, analyticsInfo] = RunApply();
        UNIT_ASSERT(applyError.Empty());

        auto expectedModifierBody = CreateResponseModifierBody();
        expectedModifierBody.MutableLayout()->MutableOutputSpeech()->remove(0, ResponseBodyVoicePrefixSize);
        expectedModifierBody.MutableLayout()->MutableOutputSpeech()->prepend(R"(<speaker voice="test_voice" lang="ar">)");

        UNIT_ASSERT_MESSAGES_EQUAL(modifierBody, expectedModifierBody);
        UNIT_ASSERT_MESSAGES_EQUAL(analyticsInfo, CreateResponseAnalyticsInfo());
    }

    Y_UNIT_TEST(ApplySimpleEn) {
        ModifierBaseRequest().SetUserLanguage(ELang::L_ENG);

        AppHostCtx().AddProtobufItem(CreatePolyglotTranslationRequest(), AH_ITEM_POLYGLOT_REQUEST_NAME);
        AppHostCtx().AddProtobufItem(CreatePolyglotHttpResponse(), AH_ITEM_POLYGLOT_HTTP_RESPONSE_NAME);

        const auto [applyError, modifierBody, analyticsInfo] = RunApply();
        UNIT_ASSERT(applyError.Empty());

        auto expectedModifierBody = CreateResponseModifierBody();
        expectedModifierBody.MutableLayout()->MutableOutputSpeech()->remove(0, ResponseBodyVoicePrefixSize);

        UNIT_ASSERT_MESSAGES_EQUAL(modifierBody, expectedModifierBody);
        UNIT_ASSERT_MESSAGES_EQUAL(analyticsInfo, CreateResponseAnalyticsInfo());
    }

    Y_UNIT_TEST(ApplySimpleAr) {
        AppHostCtx().AddProtobufItem(CreatePolyglotTranslationRequest(), AH_ITEM_POLYGLOT_REQUEST_NAME);
        AppHostCtx().AddProtobufItem(CreatePolyglotHttpResponse(), AH_ITEM_POLYGLOT_HTTP_RESPONSE_NAME);

        const auto [applyError, modifierBody, analyticsInfo] = RunApply();
        UNIT_ASSERT(applyError.Empty());

        UNIT_ASSERT_MESSAGES_EQUAL(modifierBody, CreateResponseModifierBody());
        UNIT_ASSERT_MESSAGES_EQUAL(analyticsInfo, CreateResponseAnalyticsInfo());
    }

}

} // namespace
