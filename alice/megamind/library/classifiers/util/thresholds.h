#pragma once

#include <alice/megamind/library/experiments/flags.h>
#include <alice/megamind/library/request/request.h>

#include <alice/library/client/fwd.h>
#include <alice/library/logger/fwd.h>
#include <alice/megamind/library/classifiers/formulas/formulas_storage.h>
#include <alice/megamind/library/config/protos/classification_config.pb.h>

#include <library/cpp/langs/langs.h>

#include <util/generic/fwd.h>

namespace NAlice::NMegamind {

TMaybe<double> GetHollywoodMusicThreshold(TRTLogger& logger,
                                          const THashMap<TString, TMaybe<TString>>& flags,
                                          const TClientInfo& clientInfo);

TMaybe<float> GetExperimentalSideSpeechThreshold(const THashMap<TString, TMaybe<TString>>& flags);

TMaybe<double> GetNewsFreeFrameThreshold(const THashMap<TString, TMaybe<TString>>& flags);

/**
 * Function retrieves confidence threshold for scenario by associated formula attributes
 * Threshold is used by preclassifier to cut off unlikely scenarios
 */
double GetScenarioThreshold(
    const TFormulasStorage& formulasStorage,
    const TStringBuf scenarioName,
    const EMmClassificationStage stage,
    const EClientType clientType,
    const TStringBuf experiment,
    const ELanguage language
);

double GetScenarioConfidentThreshold(
    const TFormulasStorage& formulasStorage,
    const TStringBuf scenarioName,
    const EMmClassificationStage stage,
    const EClientType clientType,
    const THashMap<TString, TMaybe<TString>>& flags,
    const TClassificationConfig& config,
    TRTLogger& logger,
    const ELanguage language
);

} // namespace NAlice::NMegamind
