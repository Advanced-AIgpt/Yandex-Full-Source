#pragma once

#include "requests_helper_base.h"

#include <alice/hollywood/library/scenarios/music/music_backend_api/music_queue/music_queue.h>

namespace NAlice::NHollywood::NMusic {

// AVATAR_COLORS_PROXY
class TBeforeAvatarColorsRequestHelper : private TBeforeHttpRequestHelper {
public:
    using TBeforeHttpRequestHelper::TBeforeHttpRequestHelper;
    TStringBuf GetRequestItemName() const override { return MUSIC_AVATAR_COLORS_REQUEST_ITEM; }
    TStringBuf Name() const override { return "AvatarColors"; }

    void TryAddRequestFromBassState(const NJson::TJsonValue& bassState);
    void AddRequest(const TStringBuf coverUri);
};

class TAfterAvatarColorsRequestHelper : public TAfterHttpJsonRequestHelper {
public:
    using TAfterHttpJsonRequestHelper::TAfterHttpJsonRequestHelper;
    TStringBuf GetResponseItemName() const override { return MUSIC_AVATAR_COLORS_RESPONSE_ITEM; }

    TString GetMainColor() const;
    TString GetSecondColor() const;
};

template<ERequestPhase RequestPhase>
using TAvatarColorsRequestHelper =
    TRequestHelperChooser<RequestPhase, TBeforeAvatarColorsRequestHelper, TAfterAvatarColorsRequestHelper>;


// MUSIC_SCENARIO_THIN_CONTENT_NEIGHBORING_TRACKS_PROXY
class TBeforeNeighboringTracksRequestHelper : private TBeforeHttpRequestHelper {
public:
    using TBeforeHttpRequestHelper::TBeforeHttpRequestHelper;
    using TBeforeHttpRequestHelper::AddRequest;
    TStringBuf GetRequestItemName() const override { return MUSIC_NEIGHBORING_TRACKS_REQUEST_ITEM; }
    TStringBuf Name() const override { return "NeighboringTracks"; };

    static std::pair<int, int> CalculatePageIdxAndSize(int left, int right);
};

class TAfterNeighboringTracksRequestHelper : public TAfterHttpJsonRequestHelper {
public:
    using TAfterHttpJsonRequestHelper::TAfterHttpJsonRequestHelper;
    TStringBuf GetResponseItemName() const override { return MUSIC_NEIGHBORING_TRACKS_RESPONSE_ITEM; }

    TDeque<const NJson::TJsonValue*> GetTracks(int left, int right) const;
};

template<ERequestPhase RequestPhase>
using TNeighboringTracksRequestHelper =
    TRequestHelperChooser<RequestPhase, TBeforeNeighboringTracksRequestHelper, TAfterNeighboringTracksRequestHelper>;


// MUSIC_SCENARIO_LIKES_TRACKS_PROXY
class TBeforeLikesTracksRequestHelper : private TBeforeMusicHttpRequestHelper {
public:
    using TBeforeMusicHttpRequestHelper::TBeforeMusicHttpRequestHelper;
    TStringBuf GetRequestItemName() const override { return MUSIC_LIKES_TRACKS_REQUEST_ITEM; }
    TStringBuf Name() const override { return "LikesTracks"; }

    using TBeforeMusicHttpRequestHelper::SetUseCache;
    void AddRequest();
};

class TAfterLikesTracksRequestHelper : public TAfterHttpJsonRequestHelper {
public:
    using TAfterHttpJsonRequestHelper::TAfterHttpJsonRequestHelper;
    TStringBuf GetResponseItemName() const override { return MUSIC_LIKES_TRACKS_RESPONSE_ITEM; }

    THashSet<TStringBuf> GetTrackIds() const;
};

template<ERequestPhase RequestPhase>
using TLikesTracksRequestHelper =
    TRequestHelperChooser<RequestPhase, TBeforeLikesTracksRequestHelper, TAfterLikesTracksRequestHelper>;


// MUSIC_SCENARIO_DISLIKES_TRACKS_PROXY
class TBeforeDislikesTracksRequestHelper : private TBeforeMusicHttpRequestHelper {
public:
    using TBeforeMusicHttpRequestHelper::TBeforeMusicHttpRequestHelper;
    TStringBuf GetRequestItemName() const override { return MUSIC_DISLIKES_TRACKS_REQUEST_ITEM; }
    TStringBuf Name() const override { return "DislikesTracks"; }

    using TBeforeMusicHttpRequestHelper::SetUseCache;
    void AddRequest();
};

class TAfterDislikesTracksRequestHelper : public TAfterHttpJsonRequestHelper {
public:
    using TAfterHttpJsonRequestHelper::TAfterHttpJsonRequestHelper;
    TStringBuf GetResponseItemName() const override { return MUSIC_DISLIKES_TRACKS_RESPONSE_ITEM; }

    THashSet<TStringBuf> GetTrackIds() const;
};

template<ERequestPhase RequestPhase>
using TDislikesTracksRequestHelper =
    TRequestHelperChooser<RequestPhase, TBeforeDislikesTracksRequestHelper, TAfterDislikesTracksRequestHelper>;

} // namespace NAlice::NHollywood::NMusic
