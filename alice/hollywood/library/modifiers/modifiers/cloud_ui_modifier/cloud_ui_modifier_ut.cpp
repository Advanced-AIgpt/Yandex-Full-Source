#include "cloud_ui_modifier.h"

#include <library/cpp/testing/gmock_in_unittest/gmock.h>
#include <library/cpp/testing/unittest/registar.h>

#include <alice/hollywood/library/modifiers/testing/mock_modifier_context.h>
#include <alice/library/proto/proto.h>
#include <alice/megamind/protos/scenarios/request.pb.h>
#include <alice/megamind/protos/scenarios/response.pb.h>

#include <apphost/lib/service_testing/service_testing.h>

namespace {

using namespace NAlice::NHollywood::NModifiers;
using namespace NAlice::NScenarios;
using namespace NAlice;
using namespace testing;

constexpr TStringBuf FILL_CLOUD_UI_TEXT = "Sample text";

TModifierBody BuildModifierBody(const TMaybe<TString> uri = Nothing(), const bool hasFillCloudUiDirective = false) {
    constexpr TStringBuf MODIFIER_BODY_SIMPLE = R"(
        Layout {
            Cards {
                DivCard {
                }
            }
            OutputSpeech: "Продам гараж"
            Directives {
                TypeTextDirective {
                    Text: "Введи текст"
                }
            }
        }
    )";
    auto modifierBody = ParseProtoText<TModifierBody>(MODIFIER_BODY_SIMPLE);

    if (uri.Defined()) {
        modifierBody.MutableLayout()->AddDirectives()->MutableOpenUriDirective()->SetUri(*uri);
    }
    if (hasFillCloudUiDirective) {
        modifierBody.MutableLayout()->AddDirectives()->MutableFillCloudUiDirective()
            ->SetText(FILL_CLOUD_UI_TEXT.data(), FILL_CLOUD_UI_TEXT.size());
    }

    return modifierBody;
}

class TFixture : public NUnitTest::TBaseFixture {
public:
    TMockModifierContext& Ctx(bool supportsCloudUiFilling = true, bool hasFlag = false) {
        BaseRequest_.MutableInterfaces()->SetSupportsCloudUiFilling(supportsCloudUiFilling);
        EXPECT_CALL(Ctx_, GetBaseRequest()).WillRepeatedly(ReturnRef(BaseRequest_));
        EXPECT_CALL(Ctx_, HasExpFlag("mm_disable_cloud_ui_modifier")).WillRepeatedly(Return(hasFlag));
        return Ctx_;
    }

    TResponseBodyBuilder& BodyBuilder(TModifierBody&& modifierBody) {
        BodyBuilder_ = std::make_unique<TResponseBodyBuilder>(std::move(modifierBody));
        return *BodyBuilder_;
    }

    TModifierAnalyticsInfoBuilder& AnalyticsInfo() {
        return AnalyticsInfo_;
    }

    NAppHost::IServiceContext& ApphostContext() {
        return ApphostContext_;
    }

    const TFillCloudUiDirective* TryGetFillCloudUiDirective() {
        const auto& directives = BodyBuilder_->GetModifierBody().GetLayout().GetDirectives();
        for (const auto& d : directives) {
            if (d.HasFillCloudUiDirective()) {
                return &d.GetFillCloudUiDirective();
            }
        }
        return nullptr;
    }

    size_t NumberOfFillCloudUiDirectives() {
        const auto& directives = BodyBuilder_->GetModifierBody().GetLayout().GetDirectives();
        return CountIf(directives, [](const auto& d) {
            return d.HasFillCloudUiDirective();
        });
    }

private:
    TModifierBaseRequest BaseRequest_;
    TMockModifierContext Ctx_;
    std::unique_ptr<TResponseBodyBuilder> BodyBuilder_;
    TModifierAnalyticsInfoBuilder AnalyticsInfo_;
    NAppHost::NService::TTestContext ApphostContext_;
};

