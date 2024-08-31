#include "formulas_description.h"

#include <alice/megamind/library/experiments/flags.h>
#include <alice/megamind/protos/quality_storage/storage.pb.h>

#include <alice/library/experiments/experiments.h>

#include <library/cpp/protobuf/util/pb_io.h>

namespace NAlice {

TFormulasDescription::TFormulasDescription(
        const google::protobuf::Map<TString, NMegamind::TClassificationConfig::TScenarioConfig>& scenarioClassificationConfigs,
        const TFsPath& formulasFolder,
        TRTLogger& logger
) {
    FillFormulasDescriptionMapByScenarioClassificationConfigs(scenarioClassificationConfigs);
    TFormulasDescription::AddFormulasFromResource(formulasFolder / "formulas_config.pb.txt", logger);
}

bool TFormulasDescription::AddFormula(TFormulaDescription formulaDescription) {
    auto&& [_, ok] = FormulasDescriptionMap_.emplace(
        std::move(*formulaDescription.MutableKey()),
        std::move(formulaDescription)
    );
    return ok;
}

void TFormulasDescription::FillFormulasDescriptionMapByScenarioClassificationConfigs(
        const google::protobuf::Map<TString, NMegamind::TClassificationConfig::TScenarioConfig>& scenarioClassificationConfigs
) {
    for (const auto& [scenarioName, scenarioConfig] : scenarioClassificationConfigs) {
        const auto& formulasDescriptionList = scenarioConfig.GetFormulasDescriptionList();
        for (TFormulaDescription formulaDescription : formulasDescriptionList.GetFormulasDescription()) {
            formulaDescription.MutableKey()->SetScenarioName(scenarioName);
            Y_ENSURE(AddFormula(std::move(formulaDescription)));
        }
    }
}

void TFormulasDescription::AddFormulasFromResource(const TFsPath& formulasPath, TRTLogger& logger) {
    NMegamind::TClassificationConfig classificationConfig;
    try {
        ParseFromTextFormat(formulasPath, classificationConfig);
        LOG_INFO(logger) << "Resource config file added successfully. Path: " << formulasPath;
    } catch (TIoException ex) {
        LOG_INFO(logger) << "There is no resource config file: " << ex.what();
    } catch (yexception ex) {
        LOG_ERR(logger) << "Error with parcing resource config file: " << ex.what();
    }

    FillFormulasDescriptionMapByScenarioClassificationConfigs(classificationConfig.GetScenarioClassificationConfigs());
}

const TFormulaDescription& TFormulasDescription::Lookup(
    const TStringBuf scenarioName,
    const EMmClassificationStage classificationStage,
    const EClientType clientType,
    const TStringBuf experiment,
    const ELanguage language
) const noexcept {
    Y_ENSURE(classificationStage != ECS_UNKNOWN);

    TFormulaKey formulaKey;
    formulaKey.SetScenarioName(TString{scenarioName});
    formulaKey.SetClassificationStage(classificationStage);
    formulaKey.SetClientType(clientType);
    formulaKey.SetExperiment(TString{experiment});
    formulaKey.SetLanguage(static_cast<ELang>(language));

    const auto* formula = FormulasDescriptionMap_.FindPtr(formulaKey);
    return formula ? *formula : Default<NAlice::TFormulaDescription>();
}

} // namespace NAlice
