#include "universal_api_utils.h"

#include <util/datetime/base.h>
#include <alice/bass/libs/logging_v2/logger.h>
#include <alice/bass/libs/source_request/handle.h>
#include <alice/bass/libs/video_common/utils.h>

#include <library/cpp/scheme/scheme.h>

#include <util/string/split.h>

using namespace NBASS;
using namespace NHttpFetcher;

namespace NVideoCommon {

namespace {

constexpr TStringBuf ANTHOLOGY_MOVIE_TYPE = "anthology_movie";
constexpr TStringBuf ANTHOLOGY_MOVIE_EPISODE_TYPE = "anthology_movie_episode";
constexpr TStringBuf TV_SHOW_SEASON = "tv_show_season";

const TString TV_SHOW_TYPE = ToString(EItemType::TvShow);
const TString TV_SHOW_EPISODE_TYPE = ToString(EItemType::TvShowEpisode);
const TString MOVIE_TYPE = ToString(EItemType::Movie);

// NOTE: UAPI 'content_type' field is mapped into 'EItemType' BASS type.
TMaybe<TString> GetContentType(TStringBuf contentType) {
    if (contentType == MOVIE_TYPE || contentType == TV_SHOW_TYPE || contentType == TV_SHOW_EPISODE_TYPE)
        return TString{contentType};

    if (contentType == TV_SHOW_SEASON)
        return TString{contentType};

    // FIXME (a-sidorin@): handle anthology_movie and anthology_movie_episode.
    if (contentType == ANTHOLOGY_MOVIE_TYPE)
        return Nothing(); // FIXME

    if (contentType == ANTHOLOGY_MOVIE_EPISODE_TYPE)
        return Nothing(); // FIXME

    return Nothing();
}

struct TFullId {
    TString ContentType;
    TString ProviderItemId;
};

TMaybe<TFullId> TryExpandFullId(TStringBuf fullId) {
    TStringBuf left, id;
    fullId.RSplit(':', left, id);
    TStringBuf parent, type;
    left.RSplit(':', parent, type);
    auto contentType = GetContentType(type);
    if (contentType && !id.empty())
        return TFullId{*contentType, TString{id}};
    return Nothing();
}

TString CreateItemId(TStringBuf itemType, TStringBuf providerItemId) {
    return TStringBuilder() << itemType << ':' << providerItemId;
}

TString CreateItemId(const TLightVideoItemConstScheme& item) {
    // FIXME (a-sidorin@): handle anthology_movie and anthology_movie_episode.
    return CreateItemId(item->Type(), item->ProviderItemId());
}

template <typename TData>
TData CreateWithItemId(TStringBuf itemId) {
    TData data;
    data.ItemId = itemId;
    return data;
}

bool IsAlreadyPublished(TInstant currentDateTime, TInstant publicationDate) {
    return currentDateTime >= publicationDate;
}

class TAllItemsRequestHandle : public NBASS::IRequestHandle<TUAPIFetchResult> {
public:
    TAllItemsRequestHandle(const IUAPIParseHelper& parseHelper, TRequest& request,
                           const TMaybe<TInstant>& previousUpdateTime)
        : Path(request.Url())
        , ParseHelper(parseHelper)
        , PreviousUpdateTime(previousUpdateTime)
    {
        RequestHandle = request.Fetch();
    }

