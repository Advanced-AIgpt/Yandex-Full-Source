#include "impl.h"

namespace NAlice::NHollywood::NMusic::NImpl {

TMaybe<NScenarios::TScenarioRunResponse> TRunPrepareHandleImpl::HandleOnboardingFrames() {
    // exp required
    if (!Request_.HasExpFlag(NExperiments::EXP_HW_MUSIC_ONBOARDING)) {
        return Nothing();
    }

    if (ScenarioState_.Defined() && Request_.IsNewSession()) {
        LOG_INFO(Logger_) << "Reset music onboarding state on new session";
        ScenarioState_->ClearOnboardingState();
    }

    if (FindFrame(MUSIC_ONBOARDING_ARTISTS_FRAME)) {
        return HandleMusicOnboardingArtists(Ctx_, Request_, Nlg_, GetScenarioStateOrDummy());
    }

    if (FindFrame(MUSIC_ONBOARDING_GENRES_FRAME)) {
        return HandleMusicOnboardingGenres(Ctx_, Request_, Nlg_, GetScenarioStateOrDummy());
    }

    if (IsThinRadioSupported(Request_)) {
        if (FindFrame(MUSIC_ONBOARDING_TRACKS_FRAME)) {
            return HandleMusicOnboardingTracks(Ctx_, Request_, Nlg_, GetScenarioStateOrDummy());
        }
        if (const auto frame = FindFrame(MUSIC_ONBOARDING_TRACKS_REASK_FRAME)) {
            const auto trackIndexSlot = frame->FindSlot(NAlice::NMusic::SLOT_TRACK_INDEX);

            int trackIndex = -1;
            if (ScenarioState_.Defined()) {
                const auto& onboardingState = ScenarioState_->GetOnboardingState();
                if (onboardingState.OnboardingSequenceSize() && onboardingState.GetOnboardingSequence(0).HasTracksGame()) {
                    trackIndex = onboardingState.GetOnboardingSequence(0).GetTracksGame().GetTrackIndex();
                }
            }

            if (!Request_.HasExpFlag(EXP_HW_MUSIC_ONBOARDING_TRACKS_REASK) ||
                !InTracksGame(Logger_, Request_.BaseRequestProto().GetDeviceState(), GetScenarioStateOrDummy()) ||
                (trackIndexSlot != nullptr && trackIndexSlot->Value.As<int>() != trackIndex))
            {
                const auto response = TRunResponseBuilder::MakeIrrelevantResponse(Nlg_, {}, ConstructBodyRenderer(Request_));
                if (ScenarioState_.Defined()) {
                    response->MutableResponseBody()->MutableState()->PackFrom(
                        *ScenarioState_
                    );
                }
                return *response;
            }
            return HandleMusicOnboardingTracksReask(Ctx_, Request_, Nlg_, GetScenarioStateOrDummy());
        }
    }

    if (FindFrame(MUSIC_ONBOARDING_FRAME)) {
        return HandleMusicMasterOnboarding(Ctx_, Request_, Nlg_, GetScenarioStateOrDummy());
    }

    return Nothing();
}

TMaybe<NScenarios::TScenarioRunResponse>
TRunPrepareHandleImpl::TryHandleMusicComplexLikeDislike(bool& stop, bool& isLike) {
    if (ScenarioState_.Defined() && Request_.IsNewSession()) {
        LOG_INFO(Logger_) << "Reset music onboarding state on new session";
        ScenarioState_->ClearOnboardingState();
    }

    if (ShouldFillDefaultOnboardingAction(Request_, Frame_, GetScenarioStateOrDummy().GetOnboardingState())) {
        AddActionRequestSlot(Frame_, ACTION_REQUEST_LIKE);
        LOG_INFO(Logger_) << "Filled like action in onboarding context";
    }

    stop = true;
    const auto actionRequest = Frame_.FindSlot(NAlice::NMusic::SLOT_ACTION_REQUEST);
    if (!actionRequest) {
        LOG_INFO(Logger_) << "No music action request";
        return Nothing();
    }
    if (!IsLikeDislikeAction(actionRequest->Value.AsString(), isLike)) {
        LOG_INFO(Logger_) << "Music action is not a like/dislike";
        return Nothing();
    }
    stop = false;

    if (IsAskingFavorite(GetScenarioStateOrDummy().GetOnboardingState())) {
        if (!Request_.HasExpFlag(NExperiments::EXP_HW_MUSIC_ONBOARDING)) {
            LOG_INFO(Logger_) << "Got a like/dislike action in asking favorite onboarding without a required flag";
            return CreateIrrelevantResponseMusicNotFound();
        }
    } else if (!Request_.HasExpFlag(NExperiments::EXP_HW_MUSIC_COMPLEX_LIKE_DISLIKE)) {
        // Non asking favorite onboarding is the same as other requests here, so we just check the common flag
        LOG_INFO(Logger_) << "Got a like/dislike action without a required flag";
        return CreateIrrelevantResponseMusicNotFound();
    }

    if (!HasMusicSubscription(Request_)) {
        LOG_INFO(Logger_) << "Can not like/dislike without MusicSubscription";
        return CreateOnboardingResponse(Ctx_, Request_, Nlg_, GetScenarioStateOrDummy(),
                                        "no_subscription__like_dislike");
    }

    return Nothing();
}

TMaybe<NScenarios::TScenarioRunResponse> TRunPrepareHandleImpl::TryHandleMusicComplexLikeDislikeNoSearch() {
    LOG_DEBUG(Logger_) << "Created music play frame for genre onboarding: " << JsonStringFromProto(Frame_.ToProto());

    bool stop, isLike;
    if (auto resp = TryHandleMusicComplexLikeDislike(stop, isLike); resp || stop) {
        return std::move(resp);
    }

    const auto& onboardingState = GetScenarioStateOrDummy().GetOnboardingState();
    const bool askingFavorite = IsAskingFavorite(onboardingState);
    const auto genre = Frame_.FindSlot(NAlice::NMusic::SLOT_GENRE);
    if (askingFavorite && GetAskingFavorite(onboardingState).GetType() != TAskingFavorite::Genre) {
        if (genre) {
            LOG_INFO(Logger_) << "Can not like/dislike artist with a genre slot";
            auto paramsBuilder = TOnboardingResponseParamsBuilder{}
                .SetTryReask(true)
                .SetTryNextMasterOnboardingStage(true);
            return CreateOnboardingResponse(Ctx_, Request_, Nlg_, GetScenarioStateOrDummy(),
                                            "artist__no_search_slot", paramsBuilder.Build());
        }
        LOG_INFO(Logger_) << "Asking for an artist, proceed farther";
        return Nothing();
    }

    if (!genre) {
        if (askingFavorite) {
            LOG_INFO(Logger_) << "Can not like/dislike genre without a genre slot";
            auto paramsBuilder = TOnboardingResponseParamsBuilder{}
                .SetTryReask(true)
                .SetTryNextMasterOnboardingStage(true);
            return CreateOnboardingResponse(Ctx_, Request_, Nlg_, GetScenarioStateOrDummy(),
                                            "genre__no_slot", paramsBuilder.Build());
        }
        LOG_INFO(Logger_) << "No genre slot, proceed farther";
        return Nothing();
    }

    if (!isLike) {
        LOG_INFO(Logger_) << "Can not dislike genre";
        auto paramsBuilder = TOnboardingResponseParamsBuilder{}.SetTryNextMasterOnboardingStage(true);
        return CreateOnboardingResponse(Ctx_, Request_, Nlg_, GetScenarioStateOrDummy(),
                                        "unsupported_found__dislike", paramsBuilder.Build());
    }

    // No bass search required, we just take genre id from slot
    LOG_INFO(Logger_) << "Building like genre response";

    auto applyArgs = MakeMusicArguments(Logger_, Request_, TMusicArguments_EExecutionFlowType_ComplexLikeDislike,
                                              /* isNewContentRequestedByUser= */ false);
    auto& likeDislike = *applyArgs.MutableComplexLikeDislikeRequest();
    likeDislike.SetIsLike(true);
    likeDislike.MutableGenreTarget()->SetId(genre->Value.AsString());

    THwFrameworkRunResponseBuilder response{Ctx_, &Nlg_, ConstructBodyRenderer(Request_)};
    FillLikeDislikeResponse(response, Ctx_, Request_, GetScenarioStateOrDummy(), applyArgs);
    return *std::move(response).BuildResponse();
}

TMaybe<NScenarios::TScenarioRunResponse> TRunPrepareHandleImpl::TryHandleMusicComplexLikeDislikeWithSearch() {
    bool stop, isLike;
    if (auto resp = TryHandleMusicComplexLikeDislike(stop, isLike); resp || stop) {
        return std::move(resp);
    }

    const auto& onboardingState = GetScenarioStateOrDummy().GetOnboardingState();
    const bool askingFavorite = IsAskingFavorite(onboardingState);
    if (askingFavorite && GetAskingFavorite(onboardingState).GetType() != TAskingFavorite::Artist) {
        LOG_INFO(Logger_) << "Reached bass music search with genre onboarding";
        return CreateIrrelevantResponseMusicNotFound();
    }

    // We can not find anything we are able to like/dislike here without a search text
    // If we got a genre, we should not have reached here at all, cause we do not need bass for it
    const auto searchText = Frame_.FindSlot(NAlice::NMusic::SLOT_SEARCH_TEXT);
    if (!searchText) {
        LOG_INFO(Logger_) << "Can not like/dislike searchable without a search_text slot";
        auto paramsBuilder = TOnboardingResponseParamsBuilder{}
            .SetTryReask(true)
            .SetTryNextMasterOnboardingStage(true);
        return CreateOnboardingResponse(
            Ctx_, Request_, Nlg_, GetScenarioStateOrDummy(),
            askingFavorite ? "artist__no_search_slot" : TString::Join("unsupported_found__", isLike ? "like" : "dislike"),
            paramsBuilder.Build());
    }
    if (askingFavorite) {
        AddPrefixToSearchText("исполнитель", *searchText);
        LOG_INFO(Logger_) << "Extended search_text for onboarding";
        // NOTICE!! searchText is invalid after this
    }

    return Nothing();
}

} // namespace NAlice::NHollywood::NMusic::NImpl
