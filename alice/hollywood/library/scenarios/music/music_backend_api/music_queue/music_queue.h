#pragma once

#include <alice/hollywood/library/scenarios/music/music_backend_api/music_config/music_config.h>
#include <alice/hollywood/library/scenarios/music/proto/music_arguments.pb.h>
#include <alice/hollywood/library/scenarios/music/proto/music_context.pb.h>

#include <alice/hollywood/library/request/request.h>

#include <alice/library/logger/logger.h>
#include <alice/library/util/rng.h>
#include <alice/protos/data/scenario/music/content_info.pb.h>
#include <util/string/builder.h>

namespace NAlice::NHollywood::NMusic {

enum struct ETrackChangeResult {
    SameTrack,
    TrackChanged,
    NeedStateUpdate,
    EndOfContent,
};

enum struct EContentError {
    NoError = 0, // default value
    Forbidden = 1,
    RestrictedByChild = 2,
    NotFound = 3,
};

enum struct EContentAttention {
    NoAttention = 0, // default value
    ContainsAdultContent = 1,
    ExplicitContentFiltered = 2,
    MayContainExplicitContent = 3,
    ForbiddenPodcast = 4,
};

struct TContentErrorsAndAttentions {
    EContentError Error = EContentError::NoError;
    EContentAttention Attention = EContentAttention::NoAttention;
};

class TMusicQueueWrapper {
public:

    TMusicQueueWrapper(TRTLogger& logger, TMusicQueue& musicQueue)
        : Logger_(logger)
        , Proto_(musicQueue) {
    }

public:

    void InitPlaybackFromContext(const TPlaybackContext& playbackContext);
    void InitPlayback(const TContentId& contentId, IRng& rng,
                      const TMusicArguments::TPlaybackOptions& playbackOptions = Default<TMusicArguments::TPlaybackOptions>(),
                      bool enableShots = false,
                      const NData::NMusic::TContentInfo& contentInfo = Default<NData::NMusic::TContentInfo>());

    void SetContentId(const TContentId& contentId);

    void ShufflePlayback(IRng& rng);

    void RepeatPlayback(ERepeatType repeatType);

    ERepeatType GetRepeatType() const;

    bool GetShuffle() const;

    i32 GetShuffleSeed() const;

    void SetEnableShots(bool enableShots);

    bool HasShotsEnabled() const;

    void SetIsEndOfContent(bool isEndOfContent);

    bool HasCurrentItemOrHistory() const;

    bool HasCurrentItem() const;

    const TQueueItem& CurrentItem() const;

    bool HasPreviousItem() const;

    const TQueueItem& PreviousItem() const;

    bool HasNextItem() const;

    const TQueueItem& NextItem() const;

    TQueueItem& MutableCurrentItem();

    bool IsCurrentTrackLast() const;

    ETrackChangeResult ChangeToNextTrack();

    ETrackChangeResult ChangeToPrevTrack();

    bool NeedToChangeState() const;

    void ChangeState(bool moveFromQueueToHistory = true);

    TMaybe<bool> GetMetadataShuffled() const;

    TMaybe<ERepeatType> GetMetadataRepeatType() const;

    i32 QueueSize() const {
        // How many items are in the queue, including currently playing item.
        return Proto_.GetQueue().size() + HasCurrentItemOrHistory();
    }

    void ClearQueue() {
        Proto_.MutableQueue()->Clear();
    }

    i32 TotalTracks() const {
        Y_ENSURE(IsPaged());
        return Proto_.GetCurrentContentLoadingState().GetPaged().GetTotalTracks();
    }

    bool TryAddItem(TQueueItem&& item, bool hasMusicSubscription);

    i32 NextPageIndex() const {
        Y_ENSURE(Proto_.GetNextContentLoadingState().HasPaged());
        return Proto_.GetNextContentLoadingState().GetPaged().GetPageIdx();
    }

    i32 CurrentPageIndex() const {
        Y_ENSURE(Proto_.GetCurrentContentLoadingState().HasPaged());
        return Proto_.GetCurrentContentLoadingState().GetPaged().GetPageIdx();
    }

    void SetNextTotalTracks(i32 totalTracks) {
        Proto_.MutableNextContentLoadingState()->MutablePaged()->SetTotalTracks(Max(totalTracks, 0));
    }