    TResultValue WaitAndParseResponse(TUAPIFetchResult* response) override {
        auto httpResponse = RequestHandle->Wait();
        if (httpResponse->IsError())
            return MakeVideoError("Cannot fetch ", Path.Quote(), ", reason: ", httpResponse->GetErrorText().Quote());

        NSc::TValue responseValue;
        if (!NSc::TValue::FromJson(responseValue, httpResponse->Data)) {
            LOG(ERR) << "Bad root item list: " << httpResponse->Data << Endl;
            return MakeVideoError("Cannot parse response data for root content!");
        }

        TMaybe<TInstant> parsedUpdateTime = ParseHelper.ParseUpdateTime(responseValue);
        bool isDataUpdated = !(PreviousUpdateTime && parsedUpdateTime && *PreviousUpdateTime >= *parsedUpdateTime);

        response->UpdateTime = parsedUpdateTime;
        response->HasUpdates = isDataUpdated;
        if (!isDataUpdated)
            return ResultSuccess();

        return ParseHelper.ParseItemList(response, responseValue);
    }

private:
    TString Path;
    const IUAPIParseHelper& ParseHelper;
    THandle::TRef RequestHandle;
    TMaybe<TInstant> PreviousUpdateTime;
};

TString JoinValueArray(const NSc::TValue& value, TStringBuf key) {
    const auto& array = value.TrySelect(key).GetArray();
    return JoinStringArray(array);
}

TMaybe<TInstant> TryParseTimeField(const NSc::TValue& responseData, TStringBuf keyPath) {
    TStringBuf updateTimeStr = responseData.TrySelect(keyPath).GetString();
    if (updateTimeStr.empty())
        return Nothing();

    TInstant updateTime;
    if (TInstant::TryParseIso8601(updateTimeStr, updateTime))
        return updateTime;

    LOG(ERR) << "Cannot parse update time: '" << updateTimeStr << '\'' << Endl;
    return Nothing();
}

// transform titles:
// [serial_name - Серия Y] -> [Y серия]
// [serial_name - Серия Y - episode_name] -> [Y. episode_name]
// [serial_name - Сезон X - Серия Y] -> [Y серия]
// [serial_name - Сезон X - Серия Y - episode_name] -> [Y. episode_name]
// all the rest is unchanged

TString TransformEpisodeTitle(const TString& title) {
    constexpr TStringBuf episodePrefix = "Серия ";
    constexpr TStringBuf del = " - ";
    TVector<TStringBuf> parts = StringSplitter(title).SplitByString(del);

    if (parts.size() >= 2) {
        TStringBuf episode;
        if (parts.back().AfterPrefix(episodePrefix, episode)) {
            return TString::Join(episode, " серия");
        } else if (parts[parts.size() - 2].AfterPrefix(episodePrefix, episode)) {
            return TString::Join(episode, ". ", parts.back());
        }
    }
    return title;
}

TResultValue ParseVideoItemCommon(const TString& itemId, const NSc::TValue& responseValue,
                                  const IUAPIParseHelper& parseHelper, TVideoItem& item, TInstant currentDateTime) {
    TString sourceType = responseValue["content_type"].ForceString();
    auto type = GetContentType(sourceType);
    if (!type) {
        LOG(ERR) << "Bad provider response for itemId " << itemId << Endl;
        LOG(ERR) << "Response: " << responseValue << Endl;
        return MakeVideoError("Unsupported item type: ", sourceType, " for itemId ", itemId.Quote());
    }

    auto providerItemId = parseHelper.GetProviderItemId(itemId);
    if (!providerItemId)
        return MakeVideoError("Incorrect item type: ", itemId);

    item->Type() = *type;
    item->ProviderName() = parseHelper.GetProviderName();

    TString hrid = parseHelper.GetHumanReadableId(itemId, responseValue);

    item->ProviderItemId() = *providerItemId;
    item->HumanReadableId() = hrid;

    item->Name() = TransformEpisodeTitle(responseValue["title"].ForceString());
    item->Description() = responseValue["description"].ForceString();

    if (TString genre = JoinValueArray(responseValue, "genres"))
        item->Genre() = genre;

    if (responseValue.Has("rating"))
        item->Rating() = responseValue["rating"].ForceNumber();

    if (responseValue.Has("duration"))
        item->Duration() = responseValue["duration"].ForceIntNumber();

    if (responseValue.Has("release_year"))
        item->ReleaseYear() = responseValue["release_year"].GetIntNumber();

    if (const auto publicationDate = TryParseTimeField(responseValue, "publication_date")) {
        bool isPublished = IsAlreadyPublished(currentDateTime, *publicationDate);
        item->Soon() = !isPublished;
        if (!isPublished) {
            item->UpdateAtUs() = publicationDate->MicroSeconds();
            LOG(DEBUG) << "Item " << itemId.Quote() << " will be published in the future at "
                       << publicationDate->ToString() << Endl;
        }
    }

    if (TString directors = JoinValueArray(responseValue, "directors"))
        item->Directors() = directors;

    if (TString actors = JoinValueArray(responseValue, "actors"))
        item->Actors() = actors;

    if (responseValue.Has("min_age")) {
        const auto& minAge = responseValue["min_age"].ForceIntNumber();
        item->MinAge() = minAge;
        item->AgeLimit() = ToString(minAge);
    }

    if (responseValue.Has("cover_url_2x3"))
        item->CoverUrl2X3() = responseValue["cover_url_2x3"].GetString();

    if (responseValue.Has("cover_url_16x9"))
        item->CoverUrl16X9() = responseValue["cover_url_16x9"].GetString();

    if (responseValue.Has("thumbnail_url_2x3_small"))
        item->ThumbnailUrl2X3Small() = responseValue["thumbnail_url_2x3_small"].GetString();

    if (responseValue.Has("thumbnail_url_16x9"))
        item->ThumbnailUrl16X9() = responseValue["thumbnail_url_16x9"].GetString();

    if (responseValue.Has("thumbnail_url_16x9_small"))
        item->ThumbnailUrl16X9Small() = responseValue["thumbnail_url_16x9_small"].GetString();

    if (responseValue.Has("misc_ids")) {
        auto& miscIds = responseValue["misc_ids"];
        if (miscIds.Has("kinopoisk") && miscIds["kinopoisk"].IsString()) {
            TStringBuf id = miscIds["kinopoisk"].GetString();
            item->MiscIds().Kinopoisk() = id;
            item->DebugInfo().WebPageUrl() = TStringBuilder() << "http://www.kinopoisk.ru/film/" << id;
        }

        if (miscIds.Has("imdb") && miscIds["imdb"].IsString())
            item->MiscIds().Imdb() = miscIds["imdb"].GetString();

        if (miscIds.Has("kinopoisk_uuid") && miscIds["kinopoisk_uuid"].IsString())
            item->MiscIds().KinopoiskUuid() = miscIds["kinopoisk_uuid"].GetString();
    }

    if (responseValue.Has("sequence_number"))
        item->ProviderNumber() = responseValue["sequence_number"].ForceIntNumber();

    return ResultSuccess();
}

class TMovieRequestHandle : public IVideoRequestHandle {
public:
    TMovieRequestHandle(TRequest& request, const IUAPIParseHelper& parseHelper, TInstant currentDateTime)
        : ParseHelper(parseHelper)
        , CurrentDateTime(currentDateTime)
    {
        RequestHandle = request.Fetch();
    }

    TResultValue WaitAndParseResponse(TVideoItemUAPIData* response) override {
        const TString& itemId = response->ItemId;

        response->IsValid = false;
        auto httpResponse = RequestHandle->Wait();
        if (!httpResponse) {
            return MakeVideoError("Cannot fetch data for item ", itemId.Quote(), ", reason: ",
                                  httpResponse->GetErrorText().Quote());
        }

        NSc::TValue responseValue;
        if (!NSc::TValue::FromJson(responseValue, httpResponse->Data)) {
            LOG(ERR) << "Bad movie item response: " << httpResponse->Data << Endl;
            return MakeVideoError("Cannot parse response data for ", itemId.Quote());
        }

        TVideoItem item;

        if (const auto error = ParseVideoItemCommon(itemId, responseValue, ParseHelper, item, CurrentDateTime))
            return error;

        if (item->ProviderItemId()->empty() && item->HumanReadableId()->empty())
            return MakeVideoError("Cannot parse id data for item ", itemId.Quote());

        response->VideoItem = std::move(item);
        response->IsValid = true;
        return ResultSuccess();
    }

private:
    THandle::TRef RequestHandle;
    const IUAPIParseHelper& ParseHelper;
    const TInstant CurrentDateTime;
};

TSeasonDescriptorUAPIData CreateSingleSeasonDescriptor(const TVideoItem& parentTvShow) {
    TSeasonDescriptorUAPIData result;
    // result.ItemId is left empty by default.
    result.SeasonDescriptor.Index = 0;
    result.SeasonDescriptor.ProviderNumber = 1;
    result.SeasonDescriptor.SerialId = parentTvShow->ProviderItemId();
    result.ItemId = parentTvShow->ProviderItemId();
    return result;
}

class TTvShowRequestHandle : public ITvShowRequestHandle {
public:
    TTvShowRequestHandle(TRequest& request, const IUAPIParseHelper& parseHelper, const TString& itemId,
                         TInstant currentDateTime)
        : Parser(parseHelper)
        , ItemId(itemId)
        , CurrentDateTime(currentDateTime)
    {
        RequestHandle = request.Fetch();
    }

