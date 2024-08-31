#include "colored_speaker_modifier.h"

#include <alice/hollywood/library/modifiers/internal/config/proto/exact_key_groups.pb.h>
#include <alice/hollywood/library/modifiers/internal/config/proto/exact_mapping_config.pb.h>
#include <alice/hollywood/library/modifiers/matchers/exact_matcher.h>

#include <alice/hollywood/library/modifiers/testing/mock_modifier_context.h>

#include <alice/library/client/protos/promo_type.pb.h>
#include <alice/library/proto/proto.h>

#include <alice/megamind/protos/analytics/modifiers/colored_speaker/colored_speaker.pb.h>

#include <library/cpp/testing/common/env.h>
#include <library/cpp/testing/gmock_in_unittest/gmock.h>
#include <library/cpp/testing/unittest/registar.h>

#include <util/charset/utf8.h>
#include <util/generic/string.h>
#include <util/stream/file.h>

namespace {

const TString TEST_SCENARIO = "test_scenario";
const TString TEST_SCENARIO_2 = "test_scenario_2";

constexpr TStringBuf OLD_TEXT_1 = "СтАрЫй текст 1";
constexpr TStringBuf OLD_TEXT_2 = "старый текст 2";
constexpr TStringBuf OLD_TEXT_3 = "СтАрЫй текст 3";
constexpr TStringBuf OLD_TEXT_4 = "СтАрЫй текст 4";

constexpr TStringBuf GROUP_NAME_1 = "group_name_1";
constexpr TStringBuf GROUP_NAME_2 = "group_name_2";

constexpr TStringBuf NEW_TEXT_1 = "Новый текст 1";
constexpr TStringBuf NEW_TEXT_2 = "Новый текст 2";
constexpr TStringBuf NEW_TEXT_3 = "Новый текст 3";
constexpr TStringBuf NEW_TEXT_4 = "Новый текст 4";

constexpr TStringBuf NEW_TTS_1 = "Новый ттс 1";
constexpr TStringBuf NEW_TTS_2 = "Новый ттс 2";
constexpr TStringBuf NEW_TTS_3 = "Новый ттс 3";
constexpr TStringBuf NEW_TTS_4 = "Новый ттс 4";

using namespace testing;
using namespace NAlice;
using namespace NAlice::NHollywood;
using namespace NAlice::NHollywood::NModifiers;
using namespace NAlice::NHollywood::NModifiers::NImpl;
using namespace NAlice::NScenarios;

std::pair<TExactMappingConfig, TExactKeyGroups> GenerateConfig() {
    TExactMappingConfig config;
    TExactKeyGroups groups;
    {
        auto* group = groups.AddGroups();
        *group->AddPhrases() = OLD_TEXT_1;
        *group->AddPhrases() = OLD_TEXT_3;
        group->SetGroupName(TString{GROUP_NAME_1});
    }
    {
        auto* group = groups.AddGroups();
        group->SetGroupName(TString{GROUP_NAME_2});
        *group->AddPhrases() = OLD_TEXT_2;
        *group->AddPhrases() = OLD_TEXT_4;
    }
    {
        auto& rule = *config.AddMappings();
        *rule.MutableProductScenarioName() = TEST_SCENARIO;
        rule.SetDeviceColor(NClient::PT_BEIGE_PERSONALITY);
        *rule.MutableOldTtsGroupName() = GROUP_NAME_1;
        auto& newText = *rule.MutableNewTtsTextList();
        {
            auto* ttsAndText = newText.Add();
            *ttsAndText->MutableTts() = NEW_TTS_1;
            *ttsAndText->MutableText() = NEW_TEXT_1;
        }
        {
            auto* ttsAndText = newText.Add();
            *ttsAndText->MutableTts() = NEW_TTS_2;
            *ttsAndText->MutableText() = NEW_TEXT_2;
        }
    }
    {
        auto& rule = *config.AddMappings();
        *rule.MutableProductScenarioName() = TEST_SCENARIO_2;
        rule.SetDeviceColor(NClient::PT_RED_PERSONALITY);
        *rule.MutableOldTtsGroupName() = GROUP_NAME_2;
        auto& newText = *rule.MutableNewTtsTextList();
        {
            auto* ttsAndText = newText.Add();
            *ttsAndText->MutableTts() = NEW_TTS_3;
            *ttsAndText->MutableText() = NEW_TEXT_3;
        }
        {
            auto* ttsAndText = newText.Add();
            *ttsAndText->MutableTts() = NEW_TTS_4;
            *ttsAndText->MutableText() = NEW_TEXT_4;
        }
    }
    return std::make_pair(config, groups);
}

TExactMatcher GenerateMatcher(std::pair<TExactMappingConfig, TExactKeyGroups> configWithGroups) {
    TExactMatcher matcher{configWithGroups.first, configWithGroups.second};
    return matcher;
}

TExactMatcher GenerateMatcher() {
    return GenerateMatcher(GenerateConfig());
}

TModifierFeatures GenerateFeatures(const TString& productScenarioName, const NClient::EPromoType promoType) {
    TModifierFeatures features;
    *features.MutableProductScenarioName() = productScenarioName;
    features.SetPromoType(promoType);
    return features;
}

TModifierFeatures GenerateFeaturesNotApplicable(const TString& productScenarioName) {
    return GenerateFeatures(productScenarioName, NClient::PT_NO_TYPE);
}

constexpr TStringBuf TEST_MODIFIER_BODY = R"(
    Layout {
        Cards {
            Text: "какой-то текст"
        }
        OutputSpeech: "старЫй теКст 1"
        SuggestButtons {
            SearchButton {
                Title: "заголовок"
                Query: "запрос"
            }
        }
    }
)";