    void SetNextPageIndex(const i32 pageIdx) {
        Proto_.MutableNextContentLoadingState()->MutablePaged()->SetPageIdx(pageIdx);
    }

    // Used in recovery callback handler
    void SetOldRadioBatchId(const TString& batchId) {
        Proto_.MutableCurrentContentLoadingState()->MutableRadio()->SetBatchId(batchId);
    }

    void SetRadioBatchId(const TString& batchId) {
        Proto_.MutableNextContentLoadingState()->MutableRadio()->SetBatchId(batchId);
    }

    const TString& GetRadioBatchId() const {
        Y_ENSURE(IsRadio());
        return Proto_.GetCurrentContentLoadingState().GetRadio().GetBatchId();
    }

    // Used in recovery callback handler
    void SetOldRadioSessionId(const TString& sessionId) {
        Proto_.MutableCurrentContentLoadingState()->MutableRadio()->SetSessionId(sessionId);
    }

    void SetRadioSessionId(const TString& sessionId) {
        Proto_.MutableNextContentLoadingState()->MutableRadio()->SetSessionId(sessionId);
    }

    const TString& GetRadioSessionId() const {
        Y_ENSURE(IsRadio());
        return Proto_.GetCurrentContentLoadingState().GetRadio().GetSessionId();
    }

    void SetConfig(const TMusicConfig& config);

    TMusicConfig Config() const {
        return {
            Proto_.GetConfig().GetPageSize(),
            Proto_.GetConfig().GetHistorySize(),
            Proto_.GetConfig().GetExplicitFilteredOutWarningRate(),
            Proto_.GetConfig().GetFindTrackIdxPageSize()
        };
    }

    size_t GetActualPageSize(const TMusicContext& mCtx) const {
        return mCtx.GetFirstRequestPageSize() ? mCtx.GetFirstRequestPageSize() : Config().PageSize;
    }

    void SetFiltrationMode(NAlice::NScenarios::TUserPreferences_EFiltrationMode filtrationMode);

    NAlice::NScenarios::TUserPreferences_EFiltrationMode FiltrationMode() const;

    // This method only have sense after content was added to the queue!
    i32 GetFilteredOut() const {
        return FilteredOut_;
    }

    // This method only have sense after content was added to the queue!
    i32 GetAddedItemsCount() const {
        return AddedItemsCount_;
    }

    // This method only have sense after content was added to the queue!
    bool HaveExplicitContent() const {
        return HaveExplicitContent_;
    }

    // This method only have sense after content was added to the queue!
    bool HaveNonChildSafeContent() const {
        return HaveNonChildSafe_;
    }

    // This method only have sense after content was added to the queue!
    bool HavePodcastContent() const {
        return HavePodcastContent_;
    }

    const TContentId& ContentId() const;

    void SetContentInfo(const NData::NMusic::TContentInfo& contentInfo);

    const NData::NMusic::TContentInfo& ContentInfo() const;

    TVector<TStringBuf> ContentIdsValues() const;

    static TString ArtistName(const TQueueItem& item);

    TContentErrorsAndAttentions CalcContentErrorsAndAttentions() const;
    void CalcContentErrorsAndAttentions(TMusicContext& mCtx) const;

    bool IsPaged() const {
        return Proto_.GetCurrentContentLoadingState().HasPaged();
    }

    bool IsRadio() const {
        return Proto_.GetCurrentContentLoadingState().HasRadio();
    }

    bool IsGenerative() const {
        return Proto_.GetCurrentContentLoadingState().HasGenerative();
    }

    bool IsFmRadio() const {
        return Proto_.GetCurrentContentLoadingState().HasFmRadio();
    }

    void SetIsRadioPumpkin(bool isRadioPumpkin) {
        Proto_.SetIsRadioPumpkin(isRadioPumpkin);
    }

    bool IsRadioPumpkin() const {
        return Proto_.GetIsRadioPumpkin();
    }

    TString MakeFrom() const;

    void AddShotBeforeTrack(const TStringBuf trackId, TExtraPlayable_TShot&& shot);