    TResultValue WaitAndParseResponse(TTvShowUAPIData* response) override {
        auto httpResponse = RequestHandle->Wait();
        if (!httpResponse) {
            return MakeVideoError("Cannot fetch data for item ", response->TvShowItem.ItemId.Quote(), ", reason: '",
                                  httpResponse->GetErrorText().Quote());
        }

        const TString& tvShowId = response->TvShowItem.ItemId;
        NSc::TValue responseValue;
        if (!NSc::TValue::FromJson(responseValue, httpResponse->Data)) {
            LOG(ERR) << "Bad tv show item response: " << httpResponse->Data << Endl;
            return MakeVideoError("Cannot parse response data for ", tvShowId.Quote());
        }

        TVideoItem tvShowItem;

        if (const auto error = ParseVideoItemCommon(ItemId, responseValue, Parser, tvShowItem, CurrentDateTime))
            return error;

        TSerialDescriptor serialDescriptor;
        serialDescriptor.Id = tvShowItem->ProviderItemId();
        if (tvShowItem->HasMinAge())
            serialDescriptor.MinAge = tvShowItem->MinAge();

        TVector<TSeasonDescriptorUAPIData> seasonDescriptors;
        TMaybe<TSeasonDescriptorUAPIData> singleSeason;

        const NSc::TArray& children = responseValue["children"].GetArray();
        for (const auto& child : children) {
            TStringBuf childId = child.GetString();
            if (!childId) {
                LOG(ERR) << "Empty child item of item " << ItemId.Quote() << ", skipping" << Endl;
                continue;
            }

            auto fullId = TryExpandFullId(childId);
            if (!fullId) {
                LOG(ERR) << "Cannot extract child content info for child id " << TString{childId}.Quote() << " of item "
                         << tvShowId.Quote() << Endl;
                continue;
            }

            if (fullId->ContentType == TV_SHOW_SEASON) {
                auto seasonDescr = CreateWithItemId<TSeasonDescriptorUAPIData>(childId);
                seasonDescr.SeasonDescriptor.Id = fullId->ProviderItemId;
                seasonDescr.SeasonDescriptor.SerialId = tvShowItem->ProviderItemId();
                seasonDescriptors.push_back(std::move(seasonDescr));

            } else if (fullId->ContentType == TV_SHOW_EPISODE_TYPE) {
                if (!singleSeason)
                    singleSeason = CreateSingleSeasonDescriptor(tvShowItem);
                singleSeason->Episodes.push_back(CreateWithItemId<TVideoItemUAPIData>(childId));
            }
        }

        if (singleSeason && !seasonDescriptors.empty())
            return MakeVideoError("Tv show ", ItemId.Quote(), " has mixed episodes and seasons as parent");

        response->TvShowItem = {ItemId, std::move(tvShowItem)};
        response->SerialDescriptor.SerialDescriptor = std::move(serialDescriptor);
        if (singleSeason) {
            response->HasSingleSeason = true;
            response->SerialDescriptor.Seasons.emplace_back(std::move(*singleSeason));
        } else {
            response->SerialDescriptor.Seasons = std::move(seasonDescriptors);
        }
        return ResultSuccess();
    }

private:
    THandle::TRef RequestHandle;
    const IUAPIParseHelper& Parser;
    const TString& ItemId;
    const TInstant CurrentDateTime;
};

class TSeasonRequestHandle : public ISeasonRequestHandle {
public:
    TSeasonRequestHandle(const TString& serialId, TInstant currentDateTime, TRequest& request)
        : SerialId(serialId)
        , CurrentDateTime(currentDateTime)
    {
        RequestHandle = request.Fetch();
    }

