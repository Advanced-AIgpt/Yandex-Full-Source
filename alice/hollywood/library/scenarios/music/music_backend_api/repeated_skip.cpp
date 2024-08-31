#include "repeated_skip.h"

#include "music_common.h"

namespace NAlice::NHollywood::NMusic {

namespace {

constexpr ui64 DEFAULT_TIME_DELTA = 86400;

void RenderProposal(TResponseBodyBuilder& bodyBuilder, bool onboarding) {
    bodyBuilder.AddListenDirective();

    NScenarios::TFrameAction declineAction;
    declineAction.MutableNluHint()->SetFrameName(HINT_FRAME_DECLINE);
    if (onboarding) {
        declineAction.MutableCallback()->SetName(TString{MUSIC_ONBOARDING_ON_REPEATED_SKIP_DECLINE_CALLBACK});
    } else {
        declineAction.MutableParsedUtterance()->MutableTypedSemanticFrame()->MutablePlayerNextTrackSemanticFrame();
    }
    bodyBuilder.AddAction("repeated_skip_decline", std::move(declineAction));
}

bool IsThresholdExceeded(const TScenarioBaseRequestWrapper& request, const TRepeatedSkipState& state, const bool inOnboarding) {
    const auto thresholdPrefix = inOnboarding ? EXP_HW_MUSIC_ONBOARDING_REPEATED_SKIP_THRESHOLD_PREFIX : EXP_HW_MUSIC_REPEATED_SKIP_THRESHOLD_PREFIX;
    if (const auto optThreshold = request.GetValueFromExpPrefix(thresholdPrefix)) {
        size_t threshold = 0;
        if (TryFromString(*optThreshold, threshold)) {
            return state.GetSkipCount() >= threshold;
        }
    }
    return false;
}

bool IsTimePassed(const TScenarioBaseRequestWrapper& request, const TRepeatedSkipState& state, const bool inOnboarding) {
    const auto timestamp = state.GetProposalTimestamp();
    const auto epoch = request.ClientInfo().Epoch;
    const auto deltaPrefix = inOnboarding ? EXP_HW_MUSIC_ONBOARDING_REPEATED_SKIP_TIME_DELTA_PREFIX : EXP_HW_MUSIC_REPEATED_SKIP_TIME_DELTA_PREFIX;
    const auto timeDelta = request.LoadValueFromExpPrefix(deltaPrefix, DEFAULT_TIME_DELTA);
    return epoch >= timestamp && epoch - timestamp >= timeDelta;
}

bool GetExpectsRequestFlag(const TScenarioBaseRequestWrapper& request, const bool inOnboarding) {
    return request.HasExpFlag(inOnboarding ? EXP_HW_MUSIC_ONBOARDING_REPEATED_SKIP_EXPECTS_REQUEST : EXP_HW_MUSIC_REPEATED_SKIP_EXPECTS_REQUEST);
}

bool IsSkip(const TMusicArguments::EPlayerCommand playerCommand) {
    return playerCommand == TMusicArguments_EPlayerCommand_NextTrack ||
           playerCommand == TMusicArguments_EPlayerCommand_Dislike;
}

bool IsSkip(const TTypedSemanticFrame& tsf) {
    return tsf.HasPlayerNextTrackSemanticFrame() ||
           tsf.HasPlayerDislikeSemanticFrame();
}

} // namespace

TRepeatedSkip::TRepeatedSkip(TScenarioState& state, TRTLogger& logger)
    : State_(state)
    , Logger_(logger)
    , InOnboarding_(state.GetOnboardingState().GetInOnboarding())
{}

void TRepeatedSkip::IncreaseCount() {
    auto& skipState = *State_.MutableRepeatedSkipState();
    skipState.SetSkipCount(skipState.GetSkipCount() + 1);
    LOG_INFO(Logger_) << "Increased repeated skip count to " << skipState.GetSkipCount();
}

void TRepeatedSkip::ResetCount() {
    if (State_.GetRepeatedSkipState().GetSkipCount()) {
        State_.MutableRepeatedSkipState()->SetSkipCount(0);
        LOG_INFO(Logger_) << "Reset repeated skip count to 0";
    }
}

void TRepeatedSkip::HandlePlayerCommand(const TMusicArguments::EPlayerCommand playerCommand) {
    if (IsSkip(playerCommand)) {
        IncreaseCount();
    } else {
        ResetCount();
    }
}

void TRepeatedSkip::HandlePlayerCommand(const TTypedSemanticFrame& tsf) {
    if (IsSkip(tsf)) {
        IncreaseCount();
    } else {
        ResetCount();
    }
}

bool TRepeatedSkip::MayPropose(const TScenarioBaseRequestWrapper& request) const {
    const auto& repeatedSkipState = State_.GetRepeatedSkipState();
    return IsThresholdExceeded(request, repeatedSkipState, InOnboarding_) && IsTimePassed(request, repeatedSkipState, InOnboarding_);
}

void TRepeatedSkip::SaidProposal(const TScenarioBaseRequestWrapper& request) {
    State_.MutableRepeatedSkipState()->SetProposalTimestamp(request.ClientInfo().Epoch);
    ResetCount();
}

bool TRepeatedSkip::TryPropose(const TScenarioBaseRequestWithInputWrapper& request, TResponseBodyBuilder& bodyBuilder, TNlgData& nlgData) {
    if (request.Input().IsVoiceInput() && MayPropose(request)) {
        LOG_INFO(Logger_) << "Rendering repeated skip proposal";
        RenderProposal(bodyBuilder, InOnboarding_);
        bodyBuilder.SetExpectsRequest(GetExpectsRequestFlag(request, InOnboarding_));
        nlgData.Context["attentions"]["repeated_skip"] = true;
        nlgData.Context["attentions"]["onboarding"] = InOnboarding_;
        SaidProposal(request);
        return true;
    }
    return false;
}

} // namespace NAlice::NHollywood::NMusic