    // To prevent shots re-request on prev/next commands
    // just mark a slot with empty queue
    void MarkBeforeTrackSlot(const TStringBuf trackId);

    // Set field Played in specific shot
    void SetShotPlayed(const TQueueItem& item, bool played, bool onlyFirstAvailable);

    // Get a shot if available
    TMaybe<TExtraPlayable_TShot> GetShotBeforeCurrentItem() const;

    TMaybe<TExtraPlayable_TShot> GetLastShotOfCurrentItem() const;

    bool HasExtraBeforeCurrentItem() const;

    bool IsAutoflowDisabled() const {
        return Proto_.GetPlaybackContext().GetDisableAutoflow();
    }

    bool IsHistoryDisabled() const {
        return Proto_.GetPlaybackContext().GetDisableHistory();
    }

    void SetPlaySingleTrack(bool on) {
        Proto_.MutablePlaybackContext()->SetPlaySingleTrack(on);
    }

    bool IsPlayingSingleTrack() const {
        return Proto_.GetPlaybackContext().GetPlaySingleTrack();
    }

    bool IsNlgDisabled() const {
        return Proto_.GetPlaybackContext().GetDisableNlg();
    }

    ui32 GetTrackOffsetIndex() const {
        return Proto_.GetPlaybackContext().GetTrackOffsetIndex();
    }

    void SetTrackOffsetIndex(ui32 trackOffsetIndex) {
        Proto_.MutablePlaybackContext()->SetTrackOffsetIndex(trackOffsetIndex);
    }

    ui32 GetPagedFirstTrackOffsetIndex(const TMusicContext& mCtx) const;

    const TString& GetStartFromTrackId() const {
        return Proto_.GetPlaybackContext().GetStartFromTrackId();
    }

    const TPlaybackContext& GetPlaybackContext() const {
        return Proto_.GetPlaybackContext();
    }

    const TBiometryOptions& GetBiometryOptions() const {
        return Proto_.GetPlaybackContext().GetBiometryOptions();
    }

    const TString& GetBiometryUserId() const {
        return GetBiometryOptions().GetUserId();
    }

    const TString& GetBiometryUserIdOrFallback(const TString& userId) const;

    const TString& GetGuestOAuthTokenEncrypted() const {
        return GetBiometryOptions().GetGuestOAuthTokenEncrypted();
    }

    TBiometryOptions::EPlaybackMode GetBiometryPlaybackMode() const {
        return GetBiometryOptions().GetPlaybackMode();
    }

    bool IsGuestBiometryPlaybackMode() const {
        return GetBiometryPlaybackMode() == TBiometryOptions_EPlaybackMode_GuestMode;
    }

    bool IsIncognitoBiometryPlaybackMode() const {
        return GetBiometryPlaybackMode() == TBiometryOptions_EPlaybackMode_IncognitoMode;
    }

    void SetUpPlaybackModeUsingClientBiometry(const NHollywood::TScenarioApplyRequestWrapper& request,
                                              const TMusicArguments& applyArgs,
                                              bool isClientBiometryModeApplyRequest);

    void SetUpPlaybackModeUsingServerBiometry(const NHollywood::TScenarioApplyRequestWrapper& request,
                                              const TMusicArguments& applyArgs);

    ::google::protobuf::Struct MakeRecoveryActionCallbackPayload() const;

    void UpdateContentId(const TString& id) {
        Proto_.MutablePlaybackContext()->MutableContentId()->SetId(id);
    }

    bool HasTrackInHistory(const TString& trackId);

    bool HasTrackInHistory(const TString& trackId, const TContentId_EContentType trackType);

    void MoveTrackFromQueueToHistory();

    ui64 GetContentHash(const ui64 musicSubscriptionRegionId) const;

    TString ToStringDebug() const;

    std::pair<int, int> GetNeighboringTracksBound(const TScenarioApplyRequestWrapper& request) const;

private:
    TRTLogger& Logger_;
    TMusicQueue& Proto_;
    i32 FilteredOut_ = 0;
    i32 AddedItemsCount_ = 0;
    bool HaveExplicitContent_ = false;
    bool HaveNonChildSafe_ = false;
    bool HavePodcastContent_ = false;
};

} // namespace NAlice::NHollywood::NMusic
