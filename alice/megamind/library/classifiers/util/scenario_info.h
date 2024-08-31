#pragma once

#include <alice/megamind/protos/quality_storage/storage.pb.h>

namespace NAlice {

void UpdateScenarioClassificationInfo(
    TQualityStorage& qualityStorage,
    const ELossReason lossReason,
    const TString& scenarioName,
    const EMmClassificationStage stage
);

}
