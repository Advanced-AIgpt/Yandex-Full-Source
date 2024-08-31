#include "priority.h"

#include <alice/megamind/library/classifiers/formulas/formulas_storage.h>
#include <alice/megamind/protos/quality_storage/storage.pb.h>

#include <alice/protos/data/language/language.pb.h>

#include <kernel/formula_storage/formula_storage.h>

#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/testing/unittest/tests_data.h>

#include <util/generic/map.h>

namespace {

using namespace NAlice;

bool HasFormulasForClassification(
    const TString& scenario,
    const EMmClassificationStage classificationStage,
    const TString& appId,
    const TString& experiment
) {
    THashMap<TString, TMaybe<TString>> expFlags{{experiment, ""}};

    TClientInfoProto clientInfoProto;
    clientInfoProto.SetAppId(appId);
    TClientFeatures clientFeatures{clientInfoProto, {}};

    TFormulasDescription description;

    TFormulaDescription formulaDescription;
    // default formula
    formulaDescription.MutableKey()->SetScenarioName("Vins");
    formulaDescription.MutableKey()->SetClassificationStage(ECS_POST);
    formulaDescription.MutableKey()->SetClientType(ECT_SMART_SPEAKER);
    formulaDescription.MutableKey()->SetLanguage(L_RUS);
    formulaDescription.SetFormulaName("794215.Vins");
    description.AddFormula(formulaDescription);

    // exp formula
    formulaDescription.MutableKey()->SetScenarioName("Vins");
    formulaDescription.MutableKey()->SetClassificationStage(ECS_POST);
    formulaDescription.MutableKey()->SetClientType(ECT_SMART_SPEAKER);
    formulaDescription.MutableKey()->SetLanguage(L_RUS);
    formulaDescription.SetFormulaName("792987.Vins");
    formulaDescription.MutableKey()->SetExperiment("correct_exp");
    description.AddFormula(formulaDescription);

    ::TFormulasStorage RawFormulasStorage;
    const auto& formulasPath = TFsPath(GetWorkPath()) / "megamind_formulas";
    RawFormulasStorage.AddFormulasFromDirectoryRecursive(formulasPath);

    NAlice::TFormulasStorage formulasStorage{RawFormulasStorage, description};
    return NImpl::HasFormulasForClassification(formulasStorage, scenario, classificationStage, clientFeatures, expFlags, LANG_RUS);
}

Y_UNIT_TEST_SUITE(HasFormulasForClassification) {
    Y_UNIT_TEST(HasFormula) {
        UNIT_ASSERT(HasFormulasForClassification("Vins", ECS_POST, "ru.yandex.quasar", ""));
        UNIT_ASSERT(HasFormulasForClassification("Vins", ECS_POST, "ru.yandex.quasar", "mm_formula=correct_exp"));
    }
    Y_UNIT_TEST(WrongScenario) {
        UNIT_ASSERT(!HasFormulasForClassification("HollywoodMusic", ECS_POST, "ru.yandex.quasar", ""));
    }
    Y_UNIT_TEST(WrongStage) {
        UNIT_ASSERT(!HasFormulasForClassification("Vins", ECS_PRE, "ru.yandex.quasar", ""));
    }
    Y_UNIT_TEST(WrongClient) {
        UNIT_ASSERT(!HasFormulasForClassification("Vins", ECS_POST, "com.yandex.tv.alice", ""));
    }
    Y_UNIT_TEST(FallbackToDefaultWrongExp) {
        UNIT_ASSERT(HasFormulasForClassification("Vins", ECS_POST, "ru.yandex.quasar", "mm_formula=wrong_exp"));
    }
    Y_UNIT_TEST(DontFallbackToDefaultOldRanking) {
        UNIT_ASSERT(!HasFormulasForClassification("Vins", ECS_POST, "ru.yandex.quasar", "mm_formula=old_ranking"));
    }
}

} // namespace
