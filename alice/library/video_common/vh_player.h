#pragma once

#include <alice/library/video_common/audio_and_subtitle_helper.h>
#include <alice/library/video_common/defs.h>
#include <alice/megamind/protos/common/data_source_type.pb.h>
#include <alice/megamind/protos/common/device_state.pb.h>
#include <alice/megamind/protos/scenarios/directives.pb.h>
#include <alice/protos/data/video/video.pb.h>

#include <library/cpp/json/writer/json_value.h>

#include <util/datetime/base.h>
#include <util/generic/ptr.h>

namespace NAlice::NVideoCommon {

class TVhPlayerData {

public:
    bool IsFilled() const { return !Uuid.empty(); }

    // Types
    bool IsFilm()          const { return VideoType == EContentType::Movie;         }
    bool IsTvShow()        const { return VideoType == EContentType::TvShow;        }
    bool IsTvShowEpisode() const { return VideoType == EContentType::TvShowEpisode; }
    bool IsTvStream()      const { return VideoType == EContentType::TvStream;      }
    bool IsVideo()         const { return VideoType == EContentType::Video;         }
    // Providers
    bool IsKinopoisk()  const { return IsFilm() || IsTvShow() || IsTvShowEpisode(); }
    bool IsVh()         const { return IsTvStream() || IsVideo(); }

    // Parser
    bool ParseJsonDoc(const NJson::TJsonValue& doc, bool isParentChannel = false);

    // Items
    NAlice::TVideoItem MakeProtoVideoItem() const;

    // PlayableVhPlayerData
    const TVhPlayerData& GetPlayableVhPlayerData() const;
    bool IsPlayableVhPlayerData() const { return this == &GetPlayableVhPlayerData(); }

    // Payload
    TString MakePayload(ui64 startTime) const;
    TString MakeKinopoiskPayload(ui64 startTime) const;
    TString MakeVhPayload() const { return Payload; }

    // Getters
    ui64 GetStartAt(ui64 timeNow) const;

    // Inner parsers
    void ParseCommonData(const NJson::TJsonValue& doc, bool isParentChannel = false);
    void ParseStreams(const NJson::TJsonValue& doc);
    void ParseFilmOrSerialData(const NJson::TJsonValue& doc);
    void ParseLicense(const NJson::TJsonValue& doc);
    void ParseChannelData(const NJson::TJsonValue& doc);

    // Fields
    TVector<std::unique_ptr<TVhPlayerData>> Includes;
    std::unique_ptr<TVhPlayerData> ActualEpisode;
    TString Title;
    TString Uuid;
    TString PlayUri;
    TString StreamType;
    TString TvShowSeasonId;
    TString TvShowItemId;

    TString Payload;

    TString Description;
    ui32 Duration;
    ui32 StartAt;

    ui32 StartTimestamp;
    ui32 EndTimestamp;
    ui32 StartPosition;

    EContentType VideoType;
    TString SessionId;

    TString CoverUrl16X9;
    TString ThumbnailUrl16X9;
    TString CoverUrl2X3;

    ui32 RestrictionAge;
    ui32 ReleaseYear;

    TString Genre;
    TString Directors;
    TString Actors;

    double RatingKP;

    bool IsPaidContent;
    bool HasActiveLicense;
    TString PurchaseTag;

    TString ChannelType;
    TString OntoId;

    ui32 SeasonNumber;
    ui32 EpisodeNumber;

    NJson::TJsonValue DrmConfig;
    NJson::TJsonValue OttParams;

    TVector<NAlice::TSkippableFragment> SkippableFragments;

    TVector<NAlice::TAudioStreamOrSubtitle> AudioStreams;
    TVector<NAlice::TAudioStreamOrSubtitle> Subtitles;

    NAlice::TPlayerRestrictionConfig PlayerRestrictionConfig;
};

} // namespace NAlice::NVideoCommon