    TResultValue WaitAndParseResponse(TSeasonDescriptorUAPIData* response) override {
        auto httpResponse = RequestHandle->Wait();
        if (!httpResponse) {
            return MakeVideoError("Cannot fetch data for season ", response->ItemId.Quote(), ", reason: '",
                                    httpResponse->GetErrorText().Quote());
        }

        NSc::TValue responseValue;
        if (!NSc::TValue::FromJson(responseValue, httpResponse->Data)) {
            LOG(ERR) << "Bad season response: " << httpResponse->Data << Endl;
            return MakeVideoError("Cannot parse response data for season ", response->ItemId.Quote());
        }

        // Sanity checks.
        if (!responseValue.Has("content_type") || responseValue["content_type"] != TV_SHOW_SEASON) {
            LOG(ERR) << "Bad season response: " << httpResponse->Data << Endl;
            return MakeVideoError("Season item ", response->ItemId.Quote(),
                                  " doesn't have 'content_type' field set properly");
        }

        auto fullId = TryExpandFullId(response->ItemId);
        if (!fullId)
            return MakeVideoError("Incorrect UAPI item id for season: ", response->ItemId);

        auto& seasonDescr = response->SeasonDescriptor;
        seasonDescr.Id = fullId->ProviderItemId;
        seasonDescr.SerialId = SerialId;
        seasonDescr.ProviderNumber = responseValue["sequence_number"].ForceIntNumber();

        if (const auto publicationDate = TryParseTimeField(responseValue, "publication_date")) {
            bool isPublished = IsAlreadyPublished(CurrentDateTime, *publicationDate);
            seasonDescr.Soon = !isPublished;
            if (!isPublished)
                seasonDescr.UpdateAt = std::move(publicationDate);
        }

        THashSet<TString> uniqueSeasonIds;
        const auto& children = responseValue["children"].GetArray();
        for (const auto& child : children) {
            TString fullIdStr = TString{child.GetString()};
            auto fullId = TryExpandFullId(fullIdStr);
            if (!fullId) {
                LOG(ERR) << "Cannot parse child itemId " << fullIdStr.Quote() << " of season "
                         << response->ItemId.Quote() << Endl;
                continue;
            }

            if (fullId->ContentType != TV_SHOW_EPISODE_TYPE && fullId->ContentType != ANTHOLOGY_MOVIE_EPISODE_TYPE) {
                LOG(WARNING) << "Found non-episode child itemId " << fullIdStr.Quote() << " of season "
                             << response->ItemId.Quote() << Endl;
                continue;
            }

            auto [iter, wasAdded] = uniqueSeasonIds.insert(fullIdStr);
            if (!wasAdded) {
                LOG(WARNING) << "Found non-unique episode itemId " << fullIdStr.Quote() << " of season "
                             << response->ItemId.Quote() << Endl;
                continue;
            }

            response->Episodes.push_back(CreateWithItemId<TVideoItemUAPIData>(fullIdStr));
        }

        if (response->Episodes.empty() && !seasonDescr.Soon)
            return MakeVideoError("No episodes in season ", response->ItemId.Quote(), " have been retrieved");

        response->Build();
        return ResultSuccess();
    }

private:
    const TString SerialId;
    THandle::TRef RequestHandle;
    const TInstant CurrentDateTime;
};

class TTvShowEpisodeRequestHandle : public IVideoRequestHandle {
public:
    TTvShowEpisodeRequestHandle(const TSeasonDescriptor& parentSeason, TRequest& request,
                                const IUAPIParseHelper& parseHelper, TInstant currentDateTime)
        : ParentSeason(parentSeason)
        , ParseHelper(parseHelper)
        , CurrentDateTime(currentDateTime)
    {
        RequestHandle = request.Fetch();
    }

    TResultValue WaitAndParseResponse(TVideoItemUAPIData* response) override {
        const auto& itemId = response->ItemId;

        response->IsValid = false;
        auto httpResponse = RequestHandle->Wait();
        if (!httpResponse) {
            return MakeVideoError("Cannot fetch data for item ", itemId.Quote(), ", reason: '",
                                  httpResponse->GetErrorText().Quote());
        }

        NSc::TValue responseValue;
        if (!NSc::TValue::FromJson(responseValue, httpResponse->Data)) {
            LOG(ERR) << "Bad episode response: " << httpResponse->Data << Endl;
            return MakeVideoError("Cannot parse response data for ", itemId.Quote());
        }

        TVideoItem item;

        if (const auto error = ParseVideoItemCommon(itemId, responseValue, ParseHelper, item, CurrentDateTime))
            return error; // Logging has already been done.

        if (item->ProviderItemId()->empty())
            return MakeVideoError("Cannot parse id data for item ", itemId.Quote());

        if (const auto& seasonId = ParentSeason.Id)
            item->TvShowSeasonId() = *seasonId;

        item->TvShowItemId() = ParentSeason.SerialId;
        response->VideoItem = std::move(item);
        response->IsValid = true;
        return ResultSuccess();
    }

private:
    const TSeasonDescriptor& ParentSeason;
    THandle::TRef RequestHandle;
    const IUAPIParseHelper& ParseHelper;
    const TInstant CurrentDateTime;
};

class TSingleSeasonEpisodeHandle : public ISeasonRequestHandle {
public:
    TSingleSeasonEpisodeHandle(IUAPIVideoProvider::TPtr videoProvider, TSeasonDescriptorUAPIData& seasonData,
                               IMultiRequest::TRef multiRequest, TInstant currentDateTime,
                               TMaybe<TTimePoint>& lastDownload, size_t maxRPS = SIZE_MAX)
        : VideoProvider(videoProvider)
        , EpisodeRequests(Reserve(seasonData.Episodes.size()))
        , MultiRequest(multiRequest)
        , CurrentDateTime(currentDateTime)
        , LastDownload(lastDownload)
        , MaxRPS(maxRPS)
        , IsSeasonValid(seasonData.IsValid)
    {
        Y_ENSURE(VideoProvider, "videoProvider should not be null!");
        if (IsSeasonValid)
            FetchWithRPS(seasonData, 0 /* startIndex */);
    }

    TResultValue WaitAndParseResponse(TSeasonDescriptorUAPIData* response) override {
        if (!IsSeasonValid)
            return ResultSuccess();

        bool hasValidEpisodes = WaitAndParseResponse(*response, 0 /* startIndex */);
        size_t fetchIndex = EpisodeRequests.size();

        while (fetchIndex < response->Episodes.size()) {
            size_t fetched = FetchWithRPS(*response, fetchIndex);
            if (const bool hasFetchedEpisodes = WaitAndParseResponse(*response, fetchIndex))
                hasValidEpisodes = true;
            fetchIndex += fetched;
        }

        if (!hasValidEpisodes) {
            response->IsValid = false;
            return MakeVideoError("No episodes for tv show season ", response->ItemId.Quote(),
                                  " have been resolved successfully");
        }

        response->Build();
        return ResultSuccess();
    }

private:
    size_t FetchWithRPS(TSeasonDescriptorUAPIData& seasonData, size_t startIndex) {
        Y_ASSERT(startIndex < seasonData.Episodes.size());
        size_t fetchCount = Min(seasonData.Episodes.size() - startIndex, MaxRPS);

        WaitForNextDownload(LastDownload);

        for (size_t requestIndex = 0; requestIndex < fetchCount; ++requestIndex) {
            auto request = VideoProvider->GetUAPIRequestProvider().CreateTvShowEpisodeRequest(
                MultiRequest, seasonData.SeasonDescriptor, seasonData.Episodes[startIndex + requestIndex].ItemId,
                VideoProvider->GetParseHelper(), CurrentDateTime);
            EpisodeRequests.push_back(std::move(request));
        }
        return fetchCount;
    }

