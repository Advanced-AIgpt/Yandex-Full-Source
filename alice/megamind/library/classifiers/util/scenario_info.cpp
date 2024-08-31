#include "scenario_info.h"

namespace NAlice {

void UpdateScenarioClassificationInfo(
    TQualityStorage& qualityStorage,
    const ELossReason lossReason,
    const TString& scenarioName,
    const EMmClassificationStage stage
) {
    (*qualityStorage.MutableScenariosInformation())[scenarioName].SetClassificationStage(stage);
    (*qualityStorage.MutableScenariosInformation())[scenarioName].SetReason(lossReason);
}

}
