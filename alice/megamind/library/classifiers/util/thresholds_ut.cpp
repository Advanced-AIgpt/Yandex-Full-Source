#include "thresholds.h"

#include <alice/megamind/protos/quality_storage/storage.pb.h>
#include <alice/protos/data/language/language.pb.h>

#include <google/protobuf/wrappers.pb.h>

#include <kernel/formula_storage/formula_storage.h>

#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/testing/unittest/tests_data.h>

using namespace NAlice;
using namespace NAlice::NMegamind;
using TLocalExpFlags = NMegamind::TClientComponent::TExpFlags;

namespace {

double GetConfidentThreshold(const TLocalExpFlags& expFlags, const TClassificationConfig& config, const TStringBuf scenario,
                             const EClientType clientType, const TMaybe<TFormulaDescription> formula)
{
    TFormulasDescription description;
    if (formula.Defined()) {
        description.AddFormula(*formula);
    }

    ::TFormulasStorage RawFormulasStorage;
    const auto& formulasPath = TFsPath(GetWorkPath()) / "formulas";
    RawFormulasStorage.AddFormulasFromDirectoryRecursive(formulasPath);

    NAlice::TFormulasStorage formulasStorage{RawFormulasStorage, description};

    return GetScenarioConfidentThreshold(
        formulasStorage,
        scenario,
        ECS_PRE,
        clientType,
        expFlags,
        config,
        TRTLogger::NullLogger(),
        LANG_RUS
    );
}

Y_UNIT_TEST_SUITE(GetConfidentScenarioThreshold) {
    Y_UNIT_TEST(WithPresentExp) {
        const TLocalExpFlags expFlags = {{"mm_pre_confident_scenario_threshold__ECT_SMART_SPEAKER__Vins=2.0", "1"}};
        const float expected = 2.0;

        const TClassificationConfig config;
        const auto actual = GetConfidentThreshold(expFlags, config, "Vins", ECT_SMART_SPEAKER, Nothing());
        UNIT_ASSERT_VALUES_EQUAL(actual, expected);
    }

    Y_UNIT_TEST(WithCorrectLastExp) {
        const TLocalExpFlags expFlags = {{"mm_pre_confident_scenario_threshold__Vins=2.0", "1"},
                                         {"mm_pre_confident_scenario_threshold__ECT_SMART_TV__Vins=2.0", "1"},
                                         {"mm_pre_confident_scenario_threshold__ECT_SMART_TV__my.scenario=2.0", "1"},
                                         {"mm_pre_confident_scenario_threshold__ECT_SMART_TV__Vins10.0", "1"},
                                         {"mm_pre_confident_scenario_threshold__ECT_SMART_SPEAKER__Vins=-2.0", "1"},};
        const float expected = -2.0;

        const TClassificationConfig config;
        const auto actual = GetConfidentThreshold(expFlags, config, "Vins", ECT_SMART_SPEAKER, Nothing());
        UNIT_ASSERT_VALUES_EQUAL(actual, expected);
    }

    Y_UNIT_TEST(ProtoVinsWithPresentExp) {
        const TLocalExpFlags expFlags = {{"mm_pre_confident_scenario_threshold__ECT_SMART_SPEAKER__Vins=2.0", "1"}};
        const float expected = 2.0;

        const TClassificationConfig config;
        const auto actual = GetConfidentThreshold(expFlags, config, "Vins", ECT_SMART_SPEAKER, Nothing());
        UNIT_ASSERT_VALUES_EQUAL(actual, expected);
    }

    Y_UNIT_TEST(Default) {
        const float expected = 1000.0;

        const TLocalExpFlags expFlags;

        NMegamind::TClassificationConfig config;
        config.MutableDefaultScenarioClassificationConfig()->SetPreclassifierConfidentScenarioThreshold(expected);

        const auto actual = GetConfidentThreshold(expFlags, config, "protocol", ECT_SMART_SPEAKER, Nothing());
        UNIT_ASSERT_VALUES_EQUAL(actual, expected);
    }

    Y_UNIT_TEST(DefaultBecauseOfIncorrectClientType) {
        const float expected = 1000.0;

        const TLocalExpFlags expFlags = {{"mm_pre_confident_scenario_threshold__ECT_TOUCH__Vins=2.0", "1"}};;

        NMegamind::TClassificationConfig config;
        config.MutableDefaultScenarioClassificationConfig()->SetPreclassifierConfidentScenarioThreshold(expected);

        const auto actual = GetConfidentThreshold(expFlags, config, "protocol", ECT_SMART_SPEAKER, Nothing());
        UNIT_ASSERT_VALUES_EQUAL(actual, expected);
    }

    Y_UNIT_TEST(DefaultBecauseOfAbsentClientType) {
        const float expected = 1000.0;

        const TLocalExpFlags expFlags = {{"mm_pre_confident_scenario_threshold__Vins=2.0", "1"}};;

        NMegamind::TClassificationConfig config;
        config.MutableDefaultScenarioClassificationConfig()->SetPreclassifierConfidentScenarioThreshold(expected);

        const auto actual = GetConfidentThreshold(expFlags, config, "protocol", ECT_SMART_SPEAKER, Nothing());
        UNIT_ASSERT_VALUES_EQUAL(actual, expected);
    }

    Y_UNIT_TEST(DefaultForScenario) {
        const float expected = 5.0;

        const TLocalExpFlags expFlags;

        NMegamind::TClassificationConfig config;
        config.MutableDefaultScenarioClassificationConfig()->SetPreclassifierConfidentScenarioThreshold(1000.0);
        (*config.MutableScenarioClassificationConfigs())["Vins"].SetPreclassifierConfidentScenarioThreshold(expected);

        const auto actual = GetConfidentThreshold(expFlags, config, "Vins", ECT_SMART_SPEAKER, Nothing());
        UNIT_ASSERT_VALUES_EQUAL(actual, expected);
    }

    Y_UNIT_TEST(FromDescription) {
        const float expected = 5.0;

        const TClassificationConfig config;
        const TLocalExpFlags expFlags;

        TFormulaDescription formula;
        formula.MutableKey()->SetScenarioName("Vins");
        formula.MutableKey()->SetClassificationStage(ECS_PRE);
        formula.MutableKey()->SetClientType(ECT_SMART_SPEAKER);
        formula.MutableKey()->SetLanguage(L_RUS);
        formula.SetFormulaName("794086.Vins");
        formula.MutableConfidentThreshold()->set_value(expected);
        const auto actual = GetConfidentThreshold(expFlags, config, "Vins", ECT_SMART_SPEAKER, std::move(formula));
        UNIT_ASSERT_VALUES_EQUAL(actual, expected);
    }
}

} // namespace