    bool WaitAndParseResponse(TSeasonDescriptorUAPIData& seasonData, size_t startIndex) {
        bool hasValidEpisodes = false;

        for (size_t requestIndex = startIndex; requestIndex < EpisodeRequests.size(); ++requestIndex) {
            TVideoRequestHolder& handle = EpisodeRequests[requestIndex];
            auto& episodeData = seasonData.Episodes[requestIndex];
            if (const auto error = handle->WaitAndParseResponse(&episodeData)) {
                LOG(ERR) << "UAPI single season episode request error: " << error->Msg << Endl;
                episodeData.IsValid = false;
            } else {
                hasValidEpisodes = true;
            }
        }
        return hasValidEpisodes;
    }

    IUAPIVideoProvider::TPtr VideoProvider;
    TVector<TVideoRequestHolder> EpisodeRequests;
    IMultiRequest::TRef MultiRequest;
    const TInstant CurrentDateTime;
    TMaybe<TTimePoint>& LastDownload;
    const size_t MaxRPS;
    const bool IsSeasonValid;
};

class TAllTvShowSeasonsHandle : public NBASS::IRequestHandle<TTvShowUAPIData> {
public:
    TAllTvShowSeasonsHandle(IUAPIVideoProvider::TPtr provider, TTvShowUAPIData& tvShowData, TInstant currentDateTime,
                            IMultiRequest::TRef multiRequest)
        : Provider(provider)
        , CurrentDateTime(currentDateTime)
        , IsValid(tvShowData.IsValid)
    {
        if (!IsValid || tvShowData.HasSingleSeason)
            return;

        const auto& serialDescr = tvShowData.SerialDescriptor;
        SeasonRequests.reserve(serialDescr.Seasons.size());
        for (const TSeasonDescriptorUAPIData& seasonData : serialDescr.Seasons) {
            SeasonRequests.push_back(Provider->GetUAPIRequestProvider().CreateTvShowSeasonRequest(
                multiRequest, serialDescr.SerialDescriptor, seasonData.ItemId, CurrentDateTime));
        }
    }

    TResultValue WaitAndParseResponse(TTvShowUAPIData* response) override {
        if (!IsValid || response->HasSingleSeason)
            return ResultSuccess();

        Y_ASSERT(response->SerialDescriptor.Seasons.size() == SeasonRequests.size());

        bool hasValidSeasons = false;
        for (size_t seasonIndex = 0; seasonIndex < SeasonRequests.size(); ++seasonIndex) {
            auto& handle = SeasonRequests[seasonIndex];
            auto& seasonData = response->SerialDescriptor.Seasons[seasonIndex];
            if (const auto error = handle->WaitAndParseResponse(&seasonData)) {
                LOG(ERR) << "UAPI season retrieving error: " << error->Msg << Endl;
                seasonData.IsValid = false;
            } else {
                hasValidSeasons = true;
            }
        }

        if (hasValidSeasons)
            return ResultSuccess();

        response->IsValid = false;
        return MakeVideoError("No seasons for tv show ", response->TvShowItem.ItemId.Quote(),
                              " have been resolved successfully");
    }

private:
    TVector<TSeasonRequestHolder> SeasonRequests;
    IUAPIVideoProvider::TPtr Provider;
    const TInstant CurrentDateTime;
    const bool IsValid;
};

// ------------------- TUAPIContentInfoProvider -----------------------------------------------------------

class TUAPIVideoItemContentInfoRequest : public IVideoItemHandle {
public:
    TUAPIVideoItemContentInfoRequest(IUAPIVideoProvider::TPtr videoProvider, TLightVideoItemConstScheme item,
                                     IMultiRequest::TRef multiRequest, TInstant currentDateTime)
        : ItemId(CreateItemId(item))
        , VideoProvider(videoProvider)
        , UAPIRequest(VideoProvider->GetUAPIRequestProvider().CreateVideoItemRequest(
              multiRequest, VideoProvider->GetParseHelper(), ItemId, currentDateTime))
    {
        Y_ENSURE(VideoProvider, "videoProvider should not be null!");
    }

    TResult WaitAndParseResponse(TVideoItem& request) override {
        auto videoItemData = CreateWithItemId<TVideoItemUAPIData>(ItemId);
        if (const auto error = UAPIRequest->WaitAndParseResponse(&videoItemData))
            return TError{error->Msg};

        request = std::move(videoItemData.VideoItem);

        return Nothing();
    }

private:
    TString ItemId;
    IUAPIVideoProvider::TPtr VideoProvider;
    TVideoRequestHolder UAPIRequest;
};

class TUAPITvShowContentInfoRequest : public IVideoItemHandle {
public:
    TUAPITvShowContentInfoRequest(IUAPIVideoProvider::TPtr videoProvider, TLightVideoItemConstScheme item,
                                  IMultiRequest::TRef multiRequest, TInstant currentDateTime)
        : ItemId(CreateItemId(item))
        , VideoProvider(videoProvider)
        , UAPIRequest(VideoProvider->GetUAPIRequestProvider().CreateTvShowRequest(
              multiRequest, VideoProvider->GetParseHelper(), ItemId, currentDateTime))
    {
        Y_ENSURE(VideoProvider, "videoProvider should not be null!");
    }

    TResult WaitAndParseResponse(TVideoItem& request) override {
        TTvShowUAPIData tvShowData;
        tvShowData.TvShowItem.ItemId = ItemId;
        if (const auto error = UAPIRequest->WaitAndParseResponse(&tvShowData)) {
            return TError{error->Msg};
        }

        tvShowData.Build();
        request = std::move(tvShowData.TvShowItem.VideoItem);

        return Nothing();
    }

private:
    const TString ItemId;
    IUAPIVideoProvider::TPtr VideoProvider;
    TTvShowRequestHolder UAPIRequest;
};

class TAllTvShowEpisodesHandle : public ITvShowRequestHandle {
public:
    TAllTvShowEpisodesHandle(IRequestProvider::TPtr requestProvider, IUAPIVideoProvider::TPtr videoProvider,
                             TInstant currentDateTime, size_t maxRPS)
        : RequestProvider(requestProvider)
        , VideoProvider(videoProvider)
        , CurrentDateTime(currentDateTime)
        , MaxRPS(maxRPS)
    {
    }

