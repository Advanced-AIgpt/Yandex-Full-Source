#include "video_item_helper.h"

#include <alice/bass/libs/client/experimental_flags.h>

#include <alice/library/json/json.h>
#include <alice/library/video_common/audio_and_subtitle_helper.h>
#include <alice/library/video_common/video_helper.h>

#include <util/string/join.h>
#include <google/protobuf/wrappers.pb.h>

using namespace NAlice::NScenarios;
using namespace NAlice::NHollywood;

namespace NAlice::NVideoCommon {

constexpr TStringBuf YATV_CHANNEL_PREFIX = "yatv";
constexpr TStringBuf VIDEO_SELECTION_FRAME = "alice.mordovia_video_selection";


// parsers section
TMaybe<TVhData> TVhData::ParseJsonDoc(const NJson::TJsonValue& doc, bool isParentChannel) {
    if (doc.IsNull() || !doc.IsMap()) {
        return Nothing();
    }
    auto vhPlayerData = MakeMaybe<TVhData>();

    ParseCommonData(doc, vhPlayerData.GetRef(), isParentChannel);
    if (vhPlayerData->Uuid.empty()) {
        return Nothing();
    }

    ParseStreams(doc, vhPlayerData.GetRef());
    if (vhPlayerData->VideoType == EContentType::Movie
        || vhPlayerData->VideoType == EContentType::TvShow
        || vhPlayerData->VideoType == EContentType::TvShowEpisode) {
        ParseFilmOrSerialData(doc, vhPlayerData.GetRef());
    }
    ParseLicense(doc, vhPlayerData.GetRef());

    if (const auto& includes = doc["includes"]; includes.IsArray()) {
        for (const auto& include : includes.GetArray()) {
            if (auto vhData = ParseJsonDoc(include)) {
                vhPlayerData->Includes.emplace_back(
                    std::make_unique<TVhData>(std::move(*vhData.Get())));
            }
        }
    }

    if (vhPlayerData->VideoType == EContentType::TvStream) {
        ParseChannelData(doc, vhPlayerData.GetRef());
    }

    return vhPlayerData;
}
void TVhData::ParseCommonData(const NJson::TJsonValue& doc, TVhData& vhPlayerData, bool isParentChannel) {
    vhPlayerData.Title = doc["title"].GetString();
    vhPlayerData.Uuid = doc["content_id"].GetString();
    vhPlayerData.RestrictionAge = doc["restriction_age"].GetUInteger();

    vhPlayerData.Description = doc["description"].GetString();
    vhPlayerData.Duration = doc["duration"].GetInteger();
    vhPlayerData.StartAt = doc["start_at"].GetInteger();

    vhPlayerData.StartTimestamp = doc["start_time"].GetInteger();
    vhPlayerData.EndTimestamp = doc["end_time"].GetInteger();
    vhPlayerData.OntoId = doc["onto_id"].GetString();

    const auto& ontoType = doc["onto_otype"].GetString();
    const auto& contentTypeName = doc["content_type_name"].GetString();
    if (ontoType == "Film/Film") {
        vhPlayerData.VideoType = EContentType::Movie;
    } else if (ontoType == "Film/Series@on" || ontoType == "Film/Season@on" /* ALICEASSESSORS-5089 */) {
        if (doc["type"].GetString() == "series") {
            vhPlayerData.VideoType = EContentType::TvShow;
        } else {
            vhPlayerData.VideoType = EContentType::TvShowEpisode;
        }
    } else if (contentTypeName == "channel" || isParentChannel) {
        vhPlayerData.VideoType = EContentType::TvStream;
    } else {
        vhPlayerData.VideoType = EContentType::Video;
    }
    vhPlayerData.SkippableFragments = ParseSkippableFragments(doc["skippableFragments"]);

    // Delete last skippable fragment in movie
    if (vhPlayerData.VideoType == EContentType::Movie) {
        if (!vhPlayerData.SkippableFragments.empty() &&
            vhPlayerData.SkippableFragments.back().GetEndTime() >= vhPlayerData.Duration - SKIP_LAST_FRAGMENT_MAX_ERROR)
        {
            vhPlayerData.SkippableFragments.pop_back();
        }
    }

    if (vhPlayerData.VideoType == EContentType::TvShowEpisode) {
        if (!vhPlayerData.SkippableFragments.empty() &&
            vhPlayerData.SkippableFragments.back().GetEndTime() >= vhPlayerData.Duration - SKIP_LAST_FRAGMENT_MAX_ERROR)
        {
            vhPlayerData.SkippableFragments.back().SetType("next_episode");
        }
    }
}
void TVhData::ParseStreams(const NJson::TJsonValue& doc, TVhData& vhPlayerData) {
    const auto& streams = doc["streams"];
    const auto* playUriPtr = streams.GetValueByPath("[0].url");
    if (!playUriPtr) {
        return;
    }
    vhPlayerData.PlayUri = playUriPtr->GetString();
    if (const auto* streamTypePtr = streams.GetValueByPath("[0].stream_type")) {
        vhPlayerData.StreamType = streamTypePtr->GetString();
    }

    NJson::TJsonValue payloadJson;
    payloadJson["streams"] = streams;
    vhPlayerData.Payload = (TStringBuilder() << payloadJson);

    if (const auto* sessionIdPointer = streams.GetValueByPath("[0].drmConfig.requestParams.watchSessionId")) {
        vhPlayerData.SessionId = sessionIdPointer->GetString();
    }
    if (const auto* drmConfigPointer = streams.GetValueByPath("[0].drmConfig")) {
        vhPlayerData.DrmConfig = *drmConfigPointer;
    }
}
void TVhData::ParseFilmOrSerialData(const NJson::TJsonValue& doc, TVhData& vhPlayerData) {
    vhPlayerData.CoverUrl16X9 = NAlice::NVideoCommon::BuildResizedThumbnailUri(doc["thumbnail"].GetString(), "1920x1080");
    vhPlayerData.CoverUrl2X3 = NAlice::NVideoCommon::BuildResizedThumbnailUri(doc["onto_poster"].GetString(), "320x480");
    vhPlayerData.ThumbnailUrl16X9 = NAlice::NVideoCommon::BuildThumbnailUri(doc["thumbnail"].GetString());

    vhPlayerData.ReleaseYear = doc["release_year"].GetUInteger();
    vhPlayerData.RatingKP = doc["rating_kp"].GetDouble();
    vhPlayerData.Directors = doc["directors"].GetString();
    vhPlayerData.Actors = doc["actors"].GetString();

    if (const auto& genres = doc["genres"]; genres.IsArray()) {
        vhPlayerData.Genre = JoinSeq(", ", genres.GetArray());
        SubstGlobal(vhPlayerData.Genre, "\"", "");
    }
    if (const auto& seasonInfo = doc["season"]; seasonInfo.IsMap()) {
        vhPlayerData.SeasonNumber = seasonInfo["season_number"].GetUInteger();
    }
    vhPlayerData.EpisodeNumber = doc["episode_number"].GetUInteger();
    if (const auto* seriesIdPtr = doc.GetValueByPath("series.id")) {
        vhPlayerData.TvShowItemId = seriesIdPtr->GetString();
    }
    vhPlayerData.TvShowSeasonId = doc["parent_id"].GetString();
    vhPlayerData.OttParams = doc["ottParams"];
}
void TVhData::ParseChannelData(const NJson::TJsonValue& doc, TVhData& vhPlayerData) {
    vhPlayerData.ChannelType = doc["channel_type"].GetString();
    vhPlayerData.StartPosition = doc["start_position"].GetInteger();
    if (auto actualEpisode = ParseJsonDoc(doc["actual_episode"], /*isParentChannel*/ true)) {
        vhPlayerData.ActualEpisode =
            std::make_unique<TVhData>(std::move(*actualEpisode.Get()));
    }
}
void TVhData::ParseLicense(const NJson::TJsonValue& doc, TVhData& vhPlayerData) {
    if (const auto& ottParams = doc["ottParams"]; ottParams.IsMap()) {
        vhPlayerData.IsPaidContent = ottParams["monetizationModel"].GetString() != "AVOD";
        if (const auto& licenses = ottParams["licenses"]; licenses.IsArray()) {
            vhPlayerData.HasActiveLicense =
                AnyOf(licenses.GetArray(), [](const auto& license) { return license["active"].GetBoolean(); });
        }
    }
}
// end of parsers

// some get methods

bool TVhData::HasDescription() const {
    return this->VideoType == EContentType::Movie
           || this->VideoType == EContentType::TvShow
           || this->VideoType == EContentType::TvShowEpisode;
}

const TVhData& TVhData::GetPlayableVhPlayerData() const {
    if (!this->Includes.empty()) {
        return *this->Includes[0];
    } else if (this->ActualEpisode) {
        return *this->ActualEpisode;
    }
    return *this;
}
TVhData& TVhData::GetPlayableVhPlayerData() {
    if (!this->Includes.empty()) {
        return *this->Includes[0];
    } else if (this->ActualEpisode) {
        return *this->ActualEpisode;
    }
    return *this;
}
TVideoItem TVhData::GetVideoItem() const {
    // is get vhPlayerData method
    return GetVideoItem(*this);
}
TVideoItem TVhData::GetVideoItem(const TVhData& data) {
    TVideoItem item;
    item.SetName(data.Title);
    item.SetProviderItemId(data.Uuid);
    item.SetType(ToString(data.VideoType));
    item.SetDescription(data.Description);
    item.SetDuration(data.Duration);
    item.SetProviderName(TString(PROVIDER_STRM));
    item.SetPlayUri(data.PlayUri);
    item.SetMinAge(data.RestrictionAge);
    item.SetAgeLimit(ToString(data.RestrictionAge));
    item.SetAvailable(true);
    item.SetEntref(MakeEntref(data.OntoId));

    if (data.VideoType == EContentType::Movie
        || data.VideoType == EContentType::TvShow
        || data.VideoType == EContentType::TvShowEpisode) {
        auto* lightItem = item.MutableProviderInfo()->Add();
        lightItem->SetType(ToString(data.VideoType));
        lightItem->SetProviderItemId(data.Uuid);
        lightItem->SetProviderName(TString(PROVIDER_KINOPOISK));
        lightItem->SetAvailable(data.HasActiveLicense);
        item.SetProviderName(TString(PROVIDER_KINOPOISK));
        item.SetAvailable(data.HasActiveLicense);
        item.SetGenre(data.Genre);
        item.SetDirectors(data.Directors);
        item.SetActors(data.Actors);
        item.SetRating(data.RatingKP);
        item.SetReleaseYear(data.ReleaseYear);
        item.SetCoverUrl16x9(data.CoverUrl16X9);
        item.SetCoverUrl2x3(data.CoverUrl2X3);
        item.SetThumbnailUrl16x9(data.ThumbnailUrl16X9);
        if (data.VideoType == EContentType::TvShowEpisode
            || data.VideoType == EContentType::TvShow) {
            item.SetEpisode(data.EpisodeNumber);
            item.SetSeason(data.SeasonNumber);
            lightItem->SetEpisode(data.EpisodeNumber);
            lightItem->SetSeason(data.SeasonNumber);
            item.SetTvShowSeasonId(data.TvShowSeasonId);
            lightItem->SetTvShowSeasonId(data.TvShowSeasonId);
            item.SetTvShowItemId(data.TvShowItemId);
            lightItem->SetTvShowItemId(data.TvShowItemId);
        }
        for (const auto& audioStream : data.AudioStreams) {
            *item.AddAudioStreams() = audioStream;
        }
        for (const auto& subtitle : data.Subtitles) {
            *item.AddSubtitles() = subtitle;
        }
        for (const auto& skippableFragment : data.SkippableFragments) {
            *item.AddSkippableFragments() = skippableFragment;
        }
        *item.MutablePlayerRestrictionConfig() = data.PlayerRestrictionConfig;
    }

    return item;
}
ui64 TVhData::GetStartAt(const TVideoItem& item, ui64 serverTime, const TDeviceState& deviceState) const {
    return GetStartAt(*this, item, serverTime, deviceState);
}
ui64 TVhData::GetStartAt(const TVhData& vhPlayerData, const TVideoItem& item, ui64 serverTime, const TDeviceState& deviceState) {
    if (vhPlayerData.VideoType == EContentType::TvStream) {
        if (vhPlayerData.ChannelType.StartsWith(YATV_CHANNEL_PREFIX)) {
            return vhPlayerData.StartPosition;
        }
        return 0;
    }
    if (vhPlayerData.StartTimestamp && vhPlayerData.EndTimestamp) {
        TInstant startTime = TInstant::Seconds(vhPlayerData.StartTimestamp);
        TInstant endTime = TInstant::Seconds(vhPlayerData.EndTimestamp);
        const auto now = TInstant::MilliSeconds(serverTime);
        if (startTime <= now && now < endTime) {
            return now.Seconds();
        }
    }

    if (vhPlayerData.StartAt && vhPlayerData.Duration) {
        return CalculateStartAt(vhPlayerData.Duration, vhPlayerData.StartAt);
    }
    if (const auto lastWatched =
            FindVideoInLastWatched(item, deviceState)) {
        return CalculateStartAt(lastWatched->GetProgress().GetDuration(),
                                                      lastWatched->GetProgress().GetPlayed());
    }
    return 0;
}
TString TVhData::MakeKinopoiskPayload(ui64 watchProgressPosition) const {
    NJson::TJsonValue payload;
    NJson::TJsonValue stream;
    const auto& episodeVhData = this->GetPlayableVhPlayerData();
    stream["uri"] = episodeVhData.PlayUri;
    if (episodeVhData.DrmConfig.Has("requestParams")) {
        payload["streams"]["drmConfig"] = episodeVhData.DrmConfig;
        stream["drmType"] = "widevine";
        stream["drmConfig"] = episodeVhData.DrmConfig;
    }
    stream["streamType"] = episodeVhData.StreamType;
    payload["allStreams"].AppendValue(std::move(stream));
    payload["episodeNumber"] = episodeVhData.EpisodeNumber;
    payload["seasonNumber"] = episodeVhData.SeasonNumber;
    payload["rootUuid"] = episodeVhData.TvShowItemId;
    payload["uuid"] = episodeVhData.Uuid;
    payload["sessionId"] = episodeVhData.SessionId;
    payload["playerRestrictionConfig"]["subtitlesButtonEnable"] = static_cast<bool>(episodeVhData.PlayerRestrictionConfig.GetSubtitlesButtonEnable());


    NJson::TJsonValue trackings;
    trackings["sid"] = episodeVhData.SessionId;
    trackings["contentTypeId"] = episodeVhData.OttParams["contentTypeID"];
    trackings["monetizationModel"] = episodeVhData.OttParams["monetizationModel"];
    trackings["serviceName"] = episodeVhData.OttParams["serviceName"];
    if (episodeVhData.OttParams.Has("subscriptionType")) {
        trackings["subscriptionType"] = episodeVhData.OttParams["subscriptionType"];
    }
    trackings["puid"] = episodeVhData.OttParams["puid"];
    trackings["uuid"] = episodeVhData.OttParams["uuid"];
    trackings["yaUID"] = episodeVhData.OttParams["puid"];
    payload["trackings"] = std::move(trackings);

    payload["watchProgressPosition"] = watchProgressPosition;

    return payload.GetStringRobust();
}
void TVhData::SetupAudioAndSubtitles(TVideoPlayDirective& videoPlayDirective, const TVideoItem& item, const TScenarioRunRequestWrapper& request) const {
    if (item.GetType() == ToString(EContentType::TvShowEpisode)) {
        const auto& tvShowItem = GetVideoItem();
        const auto& lastWatched = FindTvShowInLastWatched(tvShowItem, request.BaseRequestProto().GetDeviceState());

        const auto& deviceState = request.Proto().GetBaseRequest().GetDeviceState();
        const auto& currentlyPlayingItem = deviceState.GetVideo().GetCurrentlyPlaying().GetRawItem();

        if (item.GetTvShowItemId() == currentlyPlayingItem.GetTvShowItemId()) {
            videoPlayDirective.MutableAudioLanguage()->set_value(currentlyPlayingItem.GetAudioLanguage());
            videoPlayDirective.MutableSubtitlesLanguage()->set_value(currentlyPlayingItem.GetSubtitlesLanguage());
        } else if (lastWatched) {
            videoPlayDirective.MutableAudioLanguage()->set_value(lastWatched->GetItem().GetAudioLanguage());
            videoPlayDirective.MutableSubtitlesLanguage()->set_value(lastWatched->GetItem().GetSubtitlesLanguage());
        }
    } else {
        const auto& lastWatched = FindVideoInLastWatched(item, request.BaseRequestProto().GetDeviceState());
        if (lastWatched) {
            videoPlayDirective.MutableAudioLanguage()->set_value(lastWatched->GetAudioLanguage());
            videoPlayDirective.MutableSubtitlesLanguage()->set_value(lastWatched->GetSubtitlesLanguage());
        }
    }

    if (request.Input().FindSemanticFrame(VIDEO_SELECTION_FRAME) && !request.HasExpFlag(NBASS::EXPERIMENTAL_FLAG_DISABLE_CHANGE_TRACK)) {
        const auto frame = request.Input().CreateRequestFrame(VIDEO_SELECTION_FRAME);

        auto audioLanguageSlot = frame.FindSlot(SLOT_AUDIO_LANGUAGE);
        auto subtitlesLanguageSlot = frame.FindSlot(SLOT_SUBTITLE_LANGUAGE);

        if (audioLanguageSlot) {
            if (const auto& bestTrack = FindMostSuitableTrack(this->GetPlayableVhPlayerData().AudioStreams,
                    audioLanguageSlot->Value.AsString()); bestTrack.Defined())
            {
                videoPlayDirective.MutableAudioLanguage()->set_value(bestTrack->GetLanguage());
            }
        }

        if (subtitlesLanguageSlot) {
            if (const auto& bestTrack = FindMostSuitableTrack(this->GetPlayableVhPlayerData().Subtitles,
                    subtitlesLanguageSlot->Value.AsString()); bestTrack.Defined())
            {
                videoPlayDirective.MutableSubtitlesLanguage()->set_value(bestTrack->GetLanguage());
            }
        }
    }
}
void TVhData::SetupAudioAndSubtitles(TVideoPlayDirective& videoPlayDirective, const TVideoItem& item, const TDeviceState& deviceState) const {
    if (item.GetType() == ToString(EContentType::TvShowEpisode)) {
        const auto& tvShowItem = GetVideoItem();
        const auto& lastWatched = FindTvShowInLastWatched(tvShowItem, deviceState);
        const auto& currentlyPlayingItem = deviceState.GetVideo().GetCurrentlyPlaying().GetRawItem();

        if (item.GetTvShowItemId() == currentlyPlayingItem.GetTvShowItemId()) {
            videoPlayDirective.MutableAudioLanguage()->set_value(currentlyPlayingItem.GetAudioLanguage());
            videoPlayDirective.MutableSubtitlesLanguage()->set_value(currentlyPlayingItem.GetSubtitlesLanguage());
        } else if (lastWatched) {
            videoPlayDirective.MutableAudioLanguage()->set_value(lastWatched->GetItem().GetAudioLanguage());
            videoPlayDirective.MutableSubtitlesLanguage()->set_value(lastWatched->GetItem().GetSubtitlesLanguage());
        }
    } else {
        const auto& lastWatched = FindVideoInLastWatched(item, deviceState);
        if (lastWatched) {
            videoPlayDirective.MutableAudioLanguage()->set_value(lastWatched->GetAudioLanguage());
            videoPlayDirective.MutableSubtitlesLanguage()->set_value(lastWatched->GetSubtitlesLanguage());
        }
    }
}
// end of get methods


// directives
TVideoPlayDirective TVhData::MakeVideoPlayDirective(const TScenarioRunRequestWrapper& request) const {
    const auto& playableVhData = GetPlayableVhPlayerData();
    auto item = GetVideoItem(playableVhData);
    TVideoPlayDirective directive;

    ui64 startTime = GetStartAt(item, request.ServerTimeMs(), request.BaseRequestProto().GetDeviceState());
    TString payload = this->Payload;

    if (IsIn({EContentType::Movie, EContentType::TvShow, EContentType::TvShowEpisode}, playableVhData.VideoType))
    {
        payload = MakeKinopoiskPayload(startTime);
        if (playableVhData.VideoType == EContentType::TvShowEpisode) {
            auto tvShowItem = GetVideoItem();
            *directive.MutableTvShowItem() = std::move(tvShowItem);
            directive.MutableNextItem();
        }
    }
    directive.SetUri(item.GetPlayUri());
    directive.SetPayload(std::move(payload));
    directive.SetStartAt(startTime);
    directive.SetSessionToken(playableVhData.SessionId);
    SetupAudioAndSubtitles(directive, item, request);

    *directive.MutableItem() = std::move(item);
    return directive;
}
TVideoPlayDirective TVhData::MakeVideoPlayDirective(const NScenarios::TScenarioRunRequest& request) const {
    const auto& playableVhData = GetPlayableVhPlayerData();
    auto item = GetVideoItem(playableVhData);
    TVideoPlayDirective directive;

    ui64 startTime = GetStartAt(playableVhData, item, request.GetBaseRequest().GetServerTimeMs(), request.GetBaseRequest().GetDeviceState());
    TString payload = playableVhData.Payload;

    if (IsIn({EContentType::Movie, EContentType::TvShow, EContentType::TvShowEpisode}, playableVhData.VideoType))
    {
        payload = MakeKinopoiskPayload(startTime);
        if (playableVhData.VideoType == EContentType::TvShowEpisode) {
            auto tvShowItem = GetVideoItem();
            *directive.MutableTvShowItem() = std::move(tvShowItem);
            directive.MutableNextItem();
        }
    }

    directive.SetUri(item.GetPlayUri());
    directive.SetPayload(std::move(payload));
    directive.SetStartAt(startTime);
    directive.SetSessionToken(playableVhData.SessionId);
    SetupAudioAndSubtitles(directive, item, request.GetBaseRequest().GetDeviceState());

    *directive.MutableItem() = std::move(item);
    return directive;

}
TShowVideoDescriptionDirective TVhData::MakeShowVideoDescriptionDirective() const {
    TShowVideoDescriptionDirective directive;
    auto item = GetVideoItem();
    *directive.MutableItem() = std::move(item);
    return directive;
}


TMaybe<TVideoItemHelper> TVideoItemHelper::TryMakeFromVhPlayerResponse(const NJson::TJsonValue& frontendVhResponse) {
    return TryMakeFromVhResponse(frontendVhResponse["content"]);
}

TMaybe<TVideoItemHelper> TVideoItemHelper::TryMakeFromVhResponse(const NJson::TJsonValue& frontendVhResponse) {
    if (auto vhData = TVhData::ParseJsonDoc(frontendVhResponse)) {
        auto videoItemHelper = MakeMaybe(TVideoItemHelper());
        videoItemHelper->VhPlayerData = std::move(*vhData);
        return videoItemHelper;
    }
    return Nothing();
}

void TVideoItemHelper::AddSubtitlesAndAudioTracks(const NJson::TJsonValue& ottResponse) {
    auto& playableVhData = VhPlayerData.GetPlayableVhPlayerData();
    FillAudioStreamsAndSubtitlesInfo(playableVhData.AudioStreams,
                                     playableVhData.Subtitles,
                                     playableVhData.PlayerRestrictionConfig,
                                     ottResponse);
}

// Seasonal vh helper

TVhSeasonData::TVhSeasonData(const NJson::TJsonValue& season) {
    SeasonNumber = season["season_number"].GetUInteger();
    EpisodesCount = season["episodes_count"].GetUInteger();
    ContentId = season["content_id"].GetString();
    SeriesContentId = season["series"]["content_id"].GetString();
    FirstEpisode = season["episodes"].GetArray()[0];
}

TMaybe<TVhSeriesHelper> TVhSeriesHelper::TryMakeFromVhSeriesResponse(const NJson::TJsonValue& response) {
    if (response.IsNull() || !response.IsMap() || !response.Has("series") || !response.Has("set")) {
        return Nothing();
    }
    TVhSeriesHelper seriesHelper;
    seriesHelper.Seasons_ = response["set"].GetArray();
    seriesHelper.Series_ = response["series"];
    return seriesHelper;
}

TMaybe<TVhSeasonData> TVhSeriesHelper::GetSeasonByNumber(ui32 number) const {
    for (const auto& s : Seasons_) {
        if (s["season_number"].GetUInteger() == number) {
            return TVhSeasonData{s};
        }
    }
    return Nothing();
}

TMaybe<TVideoItem> TVhSeriesHelper::GetSeriesVideoItem() const {
    if (auto data = TVideoItemHelper::TryMakeFromVhResponse(Series_)) {
        return data->GetVideoItem();
    }
    return Nothing();
}

TMaybe<TVhEpisodeData> GetEpisodeFromVhEpisodesResponse(const NJson::TJsonValue& response, ui32 episodeNumber) {
    if (response.IsNull() || !response.IsMap() || !response.Has("set")) {
        return Nothing();
    }
    for (const auto& e : response["set"].GetArray()) {
        if (e["episode_number"].GetUInteger() == episodeNumber) {
            if (TVhEpisodeData episode(e); !episode.ContentId.Empty()) {
                return episode;
            }
        }
    }
    return Nothing();
}

} // namespace NAlice::NVideoCommon
