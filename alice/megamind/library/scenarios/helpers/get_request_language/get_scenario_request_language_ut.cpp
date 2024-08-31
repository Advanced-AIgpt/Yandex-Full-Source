#include <alice/megamind/library/scenarios/helpers/get_request_language/get_scenario_request_language.h>
#include <alice/megamind/library/testing/mock_context.h>

#include <alice/protos/data/language/language.pb.h>

#include <library/cpp/testing/gmock_in_unittest/gmock.h>
#include <library/cpp/testing/unittest/registar.h>


using namespace NAlice;
using namespace NAlice::NMegamind;
using namespace testing;

namespace {

const TString SCENARIO_NAME = "test_scenario";

THolder<NAlice::TMockContext> CreateContext(const ELanguage language, const TVector<TString>& expFlags = {}) {
    auto context = MakeHolder<NAlice::TMockContext>();
    EXPECT_CALL(*context, Language()).WillRepeatedly(Return(language));

    EXPECT_CALL(*context, HasExpFlag(_)).WillRepeatedly(Return(false));
    for (const auto& expFlag : expFlags) {
        EXPECT_CALL(*context, HasExpFlag(expFlag)).WillRepeatedly(Return(true));
    }
    return context;
}

TScenarioConfig CreateConfig(const TVector<::NAlice::ELang> languages) {
    auto config = TScenarioConfig();
    config.SetName(SCENARIO_NAME);
    *config.MutableLanguages() = {languages.cbegin(), languages.cend()};
    return config;
}

} // namespace

Y_UNIT_TEST_SUITE(GetScenarioRequestLanguage) {
    Y_UNIT_TEST(TestSmokeRu) {
        const auto context = CreateContext(ELanguage::LANG_RUS);
        UNIT_ASSERT_EQUAL(GetScenarioRequestLanguage(
            CreateConfig({::NAlice::ELang::L_RUS}),
            *context
        ), ELanguage::LANG_RUS);
        UNIT_ASSERT_EQUAL(GetScenarioRequestLanguage(
            CreateConfig({::NAlice::ELang::L_RUS, ::NAlice::ELang::L_ARA}),
            *context
        ), ELanguage::LANG_RUS);
    }

    Y_UNIT_TEST(TestSmokeAr) {
        const auto context = CreateContext(ELanguage::LANG_ARA);
        UNIT_ASSERT_EQUAL(GetScenarioRequestLanguage(
            CreateConfig({::NAlice::ELang::L_RUS}),
            *context
        ), ELanguage::LANG_RUS);
        UNIT_ASSERT_EQUAL(GetScenarioRequestLanguage(
            CreateConfig({::NAlice::ELang::L_RUS, ::NAlice::ELang::L_ARA}),
            *context
        ), ELanguage::LANG_ARA);
        UNIT_ASSERT_EQUAL(GetScenarioRequestLanguage(
            CreateConfig({::NAlice::ELang::L_ARA}),
            *context
        ), ELanguage::LANG_ARA);
    }

    Y_UNIT_TEST(TestExpFlagForceLanguageForScenario) {
        const auto ruConfig = CreateConfig({::NAlice::ELang::L_RUS});
        const auto ruArConfig = CreateConfig({::NAlice::ELang::L_RUS, ::NAlice::ELang::L_ARA});
        const auto arConfig = CreateConfig({::NAlice::ELang::L_ARA});

        for (const auto& config : {ruConfig, ruArConfig, arConfig}) {
            UNIT_ASSERT_EQUAL(GetScenarioRequestLanguage(config,
                *CreateContext(ELanguage::LANG_ARA, {"mm_force_polyglot_language_for_scenario=test_scenario"})
            ), ELanguage::LANG_ARA);
            UNIT_ASSERT_EQUAL(GetScenarioRequestLanguage(config,
                *CreateContext(ELanguage::LANG_ARA, {"mm_force_translated_language_for_scenario=test_scenario"})
            ), ELanguage::LANG_RUS);
            UNIT_ASSERT_EQUAL(GetScenarioRequestLanguage(config,
                *CreateContext(ELanguage::LANG_ARA, {"mm_force_translated_language_for_scenarios", "mm_force_polyglot_language_for_scenario=test_scenario"})
            ), ELanguage::LANG_ARA);
        }
    }

    Y_UNIT_TEST(TestExpFlagForceLanguageForAllScenarios) {
        const auto context = CreateContext(ELanguage::LANG_ARA, {"mm_force_translated_language_for_all_scenarios"});

        UNIT_ASSERT_EQUAL(
            GetScenarioRequestLanguage(CreateConfig({::NAlice::ELang::L_RUS}), *context),
            ELanguage::LANG_RUS);
        UNIT_ASSERT_EQUAL(
            GetScenarioRequestLanguage(CreateConfig({::NAlice::ELang::L_RUS, ::NAlice::ELang::L_ARA}), *context),
            ELanguage::LANG_RUS);
    }
}
