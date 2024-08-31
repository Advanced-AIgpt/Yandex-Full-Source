#pragma once

#include <alice/megamind/library/classifiers/util/scenario_info.h>
#include <alice/megamind/library/classifiers/util/table.h>
#include <alice/megamind/library/request/request.h>
#include <alice/megamind/library/scenarios/helpers/interface/scenario_ref.h>

#include <util/generic/fwd.h>

class TFactorStorage;

namespace NAlice {

class IContext;
class TFormulasStorage;
class TRTLogger;
class TScenario;
class TSemanticFrame;
class TWizardResponse;

using TScenarioToRequestFrames = THashMap<TIntrusivePtr<IScenarioRef>, TVector<TSemanticFrame>>;

namespace NImpl {

constexpr TStringBuf CONTINUE_PRECLASSIFIER_HINT_NAME = "personal_assistant.scenarios.fast_command.fast_continue";
constexpr TStringBuf CONTINUE_LITE_PRECLASSIFIER_HINT_NAME = "personal_assistant.scenarios.fast_command.fast_continue_lite";
constexpr TStringBuf OPEN_OR_CONTINUE_PRECLASSIFIER_HINT_NAME = "personal_assistant.scenarios.fast_command.fast_open_or_continue";
constexpr TStringBuf PAUSE_PRECLASSIFIER_HINT_NAME = "personal_assistant.scenarios.fast_command.fast_pause";

[[nodiscard]] bool ShouldFilterMiscHollywoodMusic(const IContext& ctx, const TScenario& scenario);

[[nodiscard]] bool ShouldFilterProtocolVideoScenario(const IContext& ctx, const TScenario& scenario);

bool ShouldFilterNewsScenario(const IContext& ctx, const TScenario& scenario,
    const TVector<TSemanticFrame>& frames, const TWizardResponse& wizardResponse);
bool ShouldFilterSideSpeechScenario(const IContext& ctx, const TScenario& scenario);
bool ShouldSkipPreclassifierHint(const IContext& ctx, const TString& scenarioName, const TString& hintFrameName);

THashMap<TStringBuf, double> GetExpThresholds(const NMegamind::TClientComponent::TExpFlags& expFlags, TRTLogger& logger);

template <class TPredicate>
bool TryEraseIf(TScenarioToRequestFrames& candidateToFrames, TPredicate predicate, TQualityStorage& qualityStorage,
                const ELossReason lossReason)
{
    TVector<const TScenarioToRequestFrames::key_type*> toRemove;
    TScenarioToRequestFrames newCandidateToFrames;
    for (const auto& candidateFrames : candidateToFrames) {
        if (predicate(candidateFrames)) {
            UpdateScenarioClassificationInfo(qualityStorage, lossReason, candidateFrames.first->GetScenario().GetName(), ECS_PRE);
            toRemove.push_back(&candidateFrames.first);
        }
    }
    for(const auto& scenario : toRemove) {
        candidateToFrames.erase(*scenario);
    }
    return !toRemove.empty();
}

bool LeaveOnly(const THashSet<TStringBuf>& names, TScenarioToRequestFrames& candidateToFrames, const IContext& ctx,
               TQualityStorage& qualityStorage, const ELossReason lossReason,
               bool ensureEnabled = true, const TMaybe<TRequest::TScenarioInfo>& boostedScenario = Nothing());

TString LogScenarioToRequestFrames(const TScenarioToRequestFrames& candidateToFrames);

TString CheckScenarioConfidentFrames(
    const google::protobuf::Map<TString, NMegamind::TClassificationConfig::TScenarioConfig>& classificationConfigs,
    const TWizardResponse& wizardResponse,
    const TString& scenarioName,
    const IContext::TExpFlags& expFlags
);

class TClassificationProcessor {
public:
    TClassificationProcessor(const TScenarioToRequestFrames& candidateToRequestFrames, const TRequest& request,
                             const IContext& ctx, const TFormulasStorage& formulasStorage,
                             const TFactorStorage& factorStorage, TQualityStorage& qualityStorage);
    TScenarioToRequestFrames PreClassify();

private:
    const TScenarioToRequestFrames& CandidateToRequestFrames;
    const TRequest& Request;
    const TMaybe<TRequest::TScenarioInfo>& BoostedScenario;
    const IEvent& Event;
    const IContext& Ctx;
    const TWizardResponse& WizardResponse;
    const TFormulasStorage& FormulasStorage;
    const TFactorStorage& FactorStorage;
    TQualityStorage& QualityStorage;

    TClassificationTable Table;
    TVector<TStringBuf> Scenarios;

    bool CanUsePreclassifierHint;
    bool ApplyHintFiltration = false;
    bool ApplyThreshHoldFiltration = false;

private:
    static TVector<TStringBuf> GetScenarios(const TScenarioToRequestFrames& candidateToRequestFrames);
    bool ClsfForcedOrBoostedScenario();
    void ClsfAllowedByMinRequirements();
    void FilterHints();
    void AvoidHintsCut();
    bool IsFoundConfidentScenario();
    void FilterThresholdAndAvoiding();
    void AvoidAnyCut();
};

} // namespace NImpl

/**
 * PreClassify filters enabled scenarios
 * Also fills some predicts in qualityStorage
 */
void PreClassify(
    TScenarioToRequestFrames& candidateToRequestFrames,
    const TRequest& request,
    const IContext& ctx,
    const TFormulasStorage& formulasStorage,
    const TFactorStorage& factorStorage,
    TQualityStorage& qualityStorage
);

/**
 * Check whether it is request with partial utterance
 */
bool PreClassifyPartial(
    const IContext& ctx,
    const TFactorStorage& factorStorage
);

} // namespace NAlice
