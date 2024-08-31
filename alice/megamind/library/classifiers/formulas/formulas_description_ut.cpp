#include "formulas_description.h"

#include <alice/megamind/library/config/config.h>
#include <alice/megamind/library/scenarios/defs/names.h>
#include <alice/megamind/library/testing/utils.h>

#include <alice/library/logger/logger.h>
#include <alice/library/unittest/message_diff.h>

#include <alice/protos/data/language/language.pb.h>
#include <alice/megamind/protos/quality_storage/storage.pb.h>
#include <google/protobuf/wrappers.pb.h>

#include <library/cpp/langs/langs.h>
#include <library/cpp/protobuf/util/pb_io.h>
#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/testing/unittest/tests_data.h>

namespace {

using namespace NAlice;


TFormulasDescription GetRealDescription() {
    TConfig config = GetRealConfig();
    NMegamind::TClassificationConfig classificationConfig = GetRealClassificationConfig();
    const auto& scenarioClassificationConfigs = classificationConfig.GetScenarioClassificationConfigs();
    return TFormulasDescription{scenarioClassificationConfigs, config.GetFormulasPath(), TRTLogger::NullLogger()};
}


Y_UNIT_TEST_SUITE(FormulasDescription) {
    Y_UNIT_TEST(GetExistingFormulaNames) {
        const TFormulasDescription description = GetRealDescription();

        {
            TFormulaDescription expectedDescription;
            TFormulaKey key(std::move(*expectedDescription.MutableKey()));
            expectedDescription.SetFormulaName(TString{"866067.HollywoodMusic"});

            UNIT_ASSERT_MESSAGES_EQUAL(
                description.Lookup(HOLLYWOOD_MUSIC_SCENARIO, ECS_POST, ECT_SMART_SPEAKER, {}, LANG_RUS),
                expectedDescription);
        }


        TFormulaDescription expectedDescription;
        TFormulaKey key(std::move(*expectedDescription.MutableKey()));
        expectedDescription.SetFormulaName(TString{"866060.Vins"});
        UNIT_ASSERT_MESSAGES_EQUAL(
            description.Lookup(MM_PROTO_VINS_SCENARIO, ECS_POST, ECT_SMART_SPEAKER, {}, LANG_RUS),
            expectedDescription
        );
        // Test different client type
        expectedDescription.SetFormulaName(TString{"861763.Vins"});
        UNIT_ASSERT_MESSAGES_EQUAL(
            description.Lookup(MM_PROTO_VINS_SCENARIO, ECS_POST, ECT_TOUCH, {}, LANG_RUS),
            expectedDescription
        );
        // Test preclassifier threshold
        expectedDescription.SetFormulaName(TString{"866018.HollywoodMusic"});
        expectedDescription.MutableThreshold()->set_value(-2.403941958);
        expectedDescription.MutableConfidentThreshold()->set_value(1.247147554);
        UNIT_ASSERT_MESSAGES_EQUAL(
            description.Lookup(HOLLYWOOD_MUSIC_SCENARIO, ECS_PRE, ECT_SMART_SPEAKER, {}, LANG_RUS),
            expectedDescription
        );
    }

    Y_UNIT_TEST(GetNonExistingFormulaNameWithOtherLanguage) {
        const TFormulasDescription description = GetRealDescription();

        TFormulaDescription expectedDescription;
        TFormulaKey key(std::move(*expectedDescription.MutableKey()));

        expectedDescription.SetFormulaName(TString{"866018.HollywoodMusic"});
        expectedDescription.MutableThreshold()->set_value(-2.403941958);
        expectedDescription.MutableConfidentThreshold()->set_value(1.247147554);
        UNIT_ASSERT_MESSAGES_EQUAL(
            description.Lookup(HOLLYWOOD_MUSIC_SCENARIO, ECS_PRE, ECT_SMART_SPEAKER, {}, LANG_RUS),
            expectedDescription
        );

        expectedDescription.Clear();
        UNIT_ASSERT_MESSAGES_EQUAL(
            description.Lookup(HOLLYWOOD_MUSIC_SCENARIO, ECS_PRE, ECT_SMART_SPEAKER, {}, LANG_ARA),
            expectedDescription
        );
    }

    Y_UNIT_TEST(GetNonExistingFormulaName) {
        const TFormulasDescription description = GetRealDescription();

        TFormulaDescription expectedDescription;
        UNIT_ASSERT_MESSAGES_EQUAL(
            description.Lookup("scenario", ECS_PRE, ECT_SMART_SPEAKER, {}, LANG_RUS),
            expectedDescription
        );
    }

    Y_UNIT_TEST(AddFormula) {
        TFormulasDescription description;
        TFormulaDescription expectedDescription;

        UNIT_ASSERT_MESSAGES_EQUAL(
            description.Lookup("scenario", ECS_PRE, ECT_SMART_SPEAKER, {}, LANG_ARA),
            expectedDescription
        );

        TFormulaDescription formulaDescription;
        formulaDescription.MutableKey()->SetScenarioName("scenario");
        formulaDescription.MutableKey()->SetClassificationStage(ECS_PRE);
        formulaDescription.MutableKey()->SetClientType(ECT_SMART_SPEAKER);
        formulaDescription.MutableKey()->SetLanguage(L_ARA);
        formulaDescription.SetFormulaName("formula");
        formulaDescription.MutableThreshold()->set_value(2.5);
        description.AddFormula(formulaDescription);

        expectedDescription.SetFormulaName("formula");
        expectedDescription.MutableThreshold()->set_value(2.5);

        TFormulaKey key(std::move(*expectedDescription.MutableKey()));
        UNIT_ASSERT_MESSAGES_EQUAL(
            description.Lookup("scenario", ECS_PRE, ECT_SMART_SPEAKER, {}, LANG_ARA),
            expectedDescription
        );
    }
}

Y_UNIT_TEST_SUITE(SourceConfigMerge) {
    Y_UNIT_TEST(GetExistingFormulaName) {
        TConfig config = GetRealConfig();
        auto classificationConfig = GetRealClassificationConfig();
        const auto& scenarioClassificationConfigs = classificationConfig.GetScenarioClassificationConfigs();
        const auto& sourceConfigFolder = TFsPath(ArcadiaSourceRoot()) / "alice/megamind/library/classifiers/formulas/ut/test_correct_config";
        const TFormulasDescription description{scenarioClassificationConfigs, sourceConfigFolder, TRTLogger::NullLogger()};

        TFormulaDescription expectedDescription;
        TFormulaKey key(std::move(*expectedDescription.MutableKey()));
        expectedDescription.SetFormulaName(TString{"04032001.search"});
        expectedDescription.MutableThreshold()->set_value(-0.5);
        expectedDescription.MutableConfidentThreshold()->set_value(1.4);

        UNIT_ASSERT_MESSAGES_EQUAL(
            description.Lookup(MM_SEARCH_PROTOCOL_SCENARIO, ECS_PRE, ECT_SMART_SPEAKER, "super_duper_exp", LANG_RUS),
            expectedDescription
        );
    }

    Y_UNIT_TEST(GetNonExistingFormulaName) {
        TConfig config = GetRealConfig();
        auto classificationConfig = GetRealClassificationConfig();
        const auto& scenarioClassificationConfigs = classificationConfig.GetScenarioClassificationConfigs();
        const auto& sourceConfigFolder = TFsPath(ArcadiaSourceRoot()) / "alice/megamind/library/classifiers/formulas/ut/test_correct_config";
        const TFormulasDescription description{scenarioClassificationConfigs, sourceConfigFolder, TRTLogger::NullLogger()};
        TFormulaDescription expectedDescription;

        UNIT_ASSERT_MESSAGES_EQUAL(
            description.Lookup(MM_SEARCH_PROTOCOL_SCENARIO, ECS_PRE, ECT_SMART_SPEAKER, "non_existing_exp", LANG_RUS),
            expectedDescription
        );
    }

    Y_UNIT_TEST(GetFormulaFromNonExistingFile) {
        TConfig config = GetRealConfig();
        auto classificationConfig = GetRealClassificationConfig();
        const auto& scenarioClassificationConfigs = classificationConfig.GetScenarioClassificationConfigs();
        const auto& sourceConfigFolder = TFsPath(ArcadiaSourceRoot()) / "non/existing/folder";
        const TFormulasDescription description{scenarioClassificationConfigs, sourceConfigFolder, TRTLogger::NullLogger()};
        TFormulaDescription expectedDescription;

        UNIT_ASSERT_MESSAGES_EQUAL(
            description.Lookup(MM_SEARCH_PROTOCOL_SCENARIO, ECS_PRE, ECT_SMART_SPEAKER, "non_existing_exp", LANG_RUS),
            expectedDescription
        );
    }

    Y_UNIT_TEST(GetFormulaFromBadFile) {
        TConfig config = GetRealConfig();
        auto classificationConfig = GetRealClassificationConfig();
        const auto& scenarioClassificationConfigs = classificationConfig.GetScenarioClassificationConfigs();
        const auto& sourceConfigFolder = TFsPath(ArcadiaSourceRoot()) / "alice/megamind/library/classifiers/formulas/ut/test_bad_config";
        const TFormulasDescription description{scenarioClassificationConfigs, sourceConfigFolder, TRTLogger::NullLogger()};
        TFormulaDescription expectedDescription;

        UNIT_ASSERT_MESSAGES_EQUAL(
            description.Lookup(MM_SEARCH_PROTOCOL_SCENARIO, ECS_PRE, ECT_SMART_SPEAKER, "super_duper_exp", LANG_RUS),
            expectedDescription
        );
    }
}


} // namespace
