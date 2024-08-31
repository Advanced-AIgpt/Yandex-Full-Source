#include "vh_player.h"

#include <alice/library/json/json.h>
#include <alice/library/video_common/video_helper.h>

#include <util/string/join.h>
#include <util/string/util.h>


namespace {
    constexpr TStringBuf YATV_CHANNEL_PREFIX = "yatv";

    TString MakeEntrefByOntoId(const TString& ontoId) {
        return "entnext=" + ontoId;
    }
} //namespace

namespace NAlice::NVideoCommon {

TString TVhPlayerData::MakePayload(ui64 startTime) const {
    return IsKinopoisk() ? MakeKinopoiskPayload(startTime) : MakeVhPayload();
}

TString TVhPlayerData::MakeKinopoiskPayload(ui64 startTime) const {
    NJson::TJsonValue payload;

    NJson::TJsonValue stream;
    stream["uri"] = PlayUri;
    if (DrmConfig.Has("requestParams")) {
        payload["streams"]["drmConfig"] = DrmConfig;
        stream["drmType"] = "widevine";
        stream["drmConfig"] = DrmConfig;
    }
    stream["streamType"] = StreamType;
    payload["allStreams"].AppendValue(std::move(stream));

    if (IsTvShowEpisode()) {
        payload["episodeNumber"] = EpisodeNumber;
        payload["seasonNumber"] = SeasonNumber;
        payload["rootUuid"] = TvShowItemId;
    }
    payload["uuid"] = Uuid;
    payload["sessionId"] = SessionId;

    NJson::TJsonValue trackings;
    trackings["sid"] = SessionId;
    trackings["contentTypeId"] = OttParams["contentTypeID"];
    trackings["monetizationModel"] = OttParams["monetizationModel"];
    trackings["serviceName"] = OttParams["serviceName"];
    if (OttParams.Has("subscriptionType")) {
        trackings["subscriptionType"] = OttParams["subscriptionType"];
    }
    trackings["puid"] = OttParams["puid"];
    trackings["uuid"] = OttParams["uuid"];
    trackings["yaUID"] = OttParams["puid"];
    payload["trackings"] = std::move(trackings);

    payload["watchProgressPosition"] = startTime;

    return NAlice::JsonToString(payload);
}

NAlice::TVideoItem TVhPlayerData::MakeProtoVideoItem() const {
    NAlice::TVideoItem item;
    item.SetName(Title);
    item.SetProviderItemId(Uuid);
    item.SetType(ToString(VideoType));
    item.SetDescription(Description);
    item.SetDuration(Duration);
    item.SetProviderName(TString(PROVIDER_STRM));
    item.SetPlayUri(PlayUri);
    item.SetMinAge(RestrictionAge);
    item.SetAvailable(true);
    item.SetEntref(MakeEntrefByOntoId(OntoId));

    if (IsFilm() || IsTvShow() || IsTvShowEpisode()) {
        auto *lightItem = item.MutableProviderInfo()->Add();
        lightItem->SetType(ToString(VideoType));
        lightItem->SetProviderItemId(Uuid);
        lightItem->SetProviderName(TString(PROVIDER_KINOPOISK));
        lightItem->SetAvailable(HasActiveLicense);
        item.SetProviderName(TString(PROVIDER_KINOPOISK));
        item.SetAvailable(HasActiveLicense);
        item.SetGenre(Genre);
        item.SetDirectors(Directors);
        item.SetActors(Actors);
        item.SetRating(RatingKP);
        item.SetReleaseYear(ReleaseYear);
        item.SetCoverUrl16x9(CoverUrl16X9);
        item.SetCoverUrl2x3(CoverUrl2X3);
        item.SetThumbnailUrl16x9(ThumbnailUrl16X9);
        if (IsTvShow() || IsTvShowEpisode()) {
            item.SetEpisode(EpisodeNumber);
            item.SetSeason(SeasonNumber);
            lightItem->SetEpisode(EpisodeNumber);
            lightItem->SetSeason(SeasonNumber);
            item.SetTvShowSeasonId(TvShowSeasonId);
            lightItem->SetTvShowSeasonId(TvShowSeasonId);
            item.SetTvShowItemId(TvShowItemId);
            lightItem->SetTvShowItemId(TvShowItemId);
        }

        for (const auto& audioStream : AudioStreams) {
            *item.AddAudioStreams() = audioStream;
        }
        for (const auto& subtitle : Subtitles) {
            *item.AddSubtitles() = subtitle;
        }
        for (const auto& skippableFragment : SkippableFragments) {
            *item.AddSkippableFragments() = skippableFragment;
        }
        *item.MutablePlayerRestrictionConfig() = PlayerRestrictionConfig;
    }

    return item;
}

const TVhPlayerData& TVhPlayerData::GetPlayableVhPlayerData() const {
    if (!Includes.empty()) {
        return *Includes[0];
    } else if (ActualEpisode) {
        return *ActualEpisode;
    }
    return *this;
}

// NB: timeNow in ms (current time), returning time in seconds (start time for playing video)
ui64 TVhPlayerData::GetStartAt(ui64 timeNow) const {
    if (IsTvStream()) {
        return ChannelType.StartsWith(YATV_CHANNEL_PREFIX) ? StartPosition : 0;
    }

    if (StartTimestamp && EndTimestamp) {
        TInstant startTime = TInstant::Seconds(StartTimestamp);
        TInstant endTime = TInstant::Seconds(EndTimestamp);
        const auto now = TInstant::MilliSeconds(timeNow);
        if (startTime <= now && now < endTime) {
            return now.Seconds();
        }
    }
    if (StartAt && Duration) {
        return CalculateStartAt(Duration, StartAt);
    }

    return 0;
}

bool TVhPlayerData::ParseJsonDoc(const NJson::TJsonValue& doc, bool isParentChannel) {
    if (doc.IsNull() || !doc.IsMap()) {
        return false;
    }

    ParseCommonData(doc, isParentChannel);
    if (Uuid.empty()) {
        return false;
    }

    ParseStreams(doc);
    if (IsKinopoisk()) {
        ParseFilmOrSerialData(doc);
    }
    ParseLicense(doc);

    if (const auto &includes = doc["includes"]; includes.IsArray()) {
        for (const auto &include : includes.GetArray()) {
            TVhPlayerData childData;
            if (childData.ParseJsonDoc(include)) {
                Includes.emplace_back(std::make_unique<TVhPlayerData>(std::move(childData)));
            }
        }
    }

    if (IsTvStream()) {
        ParseChannelData(doc);
    }

    return true;
}

void TVhPlayerData::ParseCommonData(const NJson::TJsonValue& doc, bool isParentChannel) {
    Title = doc["title"].GetString();
    Uuid = doc["content_id"].GetString();
    RestrictionAge = doc["restriction_age"].GetUInteger();

    Description = doc["description"].GetString();
    Duration = doc["duration"].GetInteger();
    StartAt = doc["start_at"].GetInteger();

    StartTimestamp = doc["start_time"].GetInteger();
    EndTimestamp = doc["end_time"].GetInteger();
    OntoId = doc["onto_id"].GetString();

    const auto &ontoType = doc["onto_otype"].GetString();
    const auto &contentTypeName = doc["content_type_name"].GetString();
    if (ontoType == "Film/Film") {
        VideoType = EContentType::Movie;
    } else if (ontoType == "Film/Series@on" || ontoType == "Film/Season@on" /* ALICEASSESSORS-5089 */) {
        if (doc["type"].GetString() == "series") {
            VideoType = EContentType::TvShow;
        } else {
            VideoType = EContentType::TvShowEpisode;
        }
    } else if (contentTypeName == "channel" || isParentChannel) {
        VideoType = EContentType::TvStream;
    } else {
        VideoType = EContentType::Video;
    }
    SkippableFragments = ParseSkippableFragments(doc["skippableFragments"]);
}

void TVhPlayerData::ParseStreams(const NJson::TJsonValue& doc) {
    const auto& streams = doc["streams"];
    if (!streams.IsArray() || !streams.Has(0)) {
        return;
    }
    const auto& firstStream = streams[0];
    const auto *playUriPtr = firstStream.GetValueByPath("url");
    if (!playUriPtr) {
        return;
    }
    PlayUri = playUriPtr->GetString();
    if (const auto *streamTypePtr = firstStream.GetValueByPath("stream_type")) {
        StreamType = streamTypePtr->GetString();
    }

    NJson::TJsonValue payloadJson;
    payloadJson["streams"] = streams;
    Payload = payloadJson.GetStringRobust();

    if (const auto *sessionIdPointer = firstStream.GetValueByPath("drmConfig.requestParams.watchSessionId")) {
        SessionId = sessionIdPointer->GetString();
    }
    if (const auto *drmConfigPointer = firstStream.GetValueByPath("drmConfig")) {
        DrmConfig = *drmConfigPointer;
    }
    ParseAudioStreamsAndSubtitlesInfo(AudioStreams, Subtitles, firstStream);
    ParsePlayerRestrictionConfig(PlayerRestrictionConfig, doc);
}

void TVhPlayerData::ParseFilmOrSerialData(const NJson::TJsonValue& doc) {
    CoverUrl16X9 = NAlice::NVideoCommon::BuildResizedThumbnailUri(doc["thumbnail"].GetString(), "1920x1080");
    CoverUrl2X3 = NAlice::NVideoCommon::BuildResizedThumbnailUri(doc["onto_poster"].GetString(), "320x480");
    ThumbnailUrl16X9 = NAlice::NVideoCommon::BuildThumbnailUri(doc["thumbnail"].GetString());

    ReleaseYear = doc["release_year"].GetUInteger();
    RatingKP = doc["rating_kp"].GetDouble();
    Directors = doc["directors"].GetString();
    Actors = doc["actors"].GetString();

    if (const auto &genres = doc["genres"]; genres.IsArray()) {
        Genre = JoinSeq(", ", genres.GetArray());
        RemoveAll(Genre, '"');
    }
    if (const auto &seasonInfo = doc["season"]; seasonInfo.IsMap()) {
        SeasonNumber = seasonInfo["season_number"].GetUInteger();
    }
    EpisodeNumber = doc["episode_number"].GetUInteger();
    if (const auto *seriesIdPtr = doc.GetValueByPath("series.id")) {
        TvShowItemId = seriesIdPtr->GetString();
    }
    TvShowSeasonId = doc["parent_id"].GetString();
    OttParams = doc["ottParams"];
}

void TVhPlayerData::ParseLicense(const NJson::TJsonValue& doc) {
    if (const auto& ottParams = doc["ottParams"]; ottParams.IsMap()) {
        IsPaidContent = ottParams["monetizationModel"].GetString() != "AVOD";
        if (const auto& licenses = ottParams["licenses"]; licenses.IsArray()) {
            HasActiveLicense =
                    AnyOf(licenses.GetArray(), [](const auto &license) { return license["active"].GetBoolean(); });
            for (const auto& license : licenses.GetArray()) {
                if (license["monetizationModel"].GetString() == "SVOD") {
                    PurchaseTag = license["purchaseTag"].GetString();
                }
            }
        }
    }
}

void TVhPlayerData::ParseChannelData(const NJson::TJsonValue& doc) {
    ChannelType = doc["channel_type"].GetString();
    StartPosition = doc["start_position"].GetInteger();
    TVhPlayerData actualEpisodeData;
    if (actualEpisodeData.ParseJsonDoc(doc["actual_episode"], /*isParentChannel*/ true)) {
        ActualEpisode = std::make_unique<TVhPlayerData>(std::move(actualEpisodeData));
    }
}

}   // namespace NAlice::NVideoCommon
