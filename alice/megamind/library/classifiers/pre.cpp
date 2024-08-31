#include "pre.h"

#include <alice/megamind/library/classifiers/defs/priorities.h>
#include <alice/megamind/library/classifiers/features/calcers.h>
#include <alice/megamind/library/classifiers/formulas/formulas_description.h>
#include <alice/megamind/library/classifiers/formulas/formulas_storage.h>
#include <alice/megamind/library/classifiers/rankers/matrixnet.h>
#include <alice/megamind/library/classifiers/rankers/priority.h>
#include <alice/megamind/library/classifiers/util/experiments.h>
#include <alice/megamind/library/classifiers/util/modes.h>
#include <alice/megamind/library/classifiers/util/scenario_info.h>
#include <alice/megamind/library/classifiers/util/table.h>
#include <alice/megamind/library/classifiers/util/thresholds.h>

#include <alice/megamind/library/context/context.h>
#include <alice/megamind/library/experiments/flags.h>
#include <alice/megamind/library/scenarios/defs/names.h>
#include <alice/megamind/library/scenarios/helpers/interface/scenario_ref.h>
#include <alice/megamind/library/scenarios/quasar/common.h>
#include <alice/megamind/library/vins/wizard.h>
#include <alice/megamind/library/worldwide/language/is_alice_worldwide_language.h>

#include <alice/megamind/protos/partials_pre/embedding.pb.h>
#include <alice/megamind/protos/quality_storage/storage.pb.h>

#include <alice/library/experiments/experiments.h>
#include <alice/library/experiments/flags.h>
#include <alice/library/frame/utils.h>
#include <alice/library/metrics/sensors.h>
#include <alice/library/metrics/names.h>
#include <alice/library/metrics/util.h>
#include <alice/library/logger/logger.h>
#include <alice/library/music/defs.h>
#include <alice/library/video_common/defs.h>
#include <alice/library/client/interfaces_util.h>

#include <catboost/libs/model/eval_processing.h>
#include <catboost/libs/model/model.h>

#include <kernel/catboost/catboost_calcer.h>
#include <kernel/factor_storage/factor_storage.h>

#include <library/cpp/iterator/filtering.h>
#include <library/cpp/iterator/mapped.h>

#include <util/generic/algorithm.h>
#include <util/generic/is_in.h>
#include <util/string/cast.h>
#include <util/string/join.h>
#include <util/string/split.h>

#include <cmath>