Y_UNIT_TEST_SUITE(CloudUiModifier) {
    Y_UNIT_TEST_F(TestDeviceDoesntSupportCloudUi, TFixture) {
        TCloudUiModifier modifier;
        const auto result = modifier.TryApply(TModifierApplyContext {
            Ctx(/* supportsCloudUiFilling = */ false, /* hasFlag = */ false),
            BodyBuilder(BuildModifierBody()),
            AnalyticsInfo(),
            TExternalSourcesResponseRetriever(ApphostContext())
        });
        UNIT_ASSERT(result.Defined());
        UNIT_ASSERT_VALUES_EQUAL(result->Type(), TNonApply::EType::NotApplicable);
    }

    Y_UNIT_TEST_F(TestNoFlag, TFixture) {
        TCloudUiModifier modifier;
        const auto result = modifier.TryApply(TModifierApplyContext {
            Ctx(/* supportsCloudUiFilling = */ true, /* hasFlag = */ true),
            BodyBuilder(BuildModifierBody()),
            AnalyticsInfo(),
            TExternalSourcesResponseRetriever(ApphostContext())
        });
        UNIT_ASSERT(result.Defined());
        UNIT_ASSERT_VALUES_EQUAL(result->Type(), TNonApply::EType::DisabledByFlag);
    }

    Y_UNIT_TEST_F(TestNoOpenUriDirective, TFixture) {
        TCloudUiModifier modifier;
        const auto result = modifier.TryApply(TModifierApplyContext {
            Ctx(),
            BodyBuilder(BuildModifierBody()),
            AnalyticsInfo(),
            TExternalSourcesResponseRetriever(ApphostContext())
        });
        UNIT_ASSERT(result.Defined());
        UNIT_ASSERT_VALUES_EQUAL(result->Type(), TNonApply::EType::NotApplicable);
    }

    Y_UNIT_TEST_F(TestStrangeOpenUriDirective, TFixture) {
        TCloudUiModifier modifier;
        const auto result = modifier.TryApply(TModifierApplyContext {
            Ctx(),
            BodyBuilder(BuildModifierBody(/* uri = */ "ftp://sparkle:12345@site.ru:8000/readme.txt")),
            AnalyticsInfo(),
            TExternalSourcesResponseRetriever(ApphostContext())
        });
        UNIT_ASSERT(result.Defined());
        UNIT_ASSERT_VALUES_EQUAL(result->Type(), TNonApply::EType::NotApplicable);
    }

    Y_UNIT_TEST_F(TestSmokeOpenUriDirective, TFixture) {
        TVector<std::pair<TString, TString>> CASES = {
            {"http://ru.wikipedia.org/", "Открываю"},
            {"musicsdk://?kind=3&owner=1083955728&play=true&repeat=repeatOff&shuffle=false", "Включаю"},
            {"viewport://?l10n=ru-RU&lr=213&noreask=1&query_source=alice&text=rammstein", "Вот что я нашла"},
        };

        for (const auto& [url, text] : CASES) {
            TCloudUiModifier modifier;
            const auto result = modifier.TryApply(TModifierApplyContext {
                Ctx(),
                BodyBuilder(BuildModifierBody(url)),
                AnalyticsInfo(),
                TExternalSourcesResponseRetriever(ApphostContext())
            });
            UNIT_ASSERT(!result.Defined());

            UNIT_ASSERT_VALUES_EQUAL(NumberOfFillCloudUiDirectives(), 1);

            const auto* fillCloudUiDirective = TryGetFillCloudUiDirective();
            UNIT_ASSERT(fillCloudUiDirective);
            UNIT_ASSERT_VALUES_EQUAL(fillCloudUiDirective->GetText(), text);
        }
    }

    Y_UNIT_TEST_F(TestExistingFillCloudUiDirective, TFixture) {
        TCloudUiModifier modifier;
        const auto result = modifier.TryApply(TModifierApplyContext {
            Ctx(),
            BodyBuilder(BuildModifierBody(/* uri = */ "http://ru.wikipedia.org/", /* hasFillCloudUiDirective = */ true)),
            AnalyticsInfo(),
            TExternalSourcesResponseRetriever(ApphostContext())
        });
        UNIT_ASSERT(result.Defined());
        UNIT_ASSERT_VALUES_EQUAL(result->Type(), TNonApply::EType::NotApplicable);
        UNIT_ASSERT_VALUES_EQUAL(NumberOfFillCloudUiDirectives(), 1);
    }
}

} // namespace
