#pragma once

#include <alice/hollywood/library/base_scenario/scenario.h>
#include <alice/hollywood/library/frame_redirect/frame_redirect.h>
#include <alice/hollywood/library/http_proxy/http_proxy.h>
#include <alice/megamind/protos/scenarios/request.pb.h>
#include <alice/megamind/protos/scenarios/response.pb.h>

#include <alice/hollywood/library/music/music_resources.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/music_queue/music_queue.h>
#include <alice/hollywood/library/scenarios/music/proto/music_context.pb.h>

namespace NAlice::NHollywood::NMusic::NImpl {

class TRunPrepareHandleImpl {
public:
    // result types:
    // (1) THttpProxyRequest - BASS run request
    // (2) TScenarioRunResponse - quick scenario response
    // (3) TString - subgraph flag
    using TResult = std::variant<THttpProxyRequest, NScenarios::TScenarioRunResponse, TString>;

public:
    TRunPrepareHandleImpl(TScenarioHandleContext& ctx);

    // main router function
    TResult Do();

private:
    // impl.cpp
    TScenarioState& GetScenarioStateOrDummy();
    const TFrame* FindFrame(TStringBuf frameName);
    const TSemanticFrame* FindSemanticFrame(TStringBuf frameName); // please only use this in exceptional cases
    void AddPrefixToSearchText(const TString& prefix, const TSlot& searchText);
    void PatchFrameIfNeeded(TFrame& frame);

    // player_command.cpp
    TMaybe<TResult> HandlePlayerCommand();

    // radio.cpp
    TMaybe<NScenarios::TScenarioRunResponse> HandleThinClientRadio();

    // stream.cpp
    TMaybe<NScenarios::TScenarioRunResponse> TryAddStreamSlotWithFallback();

    // onboarding.cpp
    TMaybe<NScenarios::TScenarioRunResponse> HandleOnboardingFrames();
    TMaybe<NScenarios::TScenarioRunResponse> TryHandleMusicComplexLikeDislike(bool& stop, bool& isLike);
    TMaybe<NScenarios::TScenarioRunResponse> TryHandleMusicComplexLikeDislikeNoSearch();
    TMaybe<NScenarios::TScenarioRunResponse> TryHandleMusicComplexLikeDislikeWithSearch();

    // reask.cpp
    TMaybe<NScenarios::TScenarioRunResponse> HandleReask();

    // fairy_tale.cpp
    TMaybe<NScenarios::TScenarioRunResponse> HandleFairyTale();
    TMaybe<NScenarios::TScenarioRunResponse> HandleFairyTaleOnDemand();

    // object.cpp
    struct TMusicPlayObjectParams {
        const bool IsFairyTalePlaylistFrame = false;
        const bool IsBedtimeTales = false;
    };
    NScenarios::TScenarioRunResponse HandleThinClientMusicPlayObject(TMusicPlayObjectParams params);
    NScenarios::TScenarioRunResponse HandleThinClientMusicPlayObject();
    TMaybe<NScenarios::TScenarioRunResponse> TryHandleThinClientMusicPlayObject(); // calls HandleThinClientMusicPlayObject if possible

    // callback.cpp
    TMaybe<NScenarios::TScenarioRunResponse> HandleCallback();
    NScenarios::TScenarioRunResponse HandleNextTrackCallback();
    NScenarios::TScenarioRunResponse HandleTrackPlayLifeCycleCallback();
    NScenarios::TScenarioRunResponse HandleRecoveryCallback();

    NScenarios::TScenarioRunResponse HandleOnboardingOnPlayDeclineCallback();
    NScenarios::TScenarioRunResponse HandleOnboardingOnTracksGameDeclineCallback();
    NScenarios::TScenarioRunResponse HandleOnboardingOnDontKnowCallback();
    NScenarios::TScenarioRunResponse HandleOnboardingOnSilenceCallback();
    NScenarios::TScenarioRunResponse HandleOnboardingOnRepeatedSkipDeclineCallback();

    // responses.cpp
    NScenarios::TScenarioRunResponse CreateIrrelevantSilentResponse();
    NScenarios::TScenarioRunResponse CreateIrrelevantResponseMusicNotFound();
    NScenarios::TScenarioRunResponse CreateNotSupportedResponse(TStringBuf phraseName);
    NScenarios::TScenarioRunResponse MakeBassRadioResponseWithContinueArguments(TStringBuf radioStationId);
    NScenarios::TScenarioRunResponse MakeThinClientDefaultResponseWithEmptyContinueArguments();
    NScenarios::TScenarioRunResponse MakeThinClientDefaultResponseWithEmptyApplyArguments();

    // multiroom.cpp
    void TryFillActivateMultiroomSlot(TFrame& frame);

    // bass.cpp
    THttpProxyRequest CreateBassRunRequest();

    // subgraph.cpp
    TMaybe<TString> TryGetSubgraphFlag();

private:
    // base request classes
    TScenarioHandleContext& Ctx_;
    TRTLogger& Logger_;
    const NScenarios::TRequestMeta& Meta_;
    const NJson::TJsonValue& AppHostParams_;
    const NScenarios::TScenarioRunRequest RequestProto_;
    const TScenarioRunRequestWrapper Request_;
    TNlgWrapper Nlg_;
    const TMusicResources& MusicResources_;

    // more specific request-unique classes
    const TBlackBoxUserInfo* UserInfo_;
    TMaybe<TScenarioState> ScenarioState_;
    TScenarioState EmptyScenarioStateDummy_;
    TMusicQueueWrapper MusicQueue_;
    TFrame Frame_; // this will be sent to BASS

    // parsed frames holder
    THashMap<TString, std::unique_ptr<TFrame>> Frames_;
    THashMap<TString, const TSemanticFrame*> SemanticFrames_;

    bool HasMusicPlayAmbientSoundRequest_ = false;
};

} // namespace NAlice::NHollywood::NMusic::NImpl