namespace NAlice {

namespace NImpl {

namespace {

const TVector<TStringBuf> AVOID_CUTTING_BY_THRESHOLD_FRAMES = {
    TStringBuf("personal_assistant.scenarios.quasar.open_current_video"),
    TStringBuf("personal_assistant.scenarios.quasar.payment_confirmed"),
    TStringBuf("personal_assistant.scenarios.quasar.select_channel_from_gallery_by_text"),
    TStringBuf("personal_assistant.scenarios.quasar.select_video_from_gallery_by_text"),
    TStringBuf("personal_assistant.scenarios.select_video_by_number"),
    TStringBuf("personal_assistant.scenarios.player.next_track"),
    TStringBuf("personal_assistant.scenarios.player.previous_track"),
    TStringBuf("personal_assistant.scenarios.player.continue"),
    TStringBuf("personal_assistant.scenarios.player.what_is_playing"),
    TStringBuf("personal_assistant.scenarios.player.like"),
    TStringBuf("personal_assistant.scenarios.player.dislike"),
    TStringBuf("personal_assistant.scenarios.player.shuffle"),
    TStringBuf("personal_assistant.scenarios.player.replay"),
    TStringBuf("personal_assistant.scenarios.player.rewind"),
    TStringBuf("personal_assistant.scenarios.player.repeat"),
    TStringBuf("alice.search.related_agree"),
};

bool ShouldAvoidCuttingByThreshold(const IContext& ctx) {
    return ctx.HasExpFlag(EXP_DISABLE_TRAINED_PRECLASSIFIER) ||
           AnyOf(AVOID_CUTTING_BY_THRESHOLD_FRAMES, [&ctx](const TStringBuf frame) {
               return ctx.Responses().WizardResponse().HasGranetFrame(frame);
           });
}

bool IsWaitingForSlotValue(const TScenario& scenario, const IContext& ctx) {
    return ctx.Session()
        && ctx.Session()->GetPreviousScenarioName() == scenario.GetName()
        && ctx.Session()->GetRequestedSlot().Defined();
}

bool DisablePreclassifierHint(const IContext& ctx) {
    return ctx.HasExpFlag(EXP_DISABLE_PRECLASSIFIER_HINTS)
        || (ctx.Session() && ctx.Session()->GetRequestIsExpected());
}

bool IsBoosted(const TScenario& scenario, const TMaybe<TRequest::TScenarioInfo>& boostedScenario) {
    return boostedScenario.Defined() && boostedScenario->GetName() == scenario.GetName();
}

bool IsEnabled(const TScenario& scenario, const TMaybe<TRequest::TScenarioInfo>& boostedScenario,
               const IContext& ctx, const TMaybe<TStringBuf>& onlyEnabledScenario = Nothing()) {
    if (onlyEnabledScenario.Defined()) {
        return scenario.GetName() == *onlyEnabledScenario;
    }
    return !ctx.HasExpFlag(EXP_PREFIX_MM_DISABLE_PROTOCOL_SCENARIO + scenario.GetName()) &&
           (scenario.IsEnabled(ctx) || NImpl::IsBoosted(scenario, boostedScenario));
}

bool FramesIntersectRecognizedActionEffectFrames(const TVector<TSemanticFrame>& lhs, const TVector<TSemanticFrame>& rhs) {
    for (const auto& lhsFrame : lhs) {
        for (const auto& rhsFrame : rhs) {
            if (lhsFrame.GetName() == rhsFrame.GetName()) {
                return true;
            }
        }
    }
    return false;
}

bool IsInFixlist(const TString& scenarioName, const IContext& ctx, const TRequest& request) {
    const auto& fl = ctx.Responses().WizardResponse().GetFixlist();
    const THashSet<TString>& matches = fl.GetMatches();
    const THashMap<TString, THashSet<TString>>& featuresMatches = fl.GetSupportedFeaturesMatches();
    const auto& interfaces = request.GetInterfaces();

    if (matches.contains(scenarioName)){
        return true;
    }
    if (featuresMatches.contains(scenarioName)){
        for (const auto& feature : featuresMatches.at(scenarioName)) {
            if (CheckFeatureSupport(interfaces, feature, ctx.Logger())) {
                return true;
            }
        }
    }
    return false;
}

bool IsScenarioAllowedByLanguage(const IContext& ctx, const TScenario& scenario) {
    const auto language = ctx.Language();
    const auto ordinarLanguage = NMegamind::ConvertAliceWorldWideLanguageToOrdinar(language);

    if (ctx.HasExpFlag(EXP_FORCE_TRANSLATED_LANGUAGE_FOR_ALL_SCENARIOS) &&
        !ctx.HasExpFlag(EXP_FORCE_POLYGLOT_LANGUAGE_FOR_SCENARIO_PREFIX + scenario.GetName()))
    {
        return scenario.IsLanguageSupported(ordinarLanguage);
    }

    return scenario.IsLanguageSupported(language) || scenario.IsLanguageSupported(ordinarLanguage);
}

} // namespace

THashMap<TStringBuf, double> GetExpThresholds(const NMegamind::TClientComponent::TExpFlags& expFlags, TRTLogger& logger) {
    THashMap<TStringBuf, double> scenarioToThreshold;
    if (auto thresholdsStr = GetExperimentValueWithPrefix(expFlags, EXP_PREFIX_MM_PRECLASSIFIER_THRESHOLDS)) {
        TVector<TStringBuf> pairs;
        Split(*thresholdsStr, ";", pairs);
        for (const auto& p : pairs) {
            TVector<TStringBuf> parts;
            Split(p, ":", parts);
            if (parts.size() != 2) {
                LOG_ERROR(logger) << "Parts size should be 2, but got " << parts.size() << ": " << p;
                continue;
            }
            double threshold;
            if (TryFromString<double>(parts[1], threshold)) {
                scenarioToThreshold[parts[0]] = threshold;
            } else {
                LOG_ERROR(logger) << "Failed to parse double from \"" << parts[1] << "\"";
            }
        }
    }

    return scenarioToThreshold;
}

bool ShouldFilterProtocolVideoScenario(const IContext& ctx, const TScenario& scenario) {
    return scenario.GetName() == MM_VIDEO_PROTOCOL_SCENARIO
           && !ctx.ClientFeatures().IsSmartSpeaker()
           && !ctx.ClientFeatures().IsTvDevice()
           && !ctx.ClientFeatures().IsLegatus();
}

bool ShouldFilterMiscHollywoodMusic(const IContext& ctx, const TScenario& scenario) {
    if (scenario.GetName() != HOLLYWOOD_MUSIC_SCENARIO) {
        return false;
    }

    if (const auto threshold = NMegamind::GetHollywoodMusicThreshold(ctx.Logger(), ctx.ExpFlags(), ctx.ClientInfo())) {
        const auto& wizardResponse = ctx.Responses().WizardResponse();
        const double confidence = wizardResponse.GetFrameConfidence(NMusic::MUSIC_PLAY);
        return confidence <= *threshold;
    }

    // if no threshold is given, never filter out, assume that the trained preclassifier works
    return false;
}

bool ShouldFilterNewsScenario(const IContext& ctx, const TScenario& scenario, const TVector<TSemanticFrame>& frames,
                              const TWizardResponse& wizardResponse) {
    if (scenario.GetName() != MM_NEWS_PROTOCOL_SCENARIO) {
        return false;
    }

    const bool isFreeGrammar = frames.size() == 1 && frames.front().GetName() == MM_NEWS_FREE_FRAME_NAME;

    if (isFreeGrammar) {
        const auto threshold = NMegamind::GetNewsFreeFrameThreshold(ctx.ExpFlags());
        if (threshold) {
            return wizardResponse.GetFrameConfidence(MM_NEWS_FRAME_NAME) < *threshold;
        }
    }
    return false;
}

bool ShouldFilterSideSpeechScenario(const IContext& ctx, const TScenario& scenario) {
    return scenario.GetName() == SIDE_SPEECH_SCENARIO && !ctx.ClientFeatures().IsSmartSpeaker();
}

bool ShouldSkipPreclassifierHint(const IContext& ctx, const TString& scenarioName, const TString& hintFrameName) {
    const auto disableAllExpFlag =
        Join(Default<TStringBuf>(), EXP_DISABLE_PRECLASSIFIER_HINT_PREFIX, hintFrameName);
    const auto disableForScenarioExpFlag =
        Join(Default<TStringBuf>(), EXP_DISABLE_PRECLASSIFIER_HINT_PREFIX, hintFrameName, ":", scenarioName);
    if (ctx.HasExpFlag(disableAllExpFlag) || ctx.HasExpFlag(disableForScenarioExpFlag)) {
        return true;
    }

    const bool hasPlayer = IsMediaPlayer(ctx) || (HasActivePlayerWidget(ctx) && IsMainScreen(ctx));
    if (hintFrameName == NImpl::OPEN_OR_CONTINUE_PRECLASSIFIER_HINT_NAME && !hasPlayer) {
        return true;
    }

    // the experiment controls whether fast_continue or fast_continue_lite is enabled
    if (hintFrameName == NImpl::CONTINUE_PRECLASSIFIER_HINT_NAME && ctx.HasExpFlag(NExperiments::EXP_MUSIC_LITE_HINT)) {
        return true;
    }

    if (hintFrameName == NImpl::CONTINUE_LITE_PRECLASSIFIER_HINT_NAME && !ctx.HasExpFlag(NExperiments::EXP_MUSIC_LITE_HINT)) {
        return true;
    }

    if (hintFrameName == NImpl::CONTINUE_PRECLASSIFIER_HINT_NAME &&
        scenarioName == HOLLYWOOD_MUSIC_SCENARIO &&
        !ctx.HasExpFlag(NExperiments::EXP_HW_MUSIC_THIN_CLIENT) &&
        !ctx.HasExpFlag(NExperiments::EXP_HW_MUSIC_THIN_CLIENT_GENERATIVE) &&
        !ctx.HasExpFlag(NExperiments::EXP_ENABLE_CONTINUE_IN_HW_MUSIC))
    {
        return true;
    }

    if (hintFrameName == NImpl::CONTINUE_PRECLASSIFIER_HINT_NAME &&
        scenarioName == MM_VIDEO_PROTOCOL_SCENARIO &&
        !ctx.HasExpFlag(EXP_VIDEO_ADD_FAST_CONTINUE_PRECLASSIFIER_HINT))
    {
        return true;
    }

    return false;
}

bool LeaveOnly(const THashSet<TStringBuf>& names, TScenarioToRequestFrames& candidateToFrames, const IContext& ctx,
               TQualityStorage& qualityStorage, const ELossReason lossReason, bool ensureEnabled,
               const TMaybe<TRequest::TScenarioInfo>& boostedScenario)
{
    return TryEraseIf(candidateToFrames,
                      [&names, &ctx, ensureEnabled, &boostedScenario]
                      (const TScenarioToRequestFrames::value_type& candidateFrames) {
                          const TScenario& scenario = candidateFrames.first->GetScenario();
                          return !(names.contains(scenario.GetName()) &&
                                   (!ensureEnabled || IsEnabled(scenario, boostedScenario, ctx)));
                      },
                      qualityStorage,
                      lossReason);
}

TString LogScenarioToRequestFrames(const TScenarioToRequestFrames& candidateToFrames) {
    if (candidateToFrames.empty()) {
        return TString{"-"};
    }
    auto range = MakeMappedRange(candidateToFrames, [](const TScenarioToRequestFrames::value_type& candidateFrames) {
        return candidateFrames.first->GetScenario().GetName();
    });
    return JoinRange(/* delim= */ TStringBuf(", "), range.begin(), range.end());
}

void LogScenariosCutByHint(const TScenarioToRequestFrames& candidateToFrames, const THashSet<TStringBuf>& leftScenarios,
                           const IContext& ctx, const TStringBuf scenarioWithHint)
{
    auto filteredRange = MakeFilteringRange(candidateToFrames, [&leftScenarios](const TScenarioToRequestFrames::value_type& candidateFrames) {
        return !leftScenarios.contains(candidateFrames.first->GetScenario().GetName());
    });
    auto namesRange = MakeMappedRange(filteredRange, [](const TScenarioToRequestFrames::value_type& candidateFrames) {
        return candidateFrames.first->GetScenario().GetName();
    });
    LOG_INFO(ctx.Logger()) << "Scenarios were cut by " << scenarioWithHint << " hint: " <<
                               JoinRange(/* delim= */ TStringBuf(", "), namesRange.begin(), namesRange.end());
}

TString CheckScenarioConfidentFrames(
    const google::protobuf::Map<TString, NMegamind::TClassificationConfig::TScenarioConfig>& classificationConfigs,
    const TWizardResponse& wizardResponse,
    const TString& scenarioName,
    const IContext::TExpFlags& expFlags
) {
    const TMaybe<TStringBuf> expFrame = NMegamind::GetPreclassificationConfidentFramesExperiment(expFlags, scenarioName);
    if (expFrame.Defined() && wizardResponse.GetRequestFrame(*expFrame)) {
        return TString(*expFrame);
    }

    const auto configIt = classificationConfigs.find(scenarioName);
    if (configIt == classificationConfigs.end()) {
        return {};
    }

    for (const auto& frameName : configIt->second.GetPreclassifierConfidentFrames()) {
        if (wizardResponse.GetRequestFrame(frameName)) {
            return frameName;
        }
    }

    return {};
}

TClassificationProcessor::TClassificationProcessor(const TScenarioToRequestFrames& candidateToRequestFrames, const TRequest& request, const IContext& ctx,
                                                   const TFormulasStorage& formulasStorage, const TFactorStorage& factorStorage,
                                                   TQualityStorage& qualityStorage)
    : CandidateToRequestFrames(candidateToRequestFrames)
    , Request(request)
    , BoostedScenario(request.GetScenario())
    , Event(request.GetEvent())
    , Ctx(ctx)
    , WizardResponse(ctx.Responses().WizardResponse())
    , FormulasStorage(formulasStorage)
    , FactorStorage(factorStorage)
    , QualityStorage(qualityStorage)
    , Table(GetScenarios(candidateToRequestFrames))
{
    CanUsePreclassifierHint = !NImpl::DisablePreclassifierHint(Ctx);
}

TVector<TStringBuf> TClassificationProcessor::GetScenarios(const TScenarioToRequestFrames& candidateToRequestFrames) {
    TVector<TStringBuf> scenarios;
    scenarios.reserve(candidateToRequestFrames.size());
    for (const auto& [candidate, frames] : candidateToRequestFrames) {
        scenarios.push_back(candidate->GetScenario().GetName());
    }
    return scenarios;
}

TScenarioToRequestFrames TClassificationProcessor::PreClassify() {
    const auto scenarios = GetScenarios(CandidateToRequestFrames);
    LOG_INFO(Ctx.Logger()) << TLogMessageTag{"PreClassification stage"} << "PreClassification initial scenarios: " << JoinRange(/* delim= */ TStringBuf(", "), scenarios.begin(), scenarios.end());;
    TVector<TStringBuf> leaderBoard;

    if (!ClsfForcedOrBoostedScenario())  {
        ClsfAllowedByMinRequirements();
        const auto scenariosAllNecessary = Table.GetAllNecessary();
        LOG_INFO(Ctx.Logger()) << "Necessary filtrations result: " << JoinRange(/* delim= */ TStringBuf(", "), scenariosAllNecessary.begin(), scenariosAllNecessary.end());
        // Here if there is no hints or prediction greater then confident threshold preclassification finishes
        // cause all necessary filtration are applied and no need to specify scenarios that should skip hints or threshhold selection

        if (CanUsePreclassifierHint) {
            FilterHints();
            if (Table.StrongMarkersExist()) {
                ApplyHintFiltration = true;
                AvoidHintsCut();
                Table.SetStrongMarkersLossReason(LR_PRE_HINT);
                LOG_INFO(Ctx.Logger()) << "Hints cut applied.";
            }
        }

        leaderBoard = Table.GetWinners(QualityStorage);
        if (leaderBoard.size() == 1) {
            LOG_INFO(Ctx.Logger()) << "OneCandidate: " << leaderBoard[0];
        } else {
            if (!ApplyHintFiltration) {
                if (ApplyThreshHoldFiltration = IsFoundConfidentScenario()) {
                    FilterThresholdAndAvoiding();
                    Table.SetStrongMarkersLossReason(LR_CUT_BY_FORMULA);
                    LOG_INFO(Ctx.Logger()) << "Threshold cut applied";
                }
            }
        }
        if (ApplyThreshHoldFiltration || ApplyHintFiltration) {
            AvoidAnyCut();
        }
    }

    leaderBoard = Table.GetWinners(QualityStorage);
    LOG_INFO(Ctx.Logger()) << "PreClassification result: " << JoinRange(/* delim= */ TStringBuf(", "), leaderBoard.begin(), leaderBoard.end());
    TScenarioToRequestFrames resultingCandidateToRequestFrames;
    for (const auto& candidate : CandidateToRequestFrames) {
        if (IsIn(leaderBoard, candidate.first->GetScenario().GetName())) {
            resultingCandidateToRequestFrames.insert(candidate);
        }
    }
    return resultingCandidateToRequestFrames;
}

bool TClassificationProcessor::ClsfForcedOrBoostedScenario() {
    const auto forcedScenarioName = GetExperimentValueWithPrefix(Ctx.ExpFlags(), EXP_PREFIX_MM_FORCE_SCENARIO);
    if (forcedScenarioName.Defined()) {
        Table.SetDirectMarker(forcedScenarioName.GetRef(), EClassificationMarker::Forced);
        Table.SetDirectMarkersLossReason(LR_FORCED_SCENARIO);
        LOG_INFO(Ctx.Logger()) << "ForcedScenario: " << forcedScenarioName.GetRef();

        if (BoostedScenario.Defined() && BoostedScenario->GetName() != forcedScenarioName.GetRef()) {
            // There will be two different direct markers so preclassify result will be empty
            Table.SetDirectMarker(BoostedScenario->GetName(), EClassificationMarker::Boosted);
            LOG_ERR(Ctx.Logger()) << "BoostedScenario (" << BoostedScenario->GetName() << ") != ForcedScenario ("
                                  << *forcedScenarioName << ")";
        }
        return true;
    }
    if (BoostedScenario.Defined()) {
        if (Ctx.HasExpFlag(EXP_PREFIX_MM_DISABLE_PROTOCOL_SCENARIO + BoostedScenario->GetName())) {
            // Preclassification result will be empty
            Table.SetEmptyDirectMarker(EClassificationMarker::Boosted);
        } else {
            Table.SetDirectMarker(BoostedScenario->GetName(), EClassificationMarker::Boosted);
            LOG_INFO(Ctx.Logger()) << "BoostedScenario: " << BoostedScenario->GetName();
        }
        Table.SetDirectMarkersLossReason(LR_BOOSTED_SCENARIO);
        return true;
    }
    return false;
}

void TClassificationProcessor::ClsfAllowedByMinRequirements() {
    const auto onlyEnabledScenario = GetExperimentValueWithPrefix(Ctx.ExpFlags(), EXP_PREFIX_MM_SCENARIO);
    if (onlyEnabledScenario.Defined()) {
        LOG_INFO(Ctx.Logger()) << "The Only Enabled Scenario: " << *onlyEnabledScenario;
    }

    for (const auto marker : {EClassificationMarker::AllowedByFixlistForTurkish, EClassificationMarker::AcceptsInput,
        EClassificationMarker::SupportsLanguage, EClassificationMarker::IsEnabled,
        EClassificationMarker::ShouldNotFilterSpecificScenario})
    {
        Table.SetEmptyNecessaryMarker(marker);
    }
    for (const auto& [candidate, frames] : CandidateToRequestFrames) {
        const auto& scenario = candidate->GetScenario();
        const auto& scenarioName = scenario.GetName();
        const bool supportsLanguage = IsScenarioAllowedByLanguage(Ctx, scenario);
        const bool isActiveScenario = IsActiveScenario(Ctx, scenarioName) ||
            IsInPlayerOwnerPriorityMode(Ctx, scenarioName);
        const bool isWaitingForSlotValue = NImpl::IsWaitingForSlotValue(scenario, Ctx);
        const bool acceptsAnyUtterance = scenario.AcceptsAnyUtterance() && Event.HasUtterance() && !Event.GetUtterance().empty();
        const bool acceptsFrames = !frames.empty()
            || acceptsAnyUtterance
            || isWaitingForSlotValue
            || isActiveScenario;

        const bool isMediaInput = Event.IsImageInput() || Event.IsMusicInput();

        const bool acceptsImage = (scenario.AcceptsImageInput() || isActiveScenario) && Event.IsImageInput();
        const bool acceptsMusic = (scenario.AcceptsMusicInput() || isActiveScenario) && Event.IsMusicInput();

        const auto acceptsInput = (acceptsFrames && !isMediaInput) || acceptsImage || acceptsMusic;

        const bool allowedByFixlist =
            Ctx.Language() != ELanguage::LANG_TUR ||
            WizardResponse.GetFixlist().IsRequestAllowedForScenario(scenarioName);

        CanUsePreclassifierHint = CanUsePreclassifierHint && !isWaitingForSlotValue && !isActiveScenario;

        if (allowedByFixlist) {
            Table.SetNecessaryMarker(scenarioName, EClassificationMarker::AllowedByFixlistForTurkish);
        } else {
            Table.SetNecessaryMarkersLossReason(scenarioName, LR_FIXLIST_RESTRICTION);
        }
        if (acceptsInput) {
            Table.SetNecessaryMarker(scenarioName, EClassificationMarker::AcceptsInput);
        } else {
            Table.SetNecessaryMarkersLossReason(scenarioName, LR_DOESNT_ACCEPT_INPUT);
        }
        if (supportsLanguage) {
            Table.SetNecessaryMarker(scenarioName, EClassificationMarker::SupportsLanguage);
        } else {
            Table.SetNecessaryMarkersLossReason(scenarioName, LR_UNSUPPORTED_LANGUAGE);
        }
        if (onlyEnabledScenario.Defined()) {
            if (scenarioName == *onlyEnabledScenario) {
                Table.SetNecessaryMarker(scenarioName, EClassificationMarker::IsEnabled);
            }  else {
                Table.SetNecessaryMarkersLossReason(scenarioName, LR_NOT_ENABLED);
            }
        } else {
            if (!Ctx.HasExpFlag(EXP_PREFIX_MM_DISABLE_PROTOCOL_SCENARIO + scenarioName) && scenario.IsEnabled(Ctx)) {
                Table.SetNecessaryMarker(scenarioName, EClassificationMarker::IsEnabled);
            } else {
                Table.SetNecessaryMarkersLossReason(scenarioName, LR_NOT_ENABLED);
            }
        }
        if (!(NImpl::ShouldFilterMiscHollywoodMusic(Ctx, scenario)
            || NImpl::ShouldFilterProtocolVideoScenario(Ctx, scenario)
            || NImpl::ShouldFilterNewsScenario(Ctx, scenario, frames, WizardResponse)
            || NImpl::ShouldFilterSideSpeechScenario(Ctx, scenario)))
        {
            Table.SetNecessaryMarker(scenarioName, EClassificationMarker::ShouldNotFilterSpecificScenario);
        } else {
            Table.SetNecessaryMarkersLossReason(scenarioName, LR_SHOULD_FILTER_SCENARIO);
        }
    }
}

void TClassificationProcessor::FilterHints() {
    const auto& classificationConfig = Ctx.ClassificationConfig().GetScenarioClassificationConfigs();

    for (const auto& [candidate, frames] : CandidateToRequestFrames) {
        const auto& scenarioName = candidate->GetScenario().GetName();
        if (!Table.AllNecessary(scenarioName)) {
            continue;
        }
        const auto& scenarioClassificationConfig = classificationConfig.find(scenarioName);
        if (FramesIntersectRecognizedActionEffectFrames(frames, Request.GetRecognizedActionEffectFrames())
            && !Ctx.HasExpFlag(EXP_DO_NOT_FORCE_ACTION_EFFECT_FRAME_WHEN_NO_UTTERANCE_UPDATE)) {
            LOG_INFO(Ctx.Logger()) << "RecognizedActionEffectFrames "
                                   << GetFrameNameListString(Request.GetRecognizedActionEffectFrames())
                                   << " were used as preclassifierHint for " << scenarioName;
            Table.SetStrongMarker(candidate->GetScenario().GetName(), EClassificationMarker::FramesIntersectRecognizedAction);
        }
        if (scenarioClassificationConfig == classificationConfig.end()) {
            continue;
        }
        for (const auto& hintFrameName : scenarioClassificationConfig->second.GetPreclassifierHint()) {
            if (NImpl::ShouldSkipPreclassifierHint(Ctx, scenarioName, hintFrameName)) {
                LOG_DEBUG(Ctx.Logger()) << "PreclassifierHint " << hintFrameName
                    << " was skipped for " << scenarioName;
                continue;
            }
            if (Ctx.Responses().WizardResponse().GetRequestFrame(hintFrameName)) {
                LOG_INFO(Ctx.Logger()) << "PreclassifierHint " << hintFrameName
                    << " was used for " << scenarioName;
                Table.SetStrongMarker(scenarioName, EClassificationMarker::ScenarioWithHint);
            }
        }
    }
}

void TClassificationProcessor::AvoidHintsCut() {
    const auto& classificationConfig = Ctx.ClassificationConfig().GetScenarioClassificationConfigs();
    for (const auto& [candidate, frames] : CandidateToRequestFrames) {
        const auto& scenarioName = candidate->GetScenario().GetName();
        const auto& scenarioClassificationConfig = classificationConfig.find(scenarioName);
        if (scenarioClassificationConfig == classificationConfig.end()) {
            continue;
        }
        if (scenarioClassificationConfig->second.GetIgnorePreclassifierHints()) {
            Table.SetStrongMarker(scenarioName, EClassificationMarker::IgnoreHintsOnClient);
        }
    }
}

bool TClassificationProcessor::IsFoundConfidentScenario() {
    const auto& classificationConfig = Ctx.ClassificationConfig().GetScenarioClassificationConfigs();
    const auto clientType = GetClientType(Ctx.ClientFeatures());
    const TMaybe<TStringBuf> experiment = NMegamind::GetMMFormulaExperimentForSpecificClient(Ctx.ExpFlags(), clientType);

    bool foundConfidentScenario = false;

    auto& storagePredicts = *QualityStorage.MutablePreclassifierPredicts();

    for (const auto& [candidate, frames] : CandidateToRequestFrames) {
        const TString& scenarioName = candidate->GetScenario().GetName();
        if (!Table.AllNecessary(scenarioName)) {
            continue;
        }
        const auto& wizardResponse = Ctx.Responses().WizardResponse();

        double predict = 0;
        const TString confidentFrame = NImpl::CheckScenarioConfidentFrames(classificationConfig, wizardResponse, scenarioName, Ctx.ExpFlags());
        if (!Ctx.HasExpFlag(EXP_DISABLE_PRECLASSIFIER_CONFIDENT_FRAMES) && confidentFrame) {
            LOG_INFO(Ctx.Logger()) << "Setting preclassifier predict = inf on scenario "
                                << scenarioName << " due to the confident frame "
                                << confidentFrame;
            predict = INFINITY;
        } else if (FramesIntersectRecognizedActionEffectFrames(frames, Request.GetRecognizedActionEffectFrames())) {
            LOG_INFO(Ctx.Logger()) << "Setting preclassifier predict = inf on scenario "
                                << scenarioName << " due to the recognized action frame";
            predict = INFINITY;
        } else {
            predict = ApplyScenarioFormula(
                FormulasStorage,
                scenarioName,
                ECS_PRE,
                clientType,
                experiment.GetOrElse({}),
                FactorStorage,
                Ctx.Logger(),
                Ctx.LanguageForClassifiers()
            );
        }

        storagePredicts[scenarioName] = predict;
        const double confidentScenarioThreshold = GetScenarioConfidentThreshold(
            FormulasStorage, scenarioName, ECS_PRE, clientType, Ctx.ExpFlags(), Ctx.ClassificationConfig(), Ctx.Logger(), Ctx.LanguageForClassifiers()
        );
        if (predict > confidentScenarioThreshold) {
            foundConfidentScenario = true;
            LOG_INFO(Ctx.Logger()) << "Preclassifier is confident about " << scenarioName;
        }
    }
    return foundConfidentScenario;
}

void TClassificationProcessor::FilterThresholdAndAvoiding() {
    bool shouldAvoidCuttingByThresholdForCtx = NImpl::ShouldAvoidCuttingByThreshold(Ctx);
    const auto clientType = GetClientType(Ctx.ClientFeatures());

    const TMaybe<TStringBuf> experiment = NMegamind::GetMMFormulaExperimentForSpecificClient(Ctx.ExpFlags(), clientType);
    const auto expThresholds = NImpl::GetExpThresholds(Ctx.ExpFlags(), Ctx.Logger());

    auto& storagePredicts = *QualityStorage.MutablePreclassifierPredicts();

    for (const auto& [candidate, frames] : CandidateToRequestFrames) {
        const TString& scenarioName = candidate->GetScenario().GetName();
        const double predict = storagePredicts[scenarioName];

        auto expThreshold = expThresholds.FindPtr(scenarioName);
        auto threshold = expThreshold ? *expThreshold
                                        : NMegamind::GetScenarioThreshold(FormulasStorage, scenarioName, ECS_PRE, clientType,
                                                                        experiment.GetOrElse({}), Ctx.LanguageForClassifiers());
        bool avoidThresholdCut = false;
        if (shouldAvoidCuttingByThresholdForCtx) {
            Table.SetStrongMarker(scenarioName, EClassificationMarker::ShouldAvoidCuttingByThresholdForCtx);
            avoidThresholdCut = true;
        }
        if (NImpl::IsWaitingForSlotValue(candidate->GetScenario(), Ctx)) {
            Table.SetStrongMarker(scenarioName, EClassificationMarker::WaitsForSlotValue);
            avoidThresholdCut = true;
        }
        if (IsActiveScenario(Ctx, scenarioName)) {
            Table.SetStrongMarker(scenarioName, EClassificationMarker::ActiveScenario);
            avoidThresholdCut = true;
        }
        if (predict >= threshold) {
            Table.SetStrongMarker(scenarioName, EClassificationMarker::ExceedThreshold);
        } else {
            Table.SetEmptyStrongMarker(EClassificationMarker::ExceedThreshold);
            if (!avoidThresholdCut) {
                LOG_INFO(Ctx.Logger()) << "Cutting by threshold " << threshold << " scenario " << scenarioName
                    << " with predict " << predict << " (Still could avoid cut due to fixlist or recognized actions)";
            }
        }
    }
}

void TClassificationProcessor::AvoidAnyCut() {
    for (const auto& [candidate, frames] : CandidateToRequestFrames) {
        const auto& scenarioName = candidate->GetScenario().GetName();
        if (IsInFixlist(scenarioName, Ctx, Request)) {
            Table.SetStrongMarker(scenarioName, EClassificationMarker::InFixlist);
        }
        if (FramesIntersectRecognizedActionEffectFrames(frames, Request.GetRecognizedActionEffectFrames())) {
            Table.SetStrongMarker(scenarioName, EClassificationMarker::FramesIntersectRecognizedAction);
        }
        if (scenarioName == MM_VIDEO_PROTOCOL_SCENARIO) {
            for (const auto& gallery : Ctx.Responses().WizardResponse().GetProtoResponse().GetAliceItemSelector().GetGalleries()) {
                for (const auto& item : gallery.GetItems()) {
                    if (item.GetIsSelected()) {
                        Table.SetStrongMarker(MM_VIDEO_PROTOCOL_SCENARIO, EClassificationMarker::SaveVideoWithItemSelector);
                    }
                }
            }
        }
    }
}

TMaybe<bool> PreClassifyPartialImpl(const IContext& ctx, const TFactorStorage& factorStorage) {
    static constexpr size_t EMBEDDING_SIZE = 120;

    LOG_INFO(ctx.Logger()) << "PreClassifyPartial start";

    TVector<float> partialPreFeatures;
    const TPartialPreCalcer& calcer = ctx.GetPartialPreClassificationCalcer();

    const TVector<const TFactorStorage*> fsv = {&factorStorage};
    TVector<TConstArrayRef<float>> finalFeatures;
    calcer.GetSlicedFeatures(fsv.data(), 1, &partialPreFeatures, &finalFeatures);

    if (Y_UNLIKELY(partialPreFeatures.size() < EMBEDDING_SIZE)) {
        LOG_ERROR(ctx.Logger()) << "Bad size of final vector: " << partialPreFeatures.size();
        return Nothing();
    }

    const TString* embedsResponse = nullptr;
    if (ctx.Responses().WizardResponse().GetProtoResponse().GetAlicePartialPreEmbedding().HasSerializedEmbedding()) {
        embedsResponse = &ctx.Responses().WizardResponse().GetProtoResponse().GetAlicePartialPreEmbedding().GetSerializedEmbedding();
    }

    if (!embedsResponse) {
        LOG_ERROR(ctx.Logger()) << "No embedding from begemot";
        return Nothing();
    }

    TString decodedProto;
    try {
        decodedProto = Base64StrictDecode(*embedsResponse);
    } catch (yexception& ex) {
    }

    NAlice::TPartialsPreEmbedding utteranceEmbeddings;
    if (decodedProto.Empty() || !utteranceEmbeddings.ParseFromString(decodedProto)
        || (utteranceEmbeddings.ValuesSize() != EMBEDDING_SIZE)
    ) {
        LOG_ERROR(ctx.Logger()) << "Incorrect embedding: " << *embedsResponse;
        return Nothing();
    }

    MemCopy(partialPreFeatures.end() - EMBEDDING_SIZE, utteranceEmbeddings.GetValues().data(), EMBEDDING_SIZE);

    TVector<double> modelResults(1);
    modelResults[0] = calcer.CalcRelev(partialPreFeatures);
    const auto prob = CalcSigmoid(MakeConstArrayRef(modelResults))[0];
    LOG_INFO(ctx.Logger()) << "Partial preclassifier result: " << prob;

    double threshold = 0.0;
    if (auto flag = GetExperimentValueWithPrefix(ctx.ExpFlags(), EXP_PARTIAL_PRECLASSIFIER_THRESHOLD_PREFIX);
        flag && TryFromString<double>(*flag, threshold)
    ) {
        return prob < threshold;
    } else {
        LOG_ERROR(ctx.Logger()) << "No threshold value for partial preclassifier";
        return Nothing();
    }
}

} // namespace NImpl

void PreClassify(TScenarioToRequestFrames& candidateToRequestFrames, const TRequest& request, const IContext& ctx,
                 const TFormulasStorage& formulasStorage, const TFactorStorage& factorStorage,
                 TQualityStorage& qualityStorage) {
    NImpl::TClassificationProcessor PreclassificationProcessor(candidateToRequestFrames, request, ctx, formulasStorage, factorStorage, qualityStorage);
    candidateToRequestFrames = PreclassificationProcessor.PreClassify();

    for (const auto& candidate : candidateToRequestFrames) {
        ctx.Sensors().IncRate(NSignal::LabelsForInvokedScenario(candidate.first->GetScenario().GetName()));
    }
}

bool PreClassifyPartial(const IContext& ctx, const TFactorStorage& factorStorage) {
    using namespace NSignal;

    if (!ctx.HasExpFlag(EXP_ENABLE_PARTIAL_PRECLASSIFIER)) {
        return false;
    }

    bool isSmartSpeaker = ctx.ClientFeatures().IsSmartSpeaker();

    if (!isSmartSpeaker || ctx.SpeechKitRequest().IsEOU().GetOrElse(true)) {
        ctx.Sensors().IncRate(LabelsForPartialPreClassificationReqs(isSmartSpeaker, EPartialPreStatus::Skip));
        LOG_INFO(ctx.Logger()) << "PreClassifyPartial skip";
        return false;
    }

    const auto result = NImpl::PreClassifyPartialImpl(ctx, factorStorage);
    if (result) {
        ctx.Sensors().IncRate(LabelsForPartialPreClassificationReqs(isSmartSpeaker,
                                                                    *result ? EPartialPreStatus::Filtered
                                                                            : EPartialPreStatus::Left));
        return *result;
    } else {
        ctx.Sensors().IncRate(LabelsForPartialPreClassificationErrors(isSmartSpeaker));
        ctx.Sensors().IncRate(LabelsForPartialPreClassificationReqs(isSmartSpeaker, EPartialPreStatus::Left));
        return false;
    }
}

} // namespace NAlice