constexpr TStringBuf EXPECTED_MODIFIER_BODY = R"(
    Layout {
        Cards {
            Text: "Новый текст 2"
        }
        OutputSpeech: "Новый ттс 2"
        SuggestButtons {
            SearchButton {
                Title: "заголовок"
                Query: "запрос"
            }
        }
    }
)";

constexpr TStringBuf EXPECTED_ANALYTICS = R"(
    PrevResponseTts: "старЫй теКст 1"
    NewResponseTts: "Новый ттс 2"
)";

TModifierBody GenerateModifierBody(TStringBuf modifierBodyText) {
    return ParseProtoText<TModifierBody>(modifierBodyText);
}

class TFixture : public NUnitTest::TBaseFixture {
public:
    TFixture()
        : Rng_{4} {
        EXPECT_CALL(Ctx_, Rng()).WillRepeatedly(ReturnRef(Rng_));
    }

    TMockModifierContext& Ctx() {
        return Ctx_;
    }

private:
    TMockModifierContext Ctx_;
    TRng Rng_;
};

Y_UNIT_TEST_SUITE(ColoredSpeakerModifier) {
    Y_UNIT_TEST(TestFillMapping) {
        const auto config = GenerateConfig();

        TExactMatcher matcher{config.first, config.second};

        {
            auto* ptr = matcher.FindPtr(NClient::PT_BEIGE_PERSONALITY, TEST_SCENARIO, "старый текст 1");
            UNIT_ASSERT(ptr);
            UNIT_ASSERT_VALUES_EQUAL(ptr->size(), 2);
            UNIT_ASSERT_VALUES_EQUAL(ptr->begin()->Text, NEW_TEXT_1);
            UNIT_ASSERT_VALUES_EQUAL(ptr->begin()->Tts, NEW_TTS_1);
            UNIT_ASSERT_VALUES_EQUAL(std::next(ptr->begin())->Text, NEW_TEXT_2);
            UNIT_ASSERT_VALUES_EQUAL(std::next(ptr->begin())->Tts, NEW_TTS_2);
        }
        {
            auto* ptr = matcher.FindPtr(NClient::PT_RED_PERSONALITY, TEST_SCENARIO_2, "старый текст 2");
            UNIT_ASSERT(ptr);
            UNIT_ASSERT_VALUES_EQUAL(ptr->size(), 2);
            UNIT_ASSERT_VALUES_EQUAL(ptr->begin()->Text, NEW_TEXT_3);
            UNIT_ASSERT_VALUES_EQUAL(ptr->begin()->Tts, NEW_TTS_3);
            UNIT_ASSERT_VALUES_EQUAL(std::next(ptr->begin())->Text, NEW_TEXT_4);
            UNIT_ASSERT_VALUES_EQUAL(std::next(ptr->begin())->Tts, NEW_TTS_4);
        }
    }

    Y_UNIT_TEST_F(TestTryApply, TFixture) {
        auto& ctx = Ctx();
        const auto features = GenerateFeatures(TString{TEST_SCENARIO}, NClient::PT_BEIGE_PERSONALITY);

        EXPECT_CALL(ctx, GetFeatures()).WillRepeatedly(ReturnRef(features));

        TResponseBodyBuilder bodyBuilder{GenerateModifierBody(TEST_MODIFIER_BODY)};
        TModifierAnalyticsInfoBuilder analyticsInfoBuilder;
        const auto result = TryApplyImpl(ctx, bodyBuilder, analyticsInfoBuilder, GenerateMatcher());
        
        UNIT_ASSERT_C(!result, result->Reason());
        auto proto = std::move(bodyBuilder).MoveProto();
        UNIT_ASSERT_VALUES_EQUAL(SerializeProtoText(proto),
                                 SerializeProtoText(ParseProtoText<TModifierBody>(EXPECTED_MODIFIER_BODY)));
        
        const auto analyticsInfo = std::move(analyticsInfoBuilder).MoveProto().GetColoredSpeaker();
        UNIT_ASSERT_VALUES_EQUAL(
            SerializeProtoText(analyticsInfo),
            SerializeProtoText(
                ParseProtoText<NAlice::NModifiers::NColoredSpeaker::TColoredSpeaker>(EXPECTED_ANALYTICS)));
    }

    Y_UNIT_TEST_F(TestTryApplyNotApplicablePtNoType, TFixture) {
        auto& ctx = Ctx();
        const auto features = GenerateFeaturesNotApplicable(TString{TEST_SCENARIO});

        EXPECT_CALL(ctx, GetFeatures()).WillRepeatedly(ReturnRef(features));

        TResponseBodyBuilder bodyBuilder{GenerateModifierBody(TEST_MODIFIER_BODY)};
        TModifierAnalyticsInfoBuilder analyticsInfoBuilder;
        const auto result = TryApplyImpl(ctx, bodyBuilder, analyticsInfoBuilder, GenerateMatcher());

        UNIT_ASSERT(result);
        UNIT_ASSERT(!std::move(analyticsInfoBuilder).MoveProto().HasColoredSpeaker());
    }

    Y_UNIT_TEST_F(TestTryApplyNotApplicableByMapping, TFixture) {
        auto& ctx = Ctx();
        const auto features = GenerateFeatures(TString{TEST_SCENARIO}, NClient::PT_RED_PERSONALITY);

        EXPECT_CALL(ctx, GetFeatures()).WillRepeatedly(ReturnRef(features));

        TResponseBodyBuilder bodyBuilder{GenerateModifierBody(TEST_MODIFIER_BODY)};
        TModifierAnalyticsInfoBuilder analyticsInfoBuilder;
        const auto result = TryApplyImpl(ctx, bodyBuilder, analyticsInfoBuilder, GenerateMatcher());

        UNIT_ASSERT(result);
        UNIT_ASSERT(!std::move(analyticsInfoBuilder).MoveProto().HasColoredSpeaker());
    }
}

} // namespace
