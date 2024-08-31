#pragma once

#include <alice/hollywood/library/scenarios/music/proto/play_audio.pb.h>

#include <library/cpp/json/json_value.h>

#include <google/protobuf/struct.pb.h>

#include <util/datetime/base.h>

namespace NAlice::NHollywood::NMusic {

//Building proto from protobuf::Struct is really expensive,
//in the end we need a json, so this is kind of shortcut.
class TPlayAudioJsonBuilder {
public:
    TPlayAudioJsonBuilder(const google::protobuf::Struct& playAudio);

    TPlayAudioJsonBuilder(const NJson::TJsonValue& playAudio);

    TPlayAudioJsonBuilder& Timestamp(TInstant timestamp);

    TPlayAudioJsonBuilder& PlayedSec(float playedSec);

    TPlayAudioJsonBuilder& PositionSec(float positionSec);

    TPlayAudioJsonBuilder& StartPositionSec(float startPositionSec);

    TPlayAudioJsonBuilder& DurationSec(float durationSec);

    TString BuildJsonString();

private:
    NJson::TJsonValue Json;
};

//This builder is used to build proto which is then used to fill protobuf::Struct
class TPlayAudioEventBuilder {
public:

    TPlayAudioEventBuilder() = default;

    TPlayAudioEventBuilder(TPlayAudioEvent&& proto)
        : Proto_(std::move(proto)) {
    }

    TPlayAudioEventBuilder& From(const TString& from);

    TPlayAudioEventBuilder& Context(const TString& context);

    TPlayAudioEventBuilder& ContextItem(const TString& contextItem);

    TPlayAudioEventBuilder& AlbumId(const TString& albumId);

    TPlayAudioEventBuilder& TrackId(const TString& trackId);

    TPlayAudioEventBuilder& ArtistId(const TString& artistId);

    TPlayAudioEventBuilder& AlbumType(const TString& albumType);

    TPlayAudioEventBuilder& PlayId(const TString& playId);

    TPlayAudioEventBuilder& Uid(const TString& uid);

    TPlayAudioEventBuilder& TotalPlayedSeconds(float totalPlayedSeconds);

    TPlayAudioEventBuilder& TrackLengthSeconds(float trackLengthSeconds);

    TPlayAudioEventBuilder& EndPositionSeconds(float endPositionSeconds);

    TPlayAudioEventBuilder& PlaylistId(const TString& playlistId);

    TPlayAudioEventBuilder& RadioSessionId(const TString& radioSessionId);

    TPlayAudioEventBuilder& BatchId(const TString& batchId);

    TPlayAudioEventBuilder& Incognito(const bool incognito);

    TPlayAudioEventBuilder& ShouldSaveProgress(const bool shouldSaveProgress);

    TPlayAudioEvent BuildProto();

private:
    TPlayAudioEvent Proto_;
};

} // namespace NAlice::NHollywood::NMusic
