#include "music_queue.h"

#include <alice/hollywood/library/scenarios/music/biometry/process_biometry.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/content_id/content_id.h>

#include <alice/hollywood/library/scenarios/music/proto/callback_payload.pb.h>

#include <alice/hollywood/library/personal_data/personal_data.h>

#include <alice/megamind/protos/scenarios/request.pb.h>
#include <alice/protos/data/scenario/music/content_info.pb.h>

#include <alice/library/experiments/flags.h>
#include <alice/library/proto/proto.h>
#include <alice/library/proto/protobuf.h>

#include <util/digest/multi.h>
#include <util/generic/adaptor.h>
#include <util/generic/hash.h>
#include <util/generic/vector.h>
#include <util/generic/yexception.h>
#include <util/string/builder.h>

#include <utility>

namespace NAlice::NHollywood::NMusic {

namespace {

constexpr TStringBuf RADIO_PUMPKIN_FROM_MUSIC_ENTITY = "pumpkin";

TString MakeBeforeTrackSlotKey(const TStringBuf trackId) {
    TString rv{trackId};
    rv += "_before";
    return rv;
}

TPlaybackContext CreatePlaybackContext(const TContentId& contentId, IRng& rng,
                                       const TMusicArguments::TPlaybackOptions& playbackOptions,
                                       bool enableShots, const NData::NMusic::TContentInfo& contentInfo) {
    TPlaybackContext pb;
    *pb.MutableContentId() = contentId;
    pb.SetPlaySingleTrack(playbackOptions.GetPlaySingleTrack());
    pb.SetDisableNlg(playbackOptions.GetDisableNlg());
    pb.SetEnableShots(enableShots);
    pb.SetFrom(playbackOptions.GetFrom());
    if (contentId.GetType() == TContentId_EContentType_Generative) {
        // Disabling autoflow for generative stream
        pb.SetDisableAutoflow(true);
    } else if (contentId.GetType() == TContentId_EContentType_Radio) {
        // There is no shuffle playback for radio.
        // TODO(vitvlkv): Support repeat single track maybe
        pb.SetStartFromTrackId(playbackOptions.GetStartFromTrackId());
        pb.SetUseIchwill(playbackOptions.GetUseIchwill());
    } else {
        pb.SetShuffle(playbackOptions.GetShuffle());
        if (pb.GetShuffle()) {
            pb.SetShuffleSeed(rng.RandomInteger(Max<i32>()));
        }
        pb.SetRepeatType(playbackOptions.GetRepeatType());
        pb.SetDisableAutoflow(playbackOptions.GetDisableAutoflow());
        pb.SetTrackOffsetIndex(playbackOptions.GetTrackOffsetIndex());
        pb.SetStartFromTrackId(playbackOptions.GetStartFromTrackId());
        pb.SetDisableHistory(playbackOptions.GetDisableHistory());
        *pb.MutableContentInfo() = contentInfo;
    }
    return pb;
}

bool IsItemAvailable(const TQueueItem& item, const bool hasMusicSubscription) {
    if (item.HasTrackInfo()) {
        if (!item.GetTrackInfo().GetAvailable()) {
            if (item.GetTrackInfo().GetAvailableForPremiumUsers() && hasMusicSubscription) {
                // an item may be unavailable to listen for simple users
                // but available for users with music subscription
                return true;
            } else {
                // an item may be fully unavailable to listen due to legal reason
                // but can be given by music backend (for example, if track belongs to personal playlist)
                return false;
            }
        }
        // the track is fully available
        return true;
    }
    // other types of tracks (generative tracks and so on...) don't have TrackInfo, and are available always
    return true;
}

TStringBuf GetFromQueryType(const TContentId& contentId) {
    if (contentId.GetType() == TContentId_EContentType_Radio ||
        contentId.GetType() == TContentId_EContentType_Generative)
    {
        return "discovery";
    }

    return "on_demand";
}

TStringBuf GetFromQuerySubtype(const TContentId& contentId) {
    switch (contentId.GetType()) {
        case TContentId_EContentType_Track:
        case TContentId_EContentType_Album:
        case TContentId_EContentType_Artist:
            return "catalogue_";
        case TContentId_EContentType_Playlist:
            return "user_playlist";
        case TContentId_EContentType_Radio:
            return "radio";
        case TContentId_EContentType_Generative:
            return "generative";
        case TContentId_EContentType_FmRadio:
            return "fm_radio";
        case TContentId_EContentType_TContentId_EContentType_INT_MIN_SENTINEL_DO_NOT_USE_:
        case TContentId_EContentType_TContentId_EContentType_INT_MAX_SENTINEL_DO_NOT_USE_:
            Y_UNREACHABLE();
    }
}

TString GetFromMusicEntity(const TContentId& contentId, const bool isRadioPumpkin) {
    const auto contentType = contentId.GetType();
    switch (contentId.GetType()) {
        case TContentId_EContentType_Track:
        case TContentId_EContentType_Album:
        case TContentId_EContentType_Artist:
        case TContentId_EContentType_Playlist:
            return ContentTypeToText(contentType);
        case TContentId_EContentType_Radio: {
            const auto& contentIdId = contentId.GetId();
            if (const auto colonPos = contentIdId.find(':'); colonPos != TString::npos) {
                if (!isRadioPumpkin) {
                    return contentIdId.substr(0, colonPos);
                } else {
                    return TString(RADIO_PUMPKIN_FROM_MUSIC_ENTITY);
                }
            } else {
                ythrow yexception() << "There is no ':' in radio content id " << contentIdId;
            }
        }
        case TContentId_EContentType_Generative:
            return "genre";
        case TContentId_EContentType_FmRadio:
            return "fm_radio";
        case TContentId_EContentType_TContentId_EContentType_INT_MIN_SENTINEL_DO_NOT_USE_:
        case TContentId_EContentType_TContentId_EContentType_INT_MAX_SENTINEL_DO_NOT_USE_:
            Y_UNREACHABLE();
    }
}

} // namespace

void TMusicQueueWrapper::SetIsEndOfContent(bool isEndOfContent) {
    Proto_.SetIsEndOfContent(isEndOfContent);
}

bool TMusicQueueWrapper::HasCurrentItemOrHistory() const {
    return Proto_.HistorySize() > Proto_.GetHistoryBound();
}

bool TMusicQueueWrapper::HasCurrentItem() const {
    return HasCurrentItemOrHistory() && !Proto_.GetIsEndOfContent();
}

const TQueueItem& TMusicQueueWrapper::CurrentItem() const {
    Y_ENSURE(HasCurrentItemOrHistory());
    return *Proto_.GetHistory().rbegin();
}

bool TMusicQueueWrapper::HasPreviousItem() const {
    return Proto_.HistorySize() > Proto_.GetHistoryBound() + 1;
}

const TQueueItem& TMusicQueueWrapper::PreviousItem() const {
    Y_ENSURE(HasPreviousItem());
    auto it = Proto_.GetHistory().rbegin();
    it++;
    Y_ENSURE(it != Proto_.GetHistory().rend());
    return *it;
}

bool TMusicQueueWrapper::HasNextItem() const {
    return !Proto_.GetQueue().empty();
}

const TQueueItem& TMusicQueueWrapper::NextItem() const {
    Y_ENSURE(HasNextItem());
    return *Proto_.GetQueue().begin();
}

TQueueItem& TMusicQueueWrapper::MutableCurrentItem() {
    Y_ENSURE(HasCurrentItemOrHistory());
    return *Proto_.MutableHistory(Proto_.HistorySize() - 1);
}

bool TMusicQueueWrapper::IsCurrentTrackLast() const {
    if (IsPlayingSingleTrack()) {
        return true;
    }
    if (IsPaged()) {
        auto& paged = Proto_.GetCurrentContentLoadingState().GetPaged();
        return (paged.GetPageIdx() + 1) * Config().PageSize >= paged.GetTotalTracks() && Proto_.GetQueue().empty();
    } else if (IsRadio()) {
        return false;
    } else if (IsGenerative()) {
        return true;
    } else if (IsFmRadio()) {
        return false;
    } else {
        ythrow yexception() << "Could not find supported CurrentState, HasCurrentState="
                            << Proto_.HasCurrentContentLoadingState();
    }
}

ETrackChangeResult TMusicQueueWrapper::ChangeToNextTrack() {
    const auto repeatType = Proto_.GetPlaybackContext().GetRepeatType();
    if (IsPlayingSingleTrack()) {
        return repeatType != RepeatNone ? ETrackChangeResult::SameTrack : ETrackChangeResult::EndOfContent;
    }
    if (repeatType == RepeatTrack) {
        return ETrackChangeResult::SameTrack;
    }
    if (IsPaged()) {
        auto& paged = Proto_.GetCurrentContentLoadingState().GetPaged();
        if (Proto_.GetQueue().empty()) {
            if ((paged.GetPageIdx() + 1) * Config().PageSize >= paged.GetTotalTracks()) {
                if (repeatType == RepeatAll) {
                    auto& nextPaged = *Proto_.MutableNextContentLoadingState()->MutablePaged();
                    nextPaged.SetPageIdx(0);
                    nextPaged.SetTotalTracks(paged.GetTotalTracks()); // TODO(vitvlkv): Do we need this?..
                    return ETrackChangeResult::NeedStateUpdate;
                } else {
                    return ETrackChangeResult::EndOfContent;
                }
            }
            auto& nextPaged = *Proto_.MutableNextContentLoadingState()->MutablePaged();
            nextPaged.SetPageIdx(paged.GetPageIdx() + 1);
            nextPaged.SetTotalTracks(paged.GetTotalTracks()); // TODO(vitvlkv): Do we need this?..
            return ETrackChangeResult::NeedStateUpdate;
        }
    } else if (IsRadio()) {
        if (Proto_.GetQueue().empty()) {
            Proto_.MutableNextContentLoadingState()->MutableRadio(); // Create empty (for now) next loading state
            return ETrackChangeResult::NeedStateUpdate;
        }
    } else if (IsFmRadio()) {
        if (Proto_.GetQueue().empty()) {
            Proto_.MutableNextContentLoadingState()->MutableFmRadio();
            return ETrackChangeResult::NeedStateUpdate;
        }
    } else {
        ythrow yexception() << "Could not find supported CurrentState, HasCurrentState="
                            << Proto_.HasCurrentContentLoadingState();
    }

    if (HasNextItem()) {
        if (NextItem().GetType() == ContentTypeToText(TContentId_EContentType_Generative)) {
            // For NextTrack command: if content type changed from something to generative
            Proto_.MutableCurrentContentLoadingState()->MutableGenerative();
            TContentId generative;
            generative.SetId(NextItem().GetGenerativeInfo().GetGenerativeStationId());
            generative.SetType(TContentId_EContentType_Generative);
            auto& pb = *Proto_.MutablePlaybackContext();
            *pb.MutableContentId() = std::move(generative);
        } else if (NextItem().GetType() == ContentTypeToText(TContentId_EContentType_FmRadio)) {
            // For NextTrack command: if content type changed from something to fmRadio
            Proto_.MutableCurrentContentLoadingState()->MutableFmRadio();
            TContentId fmRadio;
            fmRadio.SetId(NextItem().GetFmRadioInfo().GetFmRadioId());
            fmRadio.SetType(TContentId_EContentType_FmRadio);
            auto& pb = *Proto_.MutablePlaybackContext();
            *pb.MutableContentId() = std::move(fmRadio);
        } else if (HasCurrentItem() && (CurrentItem().GetType() == ContentTypeToText(TContentId_EContentType_Generative) ||
                                        CurrentItem().GetType() == ContentTypeToText(TContentId_EContentType_FmRadio)))
        {
            // For NextTrack command: if content type changed from something to Paged
            TContentId track;
            track.SetId(NextItem().GetTrackId());
            track.SetType(TContentId_EContentType_Track);
            auto& pb = *Proto_.MutablePlaybackContext();
            *pb.MutableContentId() = std::move(track);
            Proto_.MutableCurrentContentLoadingState()->MutablePaged();
        }
    }

    MoveTrackFromQueueToHistory();
    return ETrackChangeResult::TrackChanged;
}

ETrackChangeResult TMusicQueueWrapper::ChangeToPrevTrack() {
    const auto repeatType = Proto_.GetPlaybackContext().GetRepeatType();
    if (IsPlayingSingleTrack()) {
        return repeatType != RepeatNone ? ETrackChangeResult::SameTrack : ETrackChangeResult::EndOfContent;
    }
    if (repeatType == RepeatTrack) {
        return ETrackChangeResult::SameTrack;
    }
    if (!HasPreviousItem()) {
        return ETrackChangeResult::EndOfContent;
    }

    SetShotPlayed(MutableCurrentItem(), /* played = */ false, /* onlyFirstAvailable = */ false);

    auto queueTail = Proto_.GetQueue();
    auto& queue = *Proto_.MutableQueue();
    queue.Clear();

    if (PreviousItem().GetType() == ContentTypeToText(TContentId_EContentType_Generative)) {
        // If content type changed from Something to Generative
        TContentId generative;
        generative.SetId(PreviousItem().GetGenerativeInfo().GetGenerativeStationId());
        generative.SetType(TContentId_EContentType_Generative);
        SetContentId(generative);
        Proto_.MutableHistory()->erase(Proto_.MutableHistory()->end() - 1);
        Proto_.MutableHistory()->erase(Proto_.MutableHistory()->end() - 1);
    } else if (PreviousItem().GetType() == ContentTypeToText(TContentId_EContentType_FmRadio)) {
        // If content type changed from Something to FmRadio
        TContentId fmRadio;
        fmRadio.SetId(PreviousItem().GetFmRadioInfo().GetFmRadioId());
        fmRadio.SetType(TContentId_EContentType_FmRadio);
        SetContentId(fmRadio);
        *queue.Add() = Proto_.GetHistory(Proto_.HistorySize() - 1);
        for (auto& item: queueTail) {
            *queue.Add() = std::move(item);
        }

        Proto_.MutableHistory()->erase(Proto_.MutableHistory()->end() - 1);
        ChangeState(/* moveFromQueueToHistory = */ false);
    } else {
        if (CurrentItem().GetType() == ContentTypeToText(TContentId_EContentType_Generative) ||
            CurrentItem().GetType() == ContentTypeToText(TContentId_EContentType_FmRadio)) {
            // If content type changed from Generative or FmRadio to Something (Paged as default?)
            TContentId track;
            track.SetId(PreviousItem().GetTrackId());
            track.SetType(TContentId_EContentType_Track);
            auto& pb = *Proto_.MutablePlaybackContext();
            *pb.MutableContentId() = std::move(track);
            Proto_.MutableCurrentContentLoadingState()->MutablePaged();
        }

        *queue.Add() = Proto_.GetHistory(Proto_.HistorySize() - 1);
        for (auto& item: queueTail) {
            *queue.Add() = std::move(item);
        }

        Proto_.MutableHistory()->erase(Proto_.MutableHistory()->end() - 1);
    }

    return ETrackChangeResult::TrackChanged;
}

bool TMusicQueueWrapper::NeedToChangeState() const {
    return Proto_.HasNextContentLoadingState();
}

bool TMusicQueueWrapper::TryAddItem(TQueueItem&& item, bool hasMusicSubscription) {
    const auto isRadio = ContentId().GetType() == TContentId_EContentType_Radio;
    if (!IsItemAvailable(item, hasMusicSubscription) && !isRadio) {
        return false;
    }

    if (Proto_.GetFiltrationMode() == NScenarios::TUserPreferences::FamilySearch &&
        item.GetContentWarning() == EContentWarning::Explicit && !isRadio) {
        ++FilteredOut_;
        return false;
    }

    if (item.GetContentWarning() != EContentWarning::ChildSafe) {
        HaveNonChildSafe_ = true;
    }

    if (item.HasTrackInfo() && item.GetTrackInfo().GetAlbumType() == "podcast") {
        HavePodcastContent_ = true;
    }

    if (Proto_.GetFiltrationMode() == NScenarios::TUserPreferences::Safe && HaveNonChildSafe_ && !isRadio) {
        return false;
    }

    if (item.GetContentWarning() == EContentWarning::Explicit) {
        HaveExplicitContent_ = true;
    }

    *item.MutableOriginContentId() = ContentId();
    *Proto_.MutableQueue()->Add() = std::move(item);
    ++AddedItemsCount_;
    return true;
}

void TMusicQueueWrapper::ChangeState(bool moveFromQueueToHistory) {
    *Proto_.MutableCurrentContentLoadingState() = Proto_.GetNextContentLoadingState();
    Proto_.ClearNextContentLoadingState();

    // One of tracks in safe mode was non child safe. Clear whole queue.
    const auto isRadio = ContentId().GetType() == TContentId_EContentType_Radio;
    if (Proto_.GetFiltrationMode() == NScenarios::TUserPreferences::Safe && HaveNonChildSafe_ && !isRadio) {
        Proto_.MutableQueue()->Clear();
    }

    if (moveFromQueueToHistory) {
        MoveTrackFromQueueToHistory();
    }
}

TMaybe<bool> TMusicQueueWrapper::GetMetadataShuffled() const {
    if (!IsGenerative() && !IsRadio() &&
        GetPlaybackContext().GetContentId().GetType() != TContentId_EContentType_Track)
    {
        return GetShuffle();
    }

    return Nothing();
}

TMaybe<ERepeatType> TMusicQueueWrapper::GetMetadataRepeatType() const {
    if (!IsGenerative()) {
        return GetRepeatType();
    }

    return Nothing();
}

bool TMusicQueueWrapper::HasTrackInHistory(const TString& trackId) {
    return AnyOf(Proto_.GetHistory(), [&trackId](const TQueueItem& item) {
        return item.GetTrackId() == trackId;
    });
}

bool TMusicQueueWrapper::HasTrackInHistory(const TString& trackId, const TContentId_EContentType trackType) {
    return AnyOf(Proto_.GetHistory(), [&trackId, trackType](const TQueueItem& item) {
        return item.GetTrackId() == trackId && item.GetOriginContentId().GetType() == trackType;
    });
}

void TMusicQueueWrapper::MoveTrackFromQueueToHistory() {
    if (Proto_.QueueSize() == 0) {
        return;
    }
    if (IsHistoryDisabled()) {
        Proto_.MutableHistory()->Clear();
        Proto_.SetHistoryBound(Proto_.HistorySize());
    }
    *Proto_.AddHistory() = Proto_.MutableQueue()->Get(0);

    if (static_cast<i32>(Proto_.HistorySize()) > Config().HistorySize + 1) { // one item is current
        Proto_.MutableHistory()->erase(Proto_.MutableHistory()->begin());
        const size_t historyBound = Proto_.GetHistoryBound();
        if (historyBound > 0) {
            Proto_.SetHistoryBound(historyBound - 1);
        }
    }
    Proto_.MutableQueue()->erase(Proto_.MutableQueue()->begin());
}

void TMusicQueueWrapper::InitPlaybackFromContext(const TPlaybackContext& playbackContext) {
    Proto_.MutableQueue()->Clear();
    Proto_.MutableExtraPlayableMap()->clear();
    *Proto_.MutablePlaybackContext() = playbackContext;
    Proto_.SetHistoryBound(playbackContext.GetDisableHistory() ? Proto_.HistorySize() : 0);
    if (playbackContext.GetContentId().GetType() == TContentId_EContentType_Radio) {
        Proto_.MutableNextContentLoadingState()->MutableRadio();
    } else if (playbackContext.GetContentId().GetType() == TContentId_EContentType_Generative) {
        Proto_.MutableNextContentLoadingState()->MutableGenerative();
    } else if (playbackContext.GetContentId().GetType() == TContentId_EContentType_FmRadio) {
        Proto_.MutableNextContentLoadingState()->MutableFmRadio();
    } else {
        Proto_.MutableNextContentLoadingState()->MutablePaged();
    }
}

void TMusicQueueWrapper::InitPlayback(const TContentId& contentId, IRng& rng,
                                      const TMusicArguments::TPlaybackOptions& playbackOptions,
                                      bool enableShots,
                                      const NData::NMusic::TContentInfo& contentInfo) {
    InitPlaybackFromContext(CreatePlaybackContext(contentId, rng, playbackOptions, enableShots, contentInfo));
}

void TMusicQueueWrapper::SetContentId(const TContentId& contentId) {
    TPlaybackContext playbackContext = Proto_.GetPlaybackContext();
    *playbackContext.MutableContentId() = contentId;
    InitPlaybackFromContext(playbackContext);
}

void TMusicQueueWrapper::ShufflePlayback(IRng& rng) {
    Proto_.MutableQueue()->Clear();
    Proto_.MutableNextContentLoadingState()->MutablePaged();

    const auto& pbCurrent = Proto_.GetPlaybackContext();
    const auto repeat = pbCurrent.GetRepeatType();
    const auto contentId = pbCurrent.GetContentId();

    auto& pb = *Proto_.MutablePlaybackContext();
    pb.Clear();
    pb.SetShuffle(true);
    pb.SetShuffleSeed(rng.RandomInteger(Max<i32>()));
    pb.SetRepeatType(repeat);
    *pb.MutableContentId() = std::move(contentId);
}

void TMusicQueueWrapper::RepeatPlayback(ERepeatType repeatType) {
    Proto_.MutablePlaybackContext()->SetRepeatType(repeatType);
}

ERepeatType TMusicQueueWrapper::GetRepeatType() const {
    if (!Proto_.HasPlaybackContext()) {
        return RepeatNone;
    }
    return Proto_.GetPlaybackContext().GetRepeatType();
}

bool TMusicQueueWrapper::GetShuffle() const {
    return Proto_.GetPlaybackContext().GetShuffle();
}

i32 TMusicQueueWrapper::GetShuffleSeed() const {
    return Proto_.GetPlaybackContext().GetShuffleSeed();
}

void TMusicQueueWrapper::SetEnableShots(bool enableShots) {
    Proto_.MutablePlaybackContext()->SetEnableShots(enableShots);
}

bool TMusicQueueWrapper::HasShotsEnabled() const {
    return Proto_.GetPlaybackContext().GetEnableShots();
}

TString TMusicQueueWrapper::ArtistName(const TQueueItem& item) {
    if (item.GetTrackInfo().GetArtists().empty()) {
        return {};
    }
    return item.GetTrackInfo().GetArtists()[0].GetName();
}

void TMusicQueueWrapper::SetConfig(const TMusicConfig& config) {
    Proto_.MutableConfig()->SetPageSize(config.PageSize);
    Proto_.MutableConfig()->SetHistorySize(config.HistorySize);
    Proto_.MutableConfig()->SetExplicitFilteredOutWarningRate(config.ExplicitFilteredOutWarningRate);
    Proto_.MutableConfig()->SetFindTrackIdxPageSize(config.FindTrackIdxPageSize);
}

const TContentId& TMusicQueueWrapper::ContentId() const {
    return Proto_.GetPlaybackContext().GetContentId();
}

void TMusicQueueWrapper::SetContentInfo(const NData::NMusic::TContentInfo& contentInfo) {
    *Proto_.MutablePlaybackContext()->MutableContentInfo() = contentInfo;
}

const NData::NMusic::TContentInfo& TMusicQueueWrapper::ContentInfo() const {
    return Proto_.GetPlaybackContext().GetContentInfo();
}

TVector<TStringBuf> TMusicQueueWrapper::ContentIdsValues() const {
    TVector<TStringBuf> values;
    for (const auto& id : ContentId().GetIds()) {
        values.push_back(id);
    }
    return values;
}

void TMusicQueueWrapper::SetFiltrationMode(NAlice::NScenarios::TUserPreferences_EFiltrationMode filtrationMode) {
    Proto_.SetFiltrationMode(filtrationMode);
}

NScenarios::TUserPreferences::EFiltrationMode TMusicQueueWrapper::FiltrationMode() const {
    return Proto_.GetFiltrationMode();
}

TContentErrorsAndAttentions TMusicQueueWrapper::CalcContentErrorsAndAttentions() const {
    TContentErrorsAndAttentions result;

    if (FiltrationMode() == NScenarios::TUserPreferences::Safe && HaveNonChildSafeContent()) {
        result.Error = EContentError::RestrictedByChild;
        if (HavePodcastContent()) {
            result.Attention = EContentAttention::ForbiddenPodcast;
        }
        return result;
    }

    if (GetAddedItemsCount() == 0) {
        if (GetFilteredOut() > 0) {
            result.Error = EContentError::Forbidden;
        } else {
            result.Error = EContentError::NotFound;
        }
        return result;
    }

    auto total = GetAddedItemsCount() + GetFilteredOut();
    auto explicitRate = total != 0 ? static_cast<double>(GetFilteredOut()) / total : 0.0;
    if (FiltrationMode() == NScenarios::TUserPreferences::FamilySearch &&
        explicitRate > Config().ExplicitFilteredOutWarningRate)
    {
        result.Attention = EContentAttention::ExplicitContentFiltered;
    } else if (FiltrationMode() == NScenarios::TUserPreferences::Moderate && HaveExplicitContent()) {
        result.Attention = EContentAttention::MayContainExplicitContent;
    } else if (HaveNonChildSafeContent()) {
        result.Attention = EContentAttention::ContainsAdultContent;
    }
    return result;
}

void TMusicQueueWrapper::CalcContentErrorsAndAttentions(TMusicContext& mCtx) const {
    const auto result = CalcContentErrorsAndAttentions();

    switch (result.Error) {
    case EContentError::NoError:
        break;
    case EContentError::Forbidden:
        mCtx.MutableContentStatus()->SetErrorVer2(ErrorForbiddenVer2);
        LOG_INFO(Logger_) << "ErrorForbiddenVer2 was set";
        break;
    case EContentError::RestrictedByChild:
        mCtx.MutableContentStatus()->SetErrorVer2(ErrorRestrictedByChildVer2);
        LOG_INFO(Logger_) << "ErrorRestrictedByChildVer2 was set";
        break;
    case EContentError::NotFound:
        mCtx.MutableContentStatus()->SetErrorVer2(ErrorNotFoundVer2);
        LOG_INFO(Logger_) << "ErrorNotFoundVer2 was set";
        break;
    }
    
    switch (result.Attention) {
    case EContentAttention::NoAttention:
        break;
    case EContentAttention::ContainsAdultContent:
        mCtx.MutableContentStatus()->SetAttentionVer2(AttentionContainsAdultContentVer2);
        break;
    case EContentAttention::ExplicitContentFiltered:
        mCtx.MutableContentStatus()->SetAttentionVer2(AttentionExplicitContentFilteredVer2);
        break;
    case EContentAttention::MayContainExplicitContent:
        mCtx.MutableContentStatus()->SetAttentionVer2(AttentionMayContainExplicitContentVer2);
        break;
    case EContentAttention::ForbiddenPodcast:
        mCtx.MutableContentStatus()->SetAttentionVer2(AttentionForbiddenPodcast);
        break;
    }
}

TString TMusicQueueWrapper::MakeFrom() const {
    // TODO(vitvlkv, zhigan): Implement correct smart playlist and origin for from
    // https://st.yandex-team.ru/HOLLYWOOD-311#605b387b07effe485d5313c3
    if (!GetPlaybackContext().GetFrom().empty()) {
        return GetPlaybackContext().GetFrom();
    }

    const auto& contentId = GetPlaybackContext().GetContentId();

    return TString::Join("alice-", GetFromQueryType(contentId),
                         "-", GetFromQuerySubtype(contentId),
                         "-", GetFromMusicEntity(contentId, IsRadioPumpkin()));
}

void TMusicQueueWrapper::AddShotBeforeTrack(const TStringBuf trackId, TExtraPlayable_TShot&& shot) {
    Y_ENSURE(HasShotsEnabled());
    auto key = MakeBeforeTrackSlotKey(trackId);
    auto& xmap = *Proto_.MutableExtraPlayableMap();
    auto& xp = xmap.insert(std::decay_t<decltype(xmap)>::value_type(key, TExtraPlayable())).first->second;
    auto& playable = *xp.MutableQueue()->Add();
    *playable.MutableShot() = std::move(shot);
}

void TMusicQueueWrapper::MarkBeforeTrackSlot(const TStringBuf trackId) {
    Y_ENSURE(HasShotsEnabled());
    auto key = MakeBeforeTrackSlotKey(trackId);
    auto& xmap = *Proto_.MutableExtraPlayableMap();
    xmap.insert(std::decay_t<decltype(xmap)>::value_type(key, TExtraPlayable()));
}

void TMusicQueueWrapper::SetShotPlayed(const TQueueItem& item, bool played, bool onlyFirstAvailable) {
    auto& xmap = *Proto_.MutableExtraPlayableMap();
    auto it = xmap.find(MakeBeforeTrackSlotKey(item.GetTrackId()));
    if (it == xmap.end()) {
        return;
    }
    auto& queue = *it->second.MutableQueue();
    // Set played for all shots before the track
    for (auto& shotItem : queue) {
        if (shotItem.HasShot() && shotItem.GetShot().GetPlayed() != played) {
            shotItem.MutableShot()->SetPlayed(played);
            if (onlyFirstAvailable) {
                return;
            }
        }
    }
}

TMaybe<TExtraPlayable_TShot> TMusicQueueWrapper::GetShotBeforeCurrentItem() const {
    if (!HasCurrentItem()) {
        return Nothing();
    }
    auto& xmap = Proto_.GetExtraPlayableMap();
    auto it = xmap.find(MakeBeforeTrackSlotKey(CurrentItem().GetTrackId()));
    if (it == xmap.end()) {
        return Nothing();
    }
    auto& queue = it->second.GetQueue();
    for (auto& item : queue) {
        if (item.HasShot() && !item.GetShot().GetPlayed()) {
            return item.GetShot();
        }
    }
    return Nothing();
}

TMaybe<TExtraPlayable_TShot> TMusicQueueWrapper::GetLastShotOfCurrentItem() const {
    if (!HasCurrentItem()) {
        return Nothing();
    }
    const auto& xmap = Proto_.GetExtraPlayableMap();
    auto it = xmap.find(MakeBeforeTrackSlotKey(CurrentItem().GetTrackId()));
    if (it == xmap.end()) {
        return Nothing();
    }
    const auto& queue = it->second.GetQueue();
    for (const auto& item : Reversed(queue)) {
        if (item.HasShot() && item.GetShot().GetPlayed()) {
            return item.GetShot();
        }
    }
    return Nothing();
}

bool TMusicQueueWrapper::HasExtraBeforeCurrentItem() const {
    if (!HasCurrentItem()) {
        return false;
    }
    const auto& xmap = Proto_.GetExtraPlayableMap();
    return xmap.find(MakeBeforeTrackSlotKey(CurrentItem().GetTrackId())) != xmap.end();
}

ui32 TMusicQueueWrapper::GetPagedFirstTrackOffsetIndex(const TMusicContext& mCtx) const {
    if (const auto pageSize = mCtx.GetFirstRequestPageSize()) {
        // we have an unusual page, calculate the offset by dropping the remainder of division
        return (GetTrackOffsetIndex() / pageSize) * pageSize;
    } else {
        // we have a usual page, return the first index
        return Config().PageSize * NextPageIndex();
    }
}

const TString& TMusicQueueWrapper::GetBiometryUserIdOrFallback(const TString& userId) const {
    if (GetBiometryPlaybackMode() == TBiometryOptions_EPlaybackMode_IncognitoMode) {
        return userId;
    }
    return GetBiometryUserId();
}

void TMusicQueueWrapper::SetUpPlaybackModeUsingClientBiometry(
    const NHollywood::TScenarioApplyRequestWrapper& request,
    const TMusicArguments& applyArgs,
    bool isClientBiometryModeApplyRequest
)
{
    TBiometryOptions biometryOpts;
    const auto& ownerUid = applyArgs.GetAccountStatus().GetUid();
    auto isOwnerEnrolled = applyArgs.GetIsOwnerEnrolled();

    biometryOpts.SetIsOwnerEnrolled(isOwnerEnrolled);
    if (isClientBiometryModeApplyRequest) {
        biometryOpts.SetUserId(applyArgs.GetGuestCredentials().GetUid());
        biometryOpts.SetGuestOAuthTokenEncrypted(applyArgs.GetGuestCredentials().GetOAuthTokenEncrypted());
        if (biometryOpts.GetUserId() == ownerUid) {
            biometryOpts.SetPlaybackMode(TBiometryOptions_EPlaybackMode_OwnerMode);
        } else {
            biometryOpts.SetPlaybackMode(TBiometryOptions_EPlaybackMode_GuestMode);
        }
    } else {
        auto kolonkishUid = GetKolonkishUidFromDataSync(request);
        if (!isOwnerEnrolled || !kolonkishUid) {
            if (isOwnerEnrolled) {
                LOG_ERROR(Logger_) << "Owner is enrolled, but it is failed to get kolonkish uid from DataSync";
            }
            biometryOpts.SetUserId(ownerUid);
            biometryOpts.SetPlaybackMode(TBiometryOptions_EPlaybackMode_OwnerMode);
        } else {
            biometryOpts.SetUserId(*kolonkishUid);
            biometryOpts.SetPlaybackMode(TBiometryOptions_EPlaybackMode_IncognitoMode);
        }
    }

    *Proto_.MutablePlaybackContext()->MutableBiometryOptions() = std::move(biometryOpts);
}

void TMusicQueueWrapper::SetUpPlaybackModeUsingServerBiometry(
    const NHollywood::TScenarioApplyRequestWrapper& request,
    const TMusicArguments& applyArgs
)
{
    TBiometryOptions biometryOpts;

    const auto biometryData = ProcessBiometryOrFallback(Logger_, request, TStringBuf{applyArgs.GetAccountStatus().GetUid()});
    Y_ENSURE(!biometryData.IsGuestUser, "BioCapability is supported, use SetUpPlaybackModeUsingClientBiometry function instead");
    Y_ENSURE(!biometryData.UserId.Empty());

    biometryOpts.SetUserId(biometryData.UserId);
    if (biometryData.IsIncognitoUser) {
        biometryOpts.SetPlaybackMode(TBiometryOptions_EPlaybackMode_IncognitoMode);
    } else {
        biometryOpts.SetPlaybackMode(TBiometryOptions_EPlaybackMode_OwnerMode);
    }

    *Proto_.MutablePlaybackContext()->MutableBiometryOptions() = std::move(biometryOpts);
}

::google::protobuf::Struct TMusicQueueWrapper::MakeRecoveryActionCallbackPayload() const {
    TRecoveryCallbackPayload payload;
    *payload.MutablePlaybackContext() = GetPlaybackContext();
    if (IsPaged()) {
        payload.MutablePaged(); // It is empty, but we want it to be defined
    } else if (IsRadio()) {
        payload.MutableRadio()->SetBatchId(GetRadioBatchId());
        payload.MutableRadio()->SetSessionId(GetRadioSessionId());
    } else if (IsGenerative()) {
        // TODO(amullanurov): add recovery callback for generative
    } else if (IsFmRadio()) {
        // TODO(amullanurov): add recovery callback for fmRadio
    } else {
        ythrow yexception() << "MusicQueue is expected to be either Paged or Radio";
    }
    return MessageToStruct(payload);
}

ui64 TMusicQueueWrapper::GetContentHash(const ui64 musicSubscriptionRegionId) const {
    LOG_INFO(Logger_) << "Computing hash with "
                      << "type=" << ContentTypeToText(ContentId().GetType())
                      << ", id=" << ContentId().GetId()
                      << ", musicSubscriptionRegionId=" << musicSubscriptionRegionId;

    return MultiHash(
        ContentTypeToText(ContentId().GetType()),
        ContentId().GetId(),
        musicSubscriptionRegionId
    );
}

TString TMusicQueueWrapper::ToStringDebug() const {
    TStringBuilder result;

    const auto& contentId = Proto_.GetPlaybackContext().GetContentId();
    result << "ContentId=" << SerializeProtoText(contentId) << ", "
           << "Queue.size()=" << Proto_.GetQueue().size() << ", "
           << "History.size()=" << Proto_.GetHistory().size() << ", ";
    if (!Proto_.GetHistory().empty()) {
        result << "History back=" << Proto_.GetHistory().rbegin()->GetTrackId() << ", ";
    }
    if (!Proto_.GetQueue().empty()) {
        result << "Queue front=" << Proto_.GetQueue().begin()->GetTrackId() << ", ";
    }

    return result;
}

std::pair<int, int> TMusicQueueWrapper::GetNeighboringTracksBound(const TScenarioApplyRequestWrapper& request) const {
    // overall tracks count is 2 * NEIGHBORING_TRACKS_COUNT + 1
    constexpr int DEFAULT_NEIGHBORING_TRACKS_COUNT = 50;

    const int currTrackPosition = CurrentItem().GetTrackInfo().GetPosition();
    const int neighboringTracksCount = request.LoadValueFromExpPrefix(
        NExperiments::EXP_HW_MUSIC_SHOW_VIEW_NEIGHBORING_TRACKS_COUNT,
        /* default = */ DEFAULT_NEIGHBORING_TRACKS_COUNT
    );
    const int leftTrackPosition = Max(0, currTrackPosition - neighboringTracksCount);
    const int rightTrackPosition = currTrackPosition + neighboringTracksCount;
    return {leftTrackPosition, rightTrackPosition};
}

} // namespace NAlice::NHollywood::NMusic
