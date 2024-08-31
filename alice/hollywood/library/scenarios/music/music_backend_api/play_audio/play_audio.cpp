#include "play_audio.h"

#include <alice/hollywood/library/scenarios/music/time_util/time_util.h>
#include <alice/library/json/json.h>

namespace NAlice::NHollywood::NMusic {

TPlayAudioJsonBuilder::TPlayAudioJsonBuilder(const google::protobuf::Struct& playAudio)
    : Json(JsonFromProto(playAudio)) {
}

TPlayAudioJsonBuilder::TPlayAudioJsonBuilder(const NJson::TJsonValue& playAudio)
    : Json(playAudio) {
}

TPlayAudioJsonBuilder& TPlayAudioJsonBuilder::Timestamp(TInstant timestamp) {
    Json["timestamp"] = FormatTInstant(timestamp);
    return *this;
}

TPlayAudioJsonBuilder& TPlayAudioJsonBuilder::PlayedSec(float playedSec) {
    Json["totalPlayedSeconds"] = playedSec;
    return *this;
}

TPlayAudioJsonBuilder& TPlayAudioJsonBuilder::PositionSec(float positionSec) {
    Json["endPositionSeconds"] = positionSec;
    return *this;
}

TPlayAudioJsonBuilder& TPlayAudioJsonBuilder::StartPositionSec(float startPositionSec) {
    Json["startPositionSeconds"] = startPositionSec;
    return *this;
}

TPlayAudioJsonBuilder& TPlayAudioJsonBuilder::DurationSec(float durationSec) {
    Json["trackLengthSeconds"] = durationSec;
    return *this;
}

TString TPlayAudioJsonBuilder::BuildJsonString() {
    NJson::TJsonValue plays;
    plays["plays"] = NJson::JSON_ARRAY;
    plays["plays"].AppendValue(std::move(Json));
    return JsonToString(plays);
}

TPlayAudioEventBuilder& TPlayAudioEventBuilder::From(const TString& from) {
    Proto_.SetFrom(from);
    return *this;
}

TPlayAudioEventBuilder& TPlayAudioEventBuilder::Context(const TString& context) {
    Proto_.SetContext(context);
    return *this;
}

TPlayAudioEventBuilder& TPlayAudioEventBuilder::ContextItem(const TString& contextItem) {
    Proto_.SetContextItem(contextItem);
    return *this;
}

TPlayAudioEventBuilder& TPlayAudioEventBuilder::AlbumId(const TString& albumId) {
    Proto_.SetAlbumId(albumId);
    return *this;
}

TPlayAudioEventBuilder& TPlayAudioEventBuilder::TrackId(const TString& trackId) {
    Proto_.SetTrackId(trackId);
    return *this;
}

TPlayAudioEventBuilder& TPlayAudioEventBuilder::ArtistId(const TString& artistId) {
    Proto_.SetArtistId(artistId);
    return *this;
}

TPlayAudioEventBuilder& TPlayAudioEventBuilder::AlbumType(const TString& albumType) {
    Proto_.SetAlbumType(albumType);
    return *this;
}

TPlayAudioEventBuilder& TPlayAudioEventBuilder::PlayId(const TString& playId) {
    Proto_.SetPlayId(playId);
    return *this;
}

TPlayAudioEventBuilder& TPlayAudioEventBuilder::Uid(const TString& uid) {
    Proto_.SetUid(uid);
    return *this;
}

TPlayAudioEventBuilder& TPlayAudioEventBuilder::TotalPlayedSeconds(float totalPlayedSeconds) {
    Proto_.SetTotalPlayedSeconds(totalPlayedSeconds);
    return *this;
}

TPlayAudioEventBuilder& TPlayAudioEventBuilder::TrackLengthSeconds(float trackLengthSeconds) {
    Proto_.SetTrackLengthSeconds(trackLengthSeconds);
    return *this;
}

TPlayAudioEventBuilder& TPlayAudioEventBuilder::EndPositionSeconds(float endPositionSeconds) {
    Proto_.SetEndPositionSeconds(endPositionSeconds);
    return *this;
}

TPlayAudioEventBuilder& TPlayAudioEventBuilder::PlaylistId(const TString& playlistId) {
    Proto_.SetPlaylistId(playlistId);
    return *this;
}

TPlayAudioEventBuilder& TPlayAudioEventBuilder::RadioSessionId(const TString& radioSessionId) {
    Proto_.SetRadioSessionId(radioSessionId);
    return *this;
}

TPlayAudioEventBuilder& TPlayAudioEventBuilder::BatchId(const TString& batchId) {
    Proto_.SetBatchId(batchId);
    return *this;
}

TPlayAudioEventBuilder& TPlayAudioEventBuilder::Incognito(const bool incognito) {
    Proto_.SetIncognito(incognito);
    return *this;
}

TPlayAudioEventBuilder& TPlayAudioEventBuilder::ShouldSaveProgress(const bool shouldSaveProgress) {
    Proto_.SetShouldSaveProgress(shouldSaveProgress);
    return *this;
}

TPlayAudioEvent TPlayAudioEventBuilder::BuildProto() {
    return std::move(Proto_);
}

} // namespace NAlice::NHollywood::NMusic