    TResultValue WaitAndParseResponse(TTvShowUAPIData* response) override {
        if (!response->IsValid)
            return ResultSuccess();

        auto& seasons = response->SerialDescriptor.Seasons;
        bool hasValidSeasons = false;
        for (auto& seasonData : seasons) {
            // NOTE: Seasons are not queried in parallel because they have underlying episode handles downloaded
            // in parallel.
            IMultiRequest::TRef multiRequest = RequestProvider->CreateMultiRequest();
            TSingleSeasonEpisodeHandle seasonHandle{VideoProvider,   seasonData,   multiRequest,
                                                    CurrentDateTime, LastDownload, MaxRPS};
            if (const auto error = seasonHandle.WaitAndParseResponse(&seasonData))
                seasonData.IsValid = false;
            else
                hasValidSeasons = true;
        }

        if (!hasValidSeasons)
            return MakeVideoError("No episodes for tv show ", response->TvShowItem.ItemId.Quote(),
                                  " have been resolved successfully");

        response->SerialDescriptor.Build();
        return ResultSuccess();
    }

private:
    IRequestProvider::TPtr RequestProvider;
    IUAPIVideoProvider::TPtr VideoProvider;
    const TInstant CurrentDateTime;
    TMaybe<TTimePoint> LastDownload;
    size_t MaxRPS;
};

class TUAPISerialDescriptorContentInfoRequest : public ISerialDescriptorHandle {
public:
    TUAPISerialDescriptorContentInfoRequest(IRequestProvider::TPtr requestProvider,
                                            IUAPIVideoProvider::TPtr videoProvider, TLightVideoItemConstScheme item,
                                            TInstant currentDateTime, IMultiRequest::TRef multiRequest, size_t maxRPS)
        : ItemId(CreateItemId(item))
        , RequestProvider(requestProvider)
        , VideoProvider(videoProvider)
        , CurrentDateTime(currentDateTime)
        , MaxRPS(maxRPS)
        , UAPIRequest(VideoProvider->GetUAPIRequestProvider().CreateTvShowRequest(
              multiRequest, VideoProvider->GetParseHelper(), ItemId, CurrentDateTime))
    {
    }

    TResult WaitAndParseResponse(TSerialDescriptor& request) override {
        // In UAPI, serial descriptors are being formed while parsing tv show data.
        TTvShowUAPIData tvShowData;
        tvShowData.TvShowItem.ItemId = ItemId;
        if (const auto error = UAPIRequest->WaitAndParseResponse(&tvShowData))
            return TError{error->Msg};

        IMultiRequest::TRef multiRequest = RequestProvider->CreateMultiRequest();
        TAllTvShowSeasonsHandle seasons{VideoProvider, tvShowData, CurrentDateTime, multiRequest};
        if (const auto error = seasons.WaitAndParseResponse(&tvShowData))
            return TError{error->Msg};

        TAllTvShowEpisodesHandle allEpisodes{RequestProvider, VideoProvider, CurrentDateTime, MaxRPS};
        if (const auto error = allEpisodes.WaitAndParseResponse(&tvShowData))
            return TError{error->Msg};

        request = std::move(tvShowData.SerialDescriptor.Build());
        return Nothing();
    }

private:
    const TString ItemId;
    IRequestProvider::TPtr RequestProvider;
    IUAPIVideoProvider::TPtr VideoProvider;
    const TInstant CurrentDateTime;
    size_t MaxRPS;
    TTvShowRequestHolder UAPIRequest;
};

class TUAPIContentListRequest : public IVideoItemListHandle {
public:
    TUAPIContentListRequest(IUAPIVideoProvider::TPtr videoProvider, IMultiRequest::TRef multiRequest)
        : VideoProvider(videoProvider)
        , AllItemsHandler(VideoProvider->GetParseHelper(),
                          *VideoProvider->GetUAPIRequestProvider().CreateRootItemsRequest(multiRequest),
                          Nothing() /* previousUpdateTime */) {
    }

    TResult WaitAndParseResponse(TVideoItemList &request) override {
        TUAPIFetchResult fetchResult;
        if (const auto error = AllItemsHandler.WaitAndParseResponse(&fetchResult))
            return TError{error->Msg};

        TVideoItemList result;
        result.Items.reserve(fetchResult.Movies.size() + fetchResult.TvShows.size());
        result.Updated = fetchResult.UpdateTime;

        AddAllItems(result, MOVIE_TYPE, fetchResult.Movies,
                    [](const TVideoItemUAPIData& item) { return item.ItemId; });
        AddAllItems(result, TV_SHOW_TYPE, fetchResult.TvShows,
                    [](const TTvShowUAPIData& item) { return item.TvShowItem.ItemId; });

        request = std::move(result);
        return Nothing();
    }

private:
    template <typename TContainer, typename TItemIdGetter>
    void AddAllItems(TVideoItemList& result, TStringBuf type, const TContainer& container,
                     TItemIdGetter&& itemIdGetter) {
        for (const auto& item : container) {
            auto providerItemId = VideoProvider->GetParseHelper().GetProviderItemId(itemIdGetter(item));
            if (!providerItemId) {
                LOG(ERR) << "Invalid items should not get into content list!" << Endl;
                continue;
            }

            TVideoItem key;
            key->ProviderItemId() = *providerItemId;
            key->Type() = type;
            key->ProviderName() = VideoProvider->GetProviderName();
            result.Items.push_back(std::move(key));
        }
    }

