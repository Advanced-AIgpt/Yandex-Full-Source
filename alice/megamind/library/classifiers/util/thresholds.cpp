#include "thresholds.h"

#include "experiments.h"
#include "scenario_specific.h"

#include <alice/megamind/library/experiments/flags.h>
#include <alice/megamind/library/scenarios/defs/names.h>

#include <alice/library/client/client_info.h>
#include <alice/library/experiments/experiments.h>
#include <alice/library/logger/logger.h>

#include <google/protobuf/wrappers.pb.h>

#include <util/generic/hash.h>
#include <util/generic/maybe.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/string/cast.h>
#include <util/string/split.h>
#include <util/string/vector.h>

namespace NAlice::NMegamind {

namespace {

constexpr double DEFAULT_HOLLYWOOD_MUSIC_THRESHOLD = 0.03;
constexpr double DEFAULT_NEWS_FREE_FRAME_THRESHOLD = 0.7;

TMaybe<float> GetExperimentalConfidentScenarioThresholdForSpecificClient(const THashMap<TString, TMaybe<TString>>& flags,
                                                                         const EClientType clientType, const TStringBuf& scenarioName)
{
    // Exp flags look like EXP_PREFIX_PRE_CONFIDENT_SCENARIO_THRESHOLD_FOR_SPECIFIC_CLIENTclientType__scenarioName=value
    const TStringBuf client = EClientType_Name(clientType);

    const TVector<TStringBuf>& scenarioValues = FindExperimentsWithSubtype(flags, EXP_PREFIX_PRE_CONFIDENT_SCENARIO_THRESHOLD_FOR_SPECIFIC_CLIENT, client, "__");

    for (const TStringBuf scenarioValue : scenarioValues) {
        TStringBuf equalValue;
        if (!scenarioValue.AfterPrefix(scenarioName, equalValue)) {
            continue;
        }
        TStringBuf value;
        if (!equalValue.AfterPrefix(TStringBuf("="), value)) {
            continue;
        }
        float threshold;
        if (TryFromString(value, threshold)) {
            return threshold;
        }
    }

    return Nothing();
}

} // namespace

TMaybe<double> GetHollywoodMusicThreshold(TRTLogger& logger,
                                          const THashMap<TString, TMaybe<TString>>& flags,
                                          const TClientInfo& clientInfo) {
    if (flags.contains(EXP_MUSIC_PLAY_DISABLE_CONFIDENCE_THRESHOLD)) {
        return Nothing();
    }

    if (!flags.contains(EXP_MUSIC_PLAY_ENABLE_CONFIDENCE_THRESHOLD_SPEAKERS_SEARCH_APPS) &&
        HasClassificationMusicFormulas(clientInfo)) {
        // smart speakers and search apps use pretrained classifiers
        return Nothing();
    }

    const TStringBuf thresholdFlag = EXP_MUSIC_PLAY_CONFIDENCE_THRESHOLD_PREFIX;

    if (const auto thresholdStr = GetExperimentValueWithPrefix(flags, thresholdFlag)) {
        double threshold;
        if (!TryFromString<double>(*thresholdStr, threshold)) {
            LOG_WARNING(logger) << "Invalid "
                                << EXP_MUSIC_PLAY_CONFIDENCE_THRESHOLD_PREFIX
                                << *thresholdStr;
            return 1.0; // if invalid, filter out to avoid flooding the scenario
        }
        return threshold;
    }

    return DEFAULT_HOLLYWOOD_MUSIC_THRESHOLD;
}

// SideSpeech is the only scenario with a postclassifier threshold, so it makes sense to have a separate function
TMaybe<float> GetExperimentalSideSpeechThreshold(const THashMap<TString, TMaybe<TString>>& flags) {
    const auto expValue = GetExperimentValueWithPrefix(flags, EXP_PREFIX_SIDE_SPEECH_THRESHOLD);
    if (!expValue) {
        return Nothing();
    }
    float threshold;
    if (TryFromString(*expValue, threshold)) {
        return threshold;
    }
    return Nothing();
}

TMaybe<double> GetNewsFreeFrameThreshold(const THashMap<TString, TMaybe<TString>>& flags) {
    if (flags.contains(EXP_NEWS_DISABLE_CONFIDENCE_THRESHOLD)) {
        return Nothing();
    }

    return DEFAULT_NEWS_FREE_FRAME_THRESHOLD;
}

double GetScenarioThreshold(
    const TFormulasStorage& formulasStorage,
    const TStringBuf scenarioName,
    const EMmClassificationStage stage,
    const EClientType clientType,
    const TStringBuf experiment,
    const ELanguage language
) {
    auto [_, description] = formulasStorage.Lookup(
        scenarioName,
        stage,
        clientType,
        experiment,
        language
    );
    if (description->HasThreshold()) {
        return description->GetThreshold().value();
    }
    return -1000.0;
}

double GetScenarioConfidentThreshold(
    const TFormulasStorage& formulasStorage,
    const TStringBuf scenarioName,
    const EMmClassificationStage stage,
    const EClientType clientType,
    const THashMap<TString, TMaybe<TString>>& flags,
    const TClassificationConfig& config,
    TRTLogger& logger,
    const ELanguage language
) {
    // Exp flags look like EXP_PREFIX_PRE_CONFIDENT_SCENARIO_THRESHOLD_FOR_SPECIFIC_CLIENTclientType__scenarioName=value

    const TMaybe<float> expThreshold = GetExperimentalConfidentScenarioThresholdForSpecificClient(flags, clientType, scenarioName);
    if (expThreshold.Defined()) {
        LOG_DEBUG(logger) << "Preclassifier confident " << scenarioName <<
                             " threshold is set to " << expThreshold.GetRef();
        return expThreshold.GetRef();
    }

    const TMaybe<TStringBuf> formulaExperiment = GetMMFormulaExperimentForSpecificClient(flags, clientType);
    auto [_, description] = formulasStorage.Lookup(
        scenarioName,
        stage,
        clientType,
        formulaExperiment.GetOrElse({}),
        language
    );
    if (description->HasConfidentThreshold()) {
        return description->GetConfidentThreshold().value();
    }
    const auto& classificationConfig = config.GetScenarioClassificationConfigs();
    const auto& scenarioClassificationConfig = classificationConfig.find(scenarioName);
    if (scenarioClassificationConfig != classificationConfig.end()) {
        return scenarioClassificationConfig->second.GetPreclassifierConfidentScenarioThreshold();
    }
    return config.GetDefaultScenarioClassificationConfig().GetPreclassifierConfidentScenarioThreshold();
}

} // namespace NAlice::NMegamind
