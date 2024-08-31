#include "requests_helper.h"

#include <alice/hollywood/library/scenarios/music/music_backend_api/api_path/api_path.h>
#include <alice/hollywood/library/scenarios/music/music_sdk_handles/nlg_data_builder/nlg_data_builder.h>
#include <alice/hollywood/library/scenarios/music/util/util.h>

namespace NAlice::NHollywood::NMusic {

namespace {

TDeque<const NJson::TJsonValue*> CollectTracksFromArray(const NJson::TJsonValue::TArray& jsonArr) {
    TDeque<const NJson::TJsonValue*> result;
    for (const auto& trackObj : jsonArr) {
        if (const auto* innerTrackObj = trackObj.GetValueByPath("track")) {
            result.push_back(innerTrackObj);
        } else {
            result.push_back(&trackObj);
        }
    }
    return result;
}

TDeque<const NJson::TJsonValue*> CollectTracksFromJsonResponse(const NJson::TJsonValue& jsonObj) {
    // the most popular tracks place
    if (const auto* tracks = jsonObj.GetValueByPath("result.tracks"); tracks && tracks->IsArray()) {
        return CollectTracksFromArray(tracks->GetArraySafe());
    }

    // volumes are in non-shuffled albums
    if (const auto* volumes = jsonObj.GetValueByPath("result.volumes"); volumes && volumes->IsArray()) {
        TDeque<const NJson::TJsonValue*> result;
        for (const auto& volume : volumes->GetArraySafe()) {
            TDeque<const NJson::TJsonValue*> volumeTracks = CollectTracksFromArray(volume.GetArraySafe());
            std::copy(std::begin(volumeTracks), std::end(volumeTracks), std::back_inserter(result));
        }
        return result;
    }

    return {};
}

THashSet<TStringBuf> ParseLikesOrDislikesTrackIds(const NJson::TJsonValue& jsonObj) {
    THashSet<TStringBuf> ids;
    if (const auto* tracks = jsonObj.GetValueByPath("result.library.tracks")) {
        for (const auto& value : tracks->GetArray()) {
            ids.insert(value["id"].GetString());
        }
    }
    return ids;
}

} // namespace

// AVATAR_COLORS_PROXY
void TBeforeAvatarColorsRequestHelper::TryAddRequestFromBassState(const NJson::TJsonValue& bassState) {
    if (const auto coverUri = NMusicSdk::TryGetCoverUriFromBassState(bassState)) {
        if (*coverUri != DEFAULT_COVER_URI) {
            AddRequest(*coverUri);
        }
    }
}

void TBeforeAvatarColorsRequestHelper::AddRequest(const TStringBuf coverUri) {
    if (coverUri != DEFAULT_COVER_URI) {
        const auto path = NApiPath::ConstructMetaPathFromCoverUri(coverUri);
        TBeforeHttpRequestHelper::AddRequest(path);
    }
}

TString TAfterAvatarColorsRequestHelper::GetMainColor() const {
    Y_ENSURE(HasResponse());
    if (const auto& v = (*TryGetResponse())["MainColor"]; v.IsString()) {
        return v.GetString();
    }
    return Default<TString>();
}

TString TAfterAvatarColorsRequestHelper::GetSecondColor() const {
    Y_ENSURE(HasResponse());
    if (const auto& v = (*TryGetResponse())["SecondColor"]; v.IsString()) {
        return v.GetString();
    }
    return Default<TString>();
}


// MUSIC_SCENARIO_THIN_CONTENT_NEIGHBORING_TRACKS_PROXY
std::pair<int, int> TBeforeNeighboringTracksRequestHelper::CalculatePageIdxAndSize(int left, int right) {
    int startPageSize = right - left + 1;
    for (int pageSize = startPageSize; pageSize < 2 * startPageSize; ++pageSize) {
        int leftPageIdx = left / pageSize;
        int rightPageIdx = right / pageSize;
        if (leftPageIdx == rightPageIdx) {
            return {leftPageIdx, pageSize};
        }
    }
    return {0, right + 1};
}

TDeque<const NJson::TJsonValue*> TAfterNeighboringTracksRequestHelper::GetTracks(int left, int right) const {
    if (const auto& jsonObj = TryGetResponse()) {
        auto tracks = CollectTracksFromJsonResponse(*jsonObj);

        // we asked for a page [actualLeft ... actualRight], which contains [left ... right] tracks,
        // so we should drop extra tracks on the sides.
        const auto [pageIdx, pageSize] = TBeforeNeighboringTracksRequestHelper::CalculatePageIdxAndSize(left, right);
        int actualLeft = pageIdx * pageSize;
        int actualRight = actualLeft + tracks.size() - 1;

        // drop extra tracks
        while (actualRight > right) {
            tracks.pop_back();
            --actualRight;
        }

        while (actualLeft < left) {
            tracks.pop_front();
            ++actualLeft;
        }

        return tracks;
    }
    return {};
}

// MUSIC_SCENARIO_LIKES_TRACKS_PROXY
void TBeforeLikesTracksRequestHelper::AddRequest() {
    Y_ENSURE(MusicArgs_);
    const TString path = NApiPath::LikesTracks(MusicArgs_->GetAccountStatus().GetUid());
    TBeforeMusicHttpRequestHelper::AddRequest(path);
}

THashSet<TStringBuf> TAfterLikesTracksRequestHelper::GetTrackIds() const {
    if (const auto& jsonObj = TryGetResponse()) {
        return ParseLikesOrDislikesTrackIds(*jsonObj);
    }
    return {};
}

// MUSIC_SCENARIO_DISLIKES_TRACKS_PROXY
void TBeforeDislikesTracksRequestHelper::AddRequest() {
    Y_ENSURE(MusicArgs_);
    const TString path = NApiPath::DislikesTracks(MusicArgs_->GetAccountStatus().GetUid());
    TBeforeMusicHttpRequestHelper::AddRequest(path);
}

THashSet<TStringBuf> TAfterDislikesTracksRequestHelper::GetTrackIds() const {
    if (const auto& jsonObj = TryGetResponse()) {
        return ParseLikesOrDislikesTrackIds(*jsonObj);
    }
    return {};
}

} // NAlice::NHollywood::NMusic