    IUAPIVideoProvider::TPtr VideoProvider;
    TAllItemsRequestHandle AllItemsHandler;
};

class TUAPIContentInfoProvider : public IContentInfoProvider {
public:
    TUAPIContentInfoProvider(IRequestProvider::TPtr requestProvider, IUAPIVideoProvider::TPtr videoProvider,
                             size_t maxEpisodeRPS, TInstant currentDateTime)
        : RequestProvider(requestProvider)
        , VideoProvider(videoProvider)
        , MaxEpisodeRPS(maxEpisodeRPS)
        , CurrentDateTime(currentDateTime)
    {
    }

    bool IsContentListAvailable() const override { return true; }

    std::unique_ptr<IVideoItemListHandle> MakeContentListRequest(IMultiRequest::TRef multiRequest) override {
        return std::make_unique<TUAPIContentListRequest>(VideoProvider, multiRequest);
    }

    // NOTE: UAPI serial descriptor request is synchronous and should not be used in real-time scenarios.
    std::unique_ptr<ISerialDescriptorHandle>
    MakeSerialDescriptorRequest(TVideoItemConstScheme tvShowItem,
                                NHttpFetcher::IMultiRequest::TRef multiRequest) override {
        Y_ASSERT(tvShowItem->Type() == TV_SHOW_TYPE);
        return std::make_unique<TUAPISerialDescriptorContentInfoRequest>(RequestProvider, VideoProvider, tvShowItem,
                                                                         CurrentDateTime, multiRequest, MaxEpisodeRPS);
    }

    EPreferredSeasonDownloadMode GetPreferredSeasonDownloadMode() const override {
        return EPreferredSeasonDownloadMode::Individual;
    }

