#include "matrixnet.h"

#include <alice/megamind/library/classifiers/formulas/formulas_storage.h>
#include <alice/megamind/library/factor_storage/factor_storage.h>

#include <alice/protos/data/language/language.pb.h>

#include <kernel/formula_storage/formula_storage.h>

#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/testing/unittest/tests_data.h>

namespace {

using namespace NAlice;
using namespace NAlice::NMegamind;

double GetPredict(const TStringBuf flag, const TStringBuf scenario = TStringBuf("Video")) {
    TFormulasDescription description;

    TFormulaDescription formulaDescription;
    formulaDescription.MutableKey()->SetScenarioName("Video");
    formulaDescription.MutableKey()->SetClassificationStage(ECS_POST);
    formulaDescription.MutableKey()->SetClientType(ECT_SMART_SPEAKER);
    formulaDescription.MutableKey()->SetLanguage(L_RUS);
    formulaDescription.SetFormulaName("794214.Video");
    description.AddFormula(formulaDescription);

    formulaDescription.MutableKey()->SetScenarioName("Video");
    formulaDescription.MutableKey()->SetClassificationStage(ECS_POST);
    formulaDescription.MutableKey()->SetClientType(ECT_SMART_SPEAKER);
    formulaDescription.MutableKey()->SetLanguage(L_RUS);
    formulaDescription.SetFormulaName("792988.Video");
    formulaDescription.MutableKey()->SetExperiment("old_ranking");
    description.AddFormula(formulaDescription);

    ::TFormulasStorage RawFormulasStorage;
    const auto& formulasPath = TFsPath(GetWorkPath()) / "megamind_formulas";
    RawFormulasStorage.AddFormulasFromDirectoryRecursive(formulasPath);

    NAlice::TFormulasStorage formulasStorage{RawFormulasStorage, description};
    const TFactorStorage factorStorage = CreateFactorStorage(CreateFactorDomain());
    return ApplyScenarioFormula(
        formulasStorage,
        scenario,
        ECS_POST,
        ECT_SMART_SPEAKER,
        flag,
        factorStorage,
        TRTLogger::NullLogger(),
        LANG_RUS
    );
}

Y_UNIT_TEST_SUITE(ApplyScenarioFormula) {
    Y_UNIT_TEST(ApplyScenarioFormulaDefault) {
        const double predict = GetPredict({});
        UNIT_ASSERT_VALUES_UNEQUAL(predict, 0);
    }
    Y_UNIT_TEST(ApplyScenarioFormulaConsistency) {
        const double predict1 = GetPredict({});
        const double predict2 = GetPredict({});
        UNIT_ASSERT_VALUES_EQUAL(predict1, predict2);
    }
    Y_UNIT_TEST(ApplyScenarioFormulaFakeExp) {
        const double predict1 = GetPredict(TStringBuf("fake_experiment"));
        const double predict2 = GetPredict({});
        UNIT_ASSERT_VALUES_EQUAL(predict1, predict2);
    }
    Y_UNIT_TEST(ApplyScenarioFormulaExp) {
        const double predict1 = GetPredict(TStringBuf("old_ranking"));
        const double predict2 = GetPredict({});
        UNIT_ASSERT_VALUES_UNEQUAL(predict1, predict2);
    }
    Y_UNIT_TEST(ApplyScenarioFormulaFakeScenario) {
        const double predict = GetPredict({}, TStringBuf("fake_scenario"));
        UNIT_ASSERT_VALUES_EQUAL(predict, 0);
    }
}

} // namespace
