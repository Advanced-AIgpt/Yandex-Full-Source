#pragma once

#include <alice/library/video_common/frontend_vh_helpers/util.h>
#include <alice/library/video_common/defs.h>

#include <alice/hollywood/library/base_scenario/scenario.h>
#include <alice/megamind/protos/common/data_source_type.pb.h>
#include <alice/megamind/protos/scenarios/directives.pb.h>
#include <alice/protos/data/video/video.pb.h>


namespace NAlice::NVideoCommon {

struct TVhData {
public:
    static TMaybe<TVhData> ParseJsonDoc(const NJson::TJsonValue& doc, bool isParentChannel = false);

    static TVideoItem
    GetVideoItem(const TVhData& data);
    TVideoItem
    GetVideoItem() const;

    NScenarios::TVideoPlayDirective
    MakeVideoPlayDirective(const NHollywood::TScenarioRunRequestWrapper& request) const;
    NScenarios::TVideoPlayDirective
    MakeVideoPlayDirective(const NScenarios::TScenarioRunRequest& request) const;
    NScenarios::TShowVideoDescriptionDirective
    MakeShowVideoDescriptionDirective() const;

    bool HasDescription() const;
    const TVhData& GetPlayableVhPlayerData() const;
    TVhData& GetPlayableVhPlayerData();
    ui64 GetStartAt(const TVideoItem& item, ui64 serverTime, const TDeviceState& deviceState) const;
    static ui64 GetStartAt(const TVhData& vhPlayerData, const TVideoItem& item, ui64 serverTime, const TDeviceState& deviceState);
    TString MakeKinopoiskPayload(ui64 watchProgressPosition) const;
    void SetupAudioAndSubtitles(NScenarios::TVideoPlayDirective& videoPlayDirective, const NAlice::TVideoItem& item, const NHollywood::TScenarioRunRequestWrapper& request) const; // old with mordovia support method
    void SetupAudioAndSubtitles(NScenarios::TVideoPlayDirective& videoPlayDirective, const NAlice::TVideoItem& item, const TDeviceState& deviceState) const;

    TVector<std::unique_ptr<TVhData>> Includes;
    std::unique_ptr<TVhData> ActualEpisode;
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

    NAlice::NVideoCommon::EContentType VideoType;
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

    TString ChannelType;
    TString OntoId;

    ui32 SeasonNumber;
    ui32 EpisodeNumber;

    NJson::TJsonValue DrmConfig;

    TVector<NAlice::TAudioStreamOrSubtitle> AudioStreams;
    TVector<NAlice::TAudioStreamOrSubtitle> Subtitles;

    TVector<NAlice::TSkippableFragment> SkippableFragments;

    NAlice::TPlayerRestrictionConfig PlayerRestrictionConfig;
    NJson::TJsonValue OttParams;

private:
    static void ParseCommonData(const NJson::TJsonValue& doc, TVhData& vhPlayerData, bool isParentChannel);
    static void ParseStreams(const NJson::TJsonValue& doc, TVhData& vhPlayerData);
    static void ParseFilmOrSerialData(const NJson::TJsonValue& doc, TVhData& vhPlayerData);
    static void ParseChannelData(const NJson::TJsonValue& doc, TVhData& vhPlayerData);
    static void ParseLicense(const NJson::TJsonValue& doc, TVhData& vhPlayerData);
};

struct TVhSeasonData {
    TVhSeasonData(const NJson::TJsonValue& season);

    bool ContainsEpisode(ui32 episodeNumber) const {
        return episodeNumber <= EpisodesCount;
    }

    ui32 SeasonNumber;
    ui32 EpisodesCount;
    TString ContentId;
    TString SeriesContentId;
    NJson::TJsonValue FirstEpisode;
};

struct TVhEpisodeData {
    TVhEpisodeData(const NJson::TJsonValue& episode) {
        ContentId = episode["content_id"].GetString();
    }
    TString ContentId;
};

class TVideoItemHelper final {
public:
    static TMaybe<TVideoItemHelper> TryMakeFromVhPlayerResponse(const NJson::TJsonValue& frontendVhResponse);
    static TMaybe<TVideoItemHelper> TryMakeFromVhResponse(const NJson::TJsonValue& frontendVhResponse);

    inline NAlice::NScenarios::TVideoPlayDirective MakeVideoPlayDirective(const NAlice::NHollywood::TScenarioRunRequestWrapper& request) const {
        return VhPlayerData.MakeVideoPlayDirective(request);
    }
    inline NAlice::NScenarios::TVideoPlayDirective MakeVideoPlayDirective(const NScenarios::TScenarioRunRequest& request) const {
        return VhPlayerData.MakeVideoPlayDirective(request);
    }
    NAlice::NScenarios::TShowVideoDescriptionDirective MakeShowVideoDescriptionDirective() const {
        return VhPlayerData.MakeShowVideoDescriptionDirective();
    }

    TVideoItem GetVideoItem() const {return VhPlayerData.GetVideoItem();}
    ui32 GetAgeRestriction() const {return VhPlayerData.RestrictionAge;}
    bool HasDescription() const {return VhPlayerData.HasDescription();}
    bool GetIsPaidContent() const {return VhPlayerData.IsPaidContent;}
    bool GetHasActiveLicense() const {return VhPlayerData.HasActiveLicense;}
    const TString& GetOntoId() const {return VhPlayerData.OntoId;}
    const TString& GetUuid() const {return VhPlayerData.Uuid;}
    TVhData& GetPlayableVhPlayerData() {return VhPlayerData.GetPlayableVhPlayerData();}
    const TVhData& GetPlayableVhPlayerData() const {return VhPlayerData.GetPlayableVhPlayerData();}
    TVhData& GetVhPlayerData() {return VhPlayerData;}
    const TVhData& GetVhPlayerData() const {return VhPlayerData;}

    void AddSubtitlesAndAudioTracks(const NJson::TJsonValue& ottResponse);

private:
    TVideoItemHelper() = default;
    TVhData VhPlayerData;
};

class TVhSeriesHelper {
public:
    static TMaybe<TVhSeriesHelper> TryMakeFromVhSeriesResponse(const NJson::TJsonValue& response);

    TMaybe<TVhSeasonData> GetSeasonByNumber(ui32 number) const;
    TMaybe<TVideoItem> GetSeriesVideoItem() const;

private:
    NJson::TJsonValue Series_;
    NJson::TJsonValue::TArray Seasons_;
};

TMaybe<TVhEpisodeData> GetEpisodeFromVhEpisodesResponse(const NJson::TJsonValue& response, ui32 episodeNumber);

} // namespace NAlice::NVideoCommon