    // NOTE: UAPI season descriptor request is synchronous and should not be used in real-time scenarios.
    std::unique_ptr<ISeasonDescriptorHandle>
    MakeSeasonDescriptorRequest(const TSerialDescriptor& /* serialDescr */, const TSeasonDescriptor& seasonDescr,
                                NHttpFetcher::IMultiRequest::TRef /* multiRequest */) override {
        return std::make_unique<TMockContentRequestHandle<TSeasonDescriptor>>(seasonDescr);
    }

protected:
    std::unique_ptr<IVideoItemHandle>
    MakeContentInfoRequestImpl(TLightVideoItemConstScheme item,
                               NHttpFetcher::IMultiRequest::TRef multiRequest) override {
        if (item->Type() == TV_SHOW_TYPE) {
            return std::make_unique<TUAPITvShowContentInfoRequest>(VideoProvider, item, multiRequest, CurrentDateTime);
        }
        return std::make_unique<TUAPIVideoItemContentInfoRequest>(VideoProvider, item, multiRequest, CurrentDateTime);
    }

private:
    IRequestProvider::TPtr RequestProvider;
    IUAPIVideoProvider::TPtr VideoProvider;
    TMaybe<TTimePoint> LastDownload;
    const size_t MaxEpisodeRPS;
    const TInstant CurrentDateTime;
};

} // namespace

// ------------------- IUAPIParseHelper -------------------------------------------------------------------

TResultValue IUAPIParseHelper::ParseItemList(TUAPIFetchResult* response, const NSc::TValue& responseData) const {
    const NSc::TArray& contentList = responseData["content_items"].GetArray();
    if (contentList.empty())
        return MakeVideoError("Got an empty content list from provider!");

    bool hasValidContent = false;

    for (const auto& value : contentList) {
        TStringBuf fullId = value.GetString();
        if (const auto error = HandleItem(response, TString{fullId}))
            LOG(ERR) << "Item list parsing error for item " << TString{fullId}.Quote() << ": " << error->Msg << Endl;
        else
            hasValidContent = true;
    }

    if (!hasValidContent)
        return MakeVideoError("Root content list doesn't have any valid content items!");

    return ResultSuccess();
}

TMaybe<TInstant> IUAPIParseHelper::ParseUpdateTime(const NSc::TValue& responseData) const {
    return TryParseTimeField(responseData, TStringBuf("updated_at"));
}

TResultValue IUAPIParseHelper::HandleItem(TUAPIFetchResult* response, TString itemId) const {
    auto fullId = TryExpandFullId(itemId);
    if (!fullId)
        return MakeVideoError("Cannot parse content type for itemId: ", itemId);

    if (fullId->ContentType == MOVIE_TYPE) {
        response->Movies.push_back(CreateWithItemId<TVideoItemUAPIData>(itemId));
        return ResultSuccess();
    }

    // FIXME (a-sidorin@): Implement support for anthology_movie.
    if (fullId->ContentType == TV_SHOW_TYPE /* || fullId->ContentType == ANTHOLOGY_MOVIE_TYPE */) {
        response->TvShows.emplace_back();
        response->TvShows.back().TvShowItem.ItemId = std::move(itemId);
        return ResultSuccess();
    }

    return MakeVideoError("Root items of kind ", itemId.Quote(), " are not supported!");
}

TString TKinopoiskUAPIParseHelper::GetHumanReadableId(const TString& /* itemId */,
                                                      const NSc::TValue& /* responseData */) const {
    return TString{};
}

TMaybe<TString> TKinopoiskUAPIParseHelper::GetProviderItemId(const TString& itemId) const {
    if (auto fullId = TryExpandFullId(itemId))
        return fullId->ProviderItemId;
    return Nothing();
}

TString TOkkoUAPIParseHelper::GetHumanReadableId(const TString& /* itemId */, const NSc::TValue& responseData) const {
    TString urlToItemPage = responseData.TrySelect("url_to_item_page").ForceString();
    TStringBuf urlBuf(urlToItemPage);
    return TString{urlBuf.RAfter('/')};
}

TMaybe<TString> TOkkoUAPIParseHelper::GetProviderItemId(const TString& itemId) const {
    if (auto fullId = TryExpandFullId(itemId))
        return fullId->ProviderItemId;
    return Nothing();
}

// ------------------- THttpUAPIRequestProvider -----------------------------------------------------------

THolder<NHttpFetcher::TRequest>
THttpUAPIRequestProvider::CreateRootItemsRequest(IMultiRequest::TRef multiRequest) const {
    // FIXME (a-sidorin@): Use single query after it becomes available to mocking.
    return RequestFactory->AttachRequest(GetRootItemListPath(), multiRequest);
}

TVideoRequestHolder THttpUAPIRequestProvider::CreateVideoItemRequest(IMultiRequest::TRef multiRequest,
                                                                     const IUAPIParseHelper& parseHelper,
                                                                     const TString& itemId,
                                                                     TInstant currentDateTime) const {
    auto request = RequestFactory->AttachRequest(GetItemPath(itemId), multiRequest);
    return std::make_unique<TMovieRequestHandle>(*request, parseHelper, currentDateTime);
}

TTvShowRequestHolder THttpUAPIRequestProvider::CreateTvShowRequest(IMultiRequest::TRef multiRequest,
                                                                   const IUAPIParseHelper& parseHelper,
                                                                   const TString& itemId,
                                                                   TInstant currentDateTime) const {
    auto request = RequestFactory->AttachRequest(GetItemPath(itemId), multiRequest);
    return std::make_unique<TTvShowRequestHandle>(*request, parseHelper, itemId, currentDateTime);
}

TSeasonRequestHolder THttpUAPIRequestProvider::CreateTvShowSeasonRequest(IMultiRequest::TRef multiRequest,
                                                                         const TSerialDescriptor& serialDescr,
                                                                         const TString& seasonId,
                                                                         TInstant currentDateTime) const {
    auto request = RequestFactory->AttachRequest(GetItemPath(seasonId), multiRequest);
    return std::make_unique<TSeasonRequestHandle>(serialDescr.Id, currentDateTime, *request);
}

TVideoRequestHolder THttpUAPIRequestProvider::CreateTvShowEpisodeRequest(IMultiRequest::TRef multiRequest,
                                                                         const TSeasonDescriptor& seasonDescr,
                                                                         const TString& itemId,
                                                                         const IUAPIParseHelper& parseHelper,
                                                                         TInstant currentDateTime) const {
    auto request = RequestFactory->AttachRequest(GetItemPath(itemId), multiRequest);
    return std::make_unique<TTvShowEpisodeRequestHandle>(seasonDescr, *request, parseHelper, currentDateTime);
}

TString THttpUAPIRequestProvider::GetItemPath(const TString &itemId) const {
    Y_ASSERT(!itemId.empty());
    return "content/" + itemId;
}

// ------------------- TSeasonDescriptorUAPIData -----------------------------------------------------------

const TSeasonDescriptor& TSeasonDescriptorUAPIData::Build() {
    Y_ASSERT(IsValid && "Cannot build an invalid descriptor!");

    SeasonDescriptor.EpisodeIds.clear();
    SeasonDescriptor.EpisodeItems.clear();

    Sort(Episodes, [](const TVideoItemUAPIData& lhs, const TVideoItemUAPIData& rhs) {
        return lhs.VideoItem->ProviderNumber() < rhs.VideoItem->ProviderNumber();
    });

    SeasonDescriptor.EpisodeIds.reserve(Episodes.size());
    SeasonDescriptor.EpisodeItems.reserve(Episodes.size());

    size_t episodesCount = 0;
    TMaybe<TInstant> seasonPublicationDate;
    bool hasAvailableEpisodes = false;

    for (const auto& episode : Episodes) {
        if (!episode.IsValid)
            continue;

        auto& item = episode.VideoItem;
        SeasonDescriptor.EpisodeIds.push_back(TString{*item->ProviderItemId()});
        SeasonDescriptor.EpisodeItems.push_back(item);
        ++episodesCount;
        SeasonDescriptor.EpisodeItems.back()->Episode() = episodesCount;

        // NOTE: Only items with 'future' publication_date are allowed to have UpdateAtUs() field
        // by the implementation.
        if (item->HasUpdateAtUs()) {
            TInstant episodePublicationDate = TInstant::MicroSeconds(item->UpdateAtUs());
            if (!seasonPublicationDate || *seasonPublicationDate < episodePublicationDate)
                seasonPublicationDate = episodePublicationDate;
        } else {
            hasAvailableEpisodes = true;
        }
    }

    SeasonDescriptor.EpisodesCount = episodesCount;
    SeasonDescriptor.Soon = SeasonDescriptor.Soon || !hasAvailableEpisodes;
    SeasonDescriptor.UpdateAt = seasonPublicationDate;
    IsBuilt = true;
    return SeasonDescriptor;
}

// ------------------- TSerialDescriptorUAPIData -----------------------------------------------------------

const TSerialDescriptor& TSerialDescriptorUAPIData::Build() {
    SerialDescriptor.Seasons.clear();
    Sort(Seasons, [](const TSeasonDescriptorUAPIData& lhs, const TSeasonDescriptorUAPIData& rhs) {
        return lhs.SeasonDescriptor.ProviderNumber < rhs.SeasonDescriptor.ProviderNumber;
    });

    SerialDescriptor.Seasons.reserve(Seasons.size());

    for (TSeasonDescriptorUAPIData& season : Seasons) {
        if (season.IsValid)
            SerialDescriptor.Seasons.push_back(season.Build());
    }

    size_t totalSeasonsCount = SerialDescriptor.Seasons.size();
    for (TSeasonDescriptor& season : SerialDescriptor.Seasons) {
        for (TVideoItem& episode : season.EpisodeItems) {
            episode->SeasonsCount() = totalSeasonsCount;
            episode->Season() = season.ProviderNumber;
        }
    }

    return SerialDescriptor;
}

void TTvShowUAPIData::Build() {
    TvShowItem.VideoItem->SeasonsCount() = SerialDescriptor.Seasons.size();
}

// ----------------- MakeUAPIContentInfoProvider ----------------------------------------------------------

std::unique_ptr<IContentInfoProvider> MakeUAPIContentInfoProvider(IRequestProvider::TPtr requestProvider,
                                                                  IUAPIVideoProvider::TPtr videoProvider,
                                                                  size_t maxRPS, TInstant currentDateTime) {
    return std::make_unique<TUAPIContentInfoProvider>(requestProvider, videoProvider, maxRPS, currentDateTime);
}

} // namespace NVideoCommon
