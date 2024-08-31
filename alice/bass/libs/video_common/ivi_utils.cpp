#include "ivi_utils.h"

#include "ivi_genres.h"

#include <alice/bass/libs/logging_v2/logger.h>
#include <alice/bass/util/error.h>

#include <util/generic/vector.h>
#include <library/cpp/cgiparam/cgiparam.h>
#include <util/string/split.h>
#include <library/cpp/string_utils/url/url.h>
#include <util/system/env.h>
#include <util/system/types.h>

namespace NVideoCommon {

namespace {

constexpr int PERSONS_ACTORS_INFO_ID = 6;
constexpr int PERSONS_DIRECTORS_INFO_ID = 3;

bool IsValidItemId(TStringBuf id) {
    ui64 val;
    return TryFromString(id, val);
}

TString MakeUrlWithSizes(TStringBuf inputUrl, TStringBuf width, TStringBuf height) {
    TStringBuilder url;
    url << inputUrl;
    url << '/' << width << 'x' << height << '/';
    return url;
}

EItemType ParseItemType(const NSc::TValue& providerItem) {
    TStringBuf value = providerItem["object_type"].GetString();
    if (value == TStringBuf("video"))
        return providerItem.Has("serial_number") ? EItemType::TvShowEpisode : EItemType::Movie;
    if (value == TStringBuf("compilation"))
        return EItemType::TvShow;
    LOG(WARNING) << "Unknown ivi item type: " << value << Endl;
    return EItemType::Null;
}

ui32 ParseSeasonsCount(const NSc::TValue& elem) {
    const i64 seasonCountOriginal = elem["seasons_count"].GetIntNumber();

    if (seasonCountOriginal > 0)
        return seasonCountOriginal;

    if ((seasonCountOriginal == 0) && !elem["episodes"].ArrayEmpty() &&
        (elem["object_type"].GetString() == TStringBuf("compilation"))) {
        if (ui32 descSize = elem["seasons_description"].DictSize(); descSize > 0)
            return descSize;

        ui32 episodesCount = elem["episodes"].ArraySize();
        if (ui32 totalContents = elem["total_contents"].GetNumber(); totalContents != episodesCount) {
            LOG(WARNING) << "Mismatch between 'total_contents' and episodes count in ivi response: " << totalContents
                         << " vs " << episodesCount << ", using episodes data" << Endl;
        }

        return 1;
    }
    return 0;
}

void AppendPersons(TStringBuilder& target, const NSc::TValue& persons) {
    for (const auto& who : persons.GetArray()) {
        if (!who["name"].IsString() || who["name"].GetString().empty()) {
            continue;
        }
        if (!target.empty()) {
            target << TStringBuf(", ");
        }
        target << who["name"].GetString();
    }
}

void ParsePersonsItem(const NSc::TValue& elem, TVideoItemScheme& item) {
    TStringBuilder actors;
    TStringBuilder directors;
    for (const auto& item : elem.GetArray()) {
        const i64 personsInfoId = item["id"].GetIntNumber();
        const auto& persons = item["persons"];
        if (personsInfoId == PERSONS_ACTORS_INFO_ID) {
            AppendPersons(actors, persons);
        } else if (personsInfoId == PERSONS_DIRECTORS_INFO_ID) {
            AppendPersons(directors, persons);
        }
    }
    if (!actors.empty()) {
        item.Actors() = actors;
    }
    if (!directors.empty()) {
        item.Directors() = directors;
    }
}

bool IsCompilation(TLightVideoItemConstScheme item) {
    const TStringBuf hru = item.HumanReadableId();
    return !hru.empty() || item.Type() == ToString(EItemType::TvShow);
}

class TAuxInfoHandle : public IVideoItemHandle {
public:
    TAuxInfoHandle(ISourceRequestFactory& source, TLightVideoItemConstScheme item,
                   NHttpFetcher::IMultiRequest::TRef multiRequest) {
        Y_ASSERT(IsFeasible(item));
        Id = item->ProviderItemId();

        const auto path =
            IsCompilation(item) ? TStringBuf("/compilation/persons/v5/") : TStringBuf("/video/persons/v5/");

        TCgiParameters cgis;
        cgis.InsertUnescaped(TStringBuf("app_version"), IVI_APP_VERSION);
        cgis.InsertUnescaped(TStringBuf("id"), Id);
        TIviCredentials::Instance().AddSession(cgis);

        auto request = source.AttachRequest(path, multiRequest);
        request->AddCgiParams(cgis);

        Handle = request->Fetch();
    }

    static bool IsFeasible(TLightVideoItemConstScheme item) {
        return item->HasProviderItemId() && !item->ProviderItemId().Get().empty();
    }

    // IVideoItemHandle overrides:
    TResult WaitAndParseResponse(TVideoItem& item) override {
        if (!Handle)
            return TError{"Failed to download aux info"};

        auto response = Handle->Wait();
        if (!response) {
            const TString msg = TStringBuilder() << "item " << Id << " failed to resolve aux info";
            LOG(ERR) << "ivi error: " << msg << Endl;
            return TError{msg};
        }
        if (response->IsError()) {
            const TString msg = TStringBuilder() << "item " << Id
                                                 << " failed to resolve aux info: " << response->GetErrorText();
            LOG(ERR) << "ivi error: " << msg << Endl;
            return TError{msg, response->Code};
        }

        NSc::TValue data;
        if (!NSc::TValue::FromJson(data, response->Data)) {
            const TString msg = TStringBuilder() << "item " << Id << " failed to parse aux info json";
            LOG(ERR) << "ivi error: " << msg << Endl;
            return TError{msg};
        }

        ParsePersonsItem(data["result"], item.Scheme());
        return {};
    }

private:
    TString Id;
    NHttpFetcher::THandle::TRef Handle;
};

class TContentInfoHandle : public IVideoItemHandle {
public:
    TContentInfoHandle(ISourceRequestFactory& source, TIviGenres& genres, TLightVideoItemConstScheme item,
                       NHttpFetcher::IMultiRequest::TRef multiRequest)
        : Genres(genres) {
        TStringBuf id = item.ProviderItemId();
        TStringBuf hru = item.HumanReadableId();

        const bool isCompilation = IsCompilation(item);

        TStringBuf path;
        TCgiParameters cgis;
        if (isCompilation) {
            path = TStringBuf("/compilationinfo/v5/");
            if (hru.empty()) {
                cgis.InsertUnescaped(TStringBuf("id"), id);
            } else {
                cgis.InsertUnescaped(TStringBuf("hru"), hru);
            }
            DebugId = hru;
        } else {
            path = TStringBuf("/videoinfo/v6/");
            cgis.InsertUnescaped(TStringBuf("id"), id);
            DebugId = id;
        }

        TIviCredentials::Instance().AddSession(cgis);

        cgis.InsertUnescaped(TStringBuf("app_version"), IVI_APP_VERSION);

        auto request = source.AttachRequest(path, multiRequest);
        request->AddCgiParams(cgis);
        Handle = request->Fetch();
    }

    TResult WaitAndParseResponse(TVideoItem& item) override {
        NHttpFetcher::TResponse::TRef response = Handle->Wait();
        if (response->IsError()) {
            TString err = TStringBuilder() << "item " << DebugId.Quote()
                                           << " failed to resolve: " << response->GetErrorText();
            LOG(ERR) << "ivi error: " << err << Endl;
            return TError{err, response->Code};
        }

        NSc::TValue json = NSc::TValue::FromJson(response->Data);
        NSc::TValue& result = json["result"];
        if (result["subsites_availability"].GetArray().empty()) {
            TString text = TStringBuilder() << "item " << DebugId.Quote() << " is not available to play at ivi.ru";
            LOG(WARNING) << "ivi warning: " << text << Endl;
            return TError{text};
        }
        if (result["fake"].GetBool()) {
            TString text = TStringBuilder() << "item " << DebugId.Quote() << " is fake at ivi.ru";
            LOG(WARNING) << "ivi warning: " << text << Endl;
            return TError{text};
        }

        ParseIviContentItem(Genres, result, item.Scheme());
        return {};
    }

private:
    TIviGenres& Genres;

    TString DebugId;
    NHttpFetcher::THandle::TRef Handle;
};

class TCombinedInfoHandle : public IVideoItemHandle {
public:
    explicit TCombinedInfoHandle(TVector<std::unique_ptr<IVideoItemHandle>> handles)
        : Handles(std::move(handles)) {
    }

    TResult WaitAndParseResponse(TVideoItem& item) override {
        for (auto& handle : Handles) {
            if (!handle)
                continue;
            if (const auto error = handle->WaitAndParseResponse(item))
                return error;
        }
        return {};
    }

private:
    TVector<std::unique_ptr<IVideoItemHandle>> Handles;
};

class TSerialDescriptorHandle : public ISerialDescriptorHandle {
public:
    TSerialDescriptorHandle(TVideoItemConstScheme tvShowItem, NHttpFetcher::TRequestPtr request)
        : TvShowItem(*tvShowItem.GetRawValue())
        , Handle(request->Fetch()) {
    }

    // ISerialDescriptorHandle overrides:
    TResult WaitAndParseResponse(TSerialDescriptor& serial) override {
        NHttpFetcher::TResponse::TRef response = Handle->Wait();

        if (response->IsError()) {
            const TString err = TStringBuilder() << "failed to obtain serial info: " << response->GetErrorText();
            LOG(ERR) << "ivi error: " << err << Endl;
            return TError{err};
        }

        NSc::TValue json = NSc::TValue::FromJson(response->Data);

        const TStringBuf serialId = TvShowItem->ProviderItemId();
        const NSc::TValue& result = json["result"];
        const ui32 seasonsCount = result["seasons_count"].GetIntNumber();

        if (seasonsCount == 0) {
            TSeasonDescriptor season;

            ui32 episodesCount = result["episodes"].ArraySize();

            season.SerialId = serialId;
            season.Index = 0;
            season.ProviderNumber = 1;
            season.EpisodesCount = episodesCount;

            serial.Seasons.push_back(std::move(season));
        } else {
            serial.Seasons.resize(seasonsCount);

            for (ui32 i = 0; i < seasonsCount; ++i) {
                TSeasonDescriptor& season = serial.Seasons[i];

                season.SerialId = serialId;
                season.Index = i;
                const auto providerNumber =
                    result.TrySelect(TStringBuilder() << TStringBuf("seasons/") << i).GetIntNumber();
                season.ProviderNumber = (providerNumber > 0) ? providerNumber : (i + 1);

                const auto pn = ToString(season.ProviderNumber);

                season.Id = ToString(result["seasons_extra_info"][pn]["season_id"].GetIntNumber());
                season.EpisodesCount = result["seasons_content_total"][pn].GetIntNumber();
            }
        }

        serial.Id = TString{serialId};
        serial.TotalEpisodesCount = result["total_contents"].GetIntNumber();

        return {};
    }

private:
    const TVideoItem TvShowItem;
    NHttpFetcher::THandle::TRef Handle;
};

class TAllEpisodesRequest {
public:
    static constexpr ui32 MaxEpisodesPerRequest = 100; // Limited by ivi.ru

    TAllEpisodesRequest(const TSerialDescriptor& serial, ISourceRequestFactory& source,
                        NHttpFetcher::IMultiRequest::TRef multiRequest) {
        for (ui32 episodesRequested = 0; episodesRequested < serial.TotalEpisodesCount;
             episodesRequested += MaxEpisodesPerRequest) {
            const ui32 from = episodesRequested;
            const ui32 to = from + MaxEpisodesPerRequest - 1;

            auto request = source.AttachRequest("/videofromcompilation/v5/", multiRequest);
            Y_ASSERT(request);

            TCgiParameters cgis;
            TIviCredentials::Instance().AddSession(cgis);
            cgis.InsertUnescaped(TStringBuf("id"), serial.Id);
            cgis.InsertUnescaped(TStringBuf("from"), ToString(from));
            cgis.InsertUnescaped(TStringBuf("to"), ToString(to));
            cgis.InsertUnescaped(TStringBuf("app_version"), IVI_APP_VERSION);
            request->AddCgiParams(cgis);
            Handles.push_back(request->Fetch());
        }
    }

    template <typename TFn>
    [[nodiscard]] TResult ForEachEpisode(const TFn& fn) {
        TVector<TString> responses;
        for (auto& handle : Handles) {
            auto response = handle->Wait();
            if (response->IsError()) {
                const TString msg = TStringBuilder() << "Ivi error: failed to obtain episode info: "
                                                     << response->GetErrorText();
                LOG(ERR) << msg << Endl;
                return TError{msg, response->Code};
            }
            responses.push_back(std::move(response->Data));
        }

        for (const TString& response : responses) {
            NSc::TValue json = NSc::TValue::FromJson(response);

            const NSc::TArray& result = json["result"].GetArray();
            if (result.empty()) {
                LOG(WARNING) << "Ivi response array is empty" << Endl;
                continue;
            }

            for (const NSc::TValue& item : result) {
                if (const auto error = fn(item))
                    return error;
            }
        }

        return {};
    }

private:
    TVector<NHttpFetcher::THandle::TRef> Handles;
};

struct TEpisodeParams {
    TVideoItem Item;
    TString Id;
};
using TCollectedEpisodeParams = TMap<ui32, TEpisodeParams>;

void ExtractEpisodeItemAndId(TIviGenres& genres, const NSc::TValue& item, TCollectedEpisodeParams& episodeParams) {
    const ui32 providerEpisodeNumber = item["episode"].GetIntNumber();
    auto [it, wasAdded] = episodeParams.insert({providerEpisodeNumber, TEpisodeParams()});
    if (!wasAdded) {
        LOG(WARNING) << "IVI season parser: trying to process an episode that already exists" << Endl;
        return;
    }
    it->second.Id = ToString(item["id"].GetIntNumber());
    ParseIviContentItem(genres, item, it->second.Item.Scheme());
}

TResult SetAndRemapSeasonEpisodes(TSeasonDescriptor& season, TCollectedEpisodeParams&& episodeParams) {
    // Some tv show seasons have enumeration that does not start from 1 ("Tri kota", for example).
    // We remap them as their original number is stored in the `ProviderNumber` field.
    const ui32 episodesCount = season.EpisodesCount;

    if (episodeParams.size() != episodesCount) {
        TStringBuf msg = "Episode count mismatch in IVI single season request";
        LOG(ERR) << msg << Endl;
        return TError{msg};
    }

    season.EpisodeIds.resize(episodesCount);
    season.EpisodeItems.resize(episodesCount);

    size_t mappedIndex = 0;
    for (auto& [providerEpisodeNumber, episodeParam] : episodeParams) {
        season.EpisodeIds[mappedIndex] = std::move(episodeParam.Id);
        season.EpisodeItems[mappedIndex] = std::move(episodeParam.Item);
        ++mappedIndex;
    }

    episodeParams.clear(); // Leave the rvalue ref in a correct state.
    return {};
}

class TAllSeasonsDescriptorHandle : public IAllSeasonsDescriptorHandle {
public:
    TAllSeasonsDescriptorHandle(const TSerialDescriptor& serial, ISourceRequestFactory& source, TIviGenres& genres,
                                NHttpFetcher::IMultiRequest::TRef multiRequest)
        : Request(serial, source, multiRequest)
        , Genres(genres) {
    }

    TResult WaitAndParseResponse(TSerialDescriptor& serial) override {
        THashMap<ui64, size_t> providerNumberToIndex;

        const size_t seasonsCount = serial.Seasons.size();
        TVector<TCollectedEpisodeParams> allSeasonEpisodeParams(seasonsCount);

        for (size_t i = 0; i < seasonsCount; ++i) {
            auto& season = serial.Seasons[i];
            providerNumberToIndex[season.ProviderNumber] = i;
        }

        const auto error = Request.ForEachEpisode([&](const NSc::TValue& item) -> TResult {
            ui32 mappedSeasonIndex = 0;

            if (item["season"].IsIntNumber() && item["season"].GetIntNumber() >= 0) {
                const ui64 providerNumber = item["season"].GetIntNumber();
                auto foundProviderIndex = providerNumberToIndex.find(providerNumber);
                if (foundProviderIndex == providerNumberToIndex.end()) {
                    const TString msg = TStringBuilder() << "Unknown season number: " << providerNumber;
                    LOG(ERR) << msg << Endl;
                    return TError{msg};
                }
                mappedSeasonIndex = foundProviderIndex->second;
            } else {
                if (seasonsCount != 1) {
                    const TString msg = "Unknown season index";
                    LOG(ERR) << msg << Endl;
                    return TError{msg};
                }
                mappedSeasonIndex = 0;
            }

            ExtractEpisodeItemAndId(Genres, item, allSeasonEpisodeParams[mappedSeasonIndex]);
            return {};
        });

        if (error)
            return error;

        for (size_t mappedSeasonIndex = 0; mappedSeasonIndex < seasonsCount; ++mappedSeasonIndex) {
            if (const auto error = SetAndRemapSeasonEpisodes(serial.Seasons[mappedSeasonIndex],
                                                             std::move(allSeasonEpisodeParams[mappedSeasonIndex])))
                return error;
        }
        return {};
    }

private:
    TAllEpisodesRequest Request;
    TIviGenres& Genres;
};

class TSeasonDescriptorHandle : public ISeasonDescriptorHandle {
public:
    TSeasonDescriptorHandle(const TSerialDescriptor& serial, ISourceRequestFactory& source, TIviGenres& genres,
                            NHttpFetcher::IMultiRequest::TRef multiRequest)
        : Request(serial, source, multiRequest)
        , Genres(genres) {
    }

    TResult WaitAndParseResponse(TSeasonDescriptor& season) override {
        TCollectedEpisodeParams episodeParams;

        const auto error = Request.ForEachEpisode([&](const NSc::TValue& item) -> TResult {
            // Ручка /videofromcompilation возвращает все серии,
            // здесь отбрасываются серии из всех сезонов кроме
            // запрошенного.  Если это не сериал, а многосерийный
            // фильм, поле с номером сезона может отсутствовать,
            // тогда ничего не отбрасываем.
            if (item["season"].IsIntNumber() && (item["season"].GetIntNumber() >= 0) &&
                (static_cast<ui64>(item["season"].GetIntNumber()) != season.ProviderNumber)) {
                return {};
            }

            ExtractEpisodeItemAndId(Genres, item, episodeParams);
            return {};
        });

        if (error)
            return error;

        return SetAndRemapSeasonEpisodes(season, std::move(episodeParams));
    }

private:
    TAllEpisodesRequest Request;
    TIviGenres& Genres;
};

} // namespace

bool ParseIviItemFromUrl(TStringBuf url, TVideoItem& item) {
    TStringBuf path = GetPathAndQuery(url);
    if (!path.AfterPrefix("/watch/", path))
        return false;

    TVector<TStringBuf> tokens;
    Split(path, "/", tokens);

    const bool isFreeFilmUrl = tokens.size() == 1 && IsValidItemId(tokens[0]);
    const bool isPaidFilmUrl =
        tokens.size() == 2 && IsValidItemId(tokens[0]) && tokens[1] == TStringBuf("description");
    if (isFreeFilmUrl || isPaidFilmUrl) {
        item->ProviderItemId() = TString{tokens[0]};
        item->Type() = ToString(EItemType::Movie);
        return true;
    }

    const bool isCompilationUrl = tokens.size() == 2 && IsValidItemId(tokens[1]);
    if (isCompilationUrl) {
        item->HumanReadableId() = TString{tokens[0]};
        item->ProviderItemId() = TString{tokens[1]};
        item->Type() = ToString(EItemType::TvShowEpisode);
        return true;
    }

    return false;
}

void ParseIviContentItem(TIviGenres& iviGenres, const NSc::TValue& elem, TVideoItemScheme& item) {
    TString providerItemId = ToString(elem["id"].GetIntNumber());
    item.ProviderName() = PROVIDER_IVI;
    item.ProviderItemId() = providerItemId;

    EItemType type = ParseItemType(elem);
    item.Type() = ToString(type);

    if (type == EItemType::TvShow) {
        TStringBuf hru = elem["hru"].GetString();
        item.HumanReadableId() = hru;
        item.DebugInfo().WebPageUrl() = TStringBuilder() << IVI_HOST << TStringBuf("/watch/") << hru;
    } else {
        item.DebugInfo().WebPageUrl() = TStringBuilder() << IVI_HOST << TStringBuf("/watch/") << providerItemId <<
            TStringBuf("/description");
    }


    if (type == EItemType::TvShowEpisode)
        item.ProviderNumber() = elem["serial_number"];

    if (TStringBuf posterUrl = elem["poster_originals"].GetArray()[0]["path"].GetString()) {
        if (type == EItemType::Movie || type == EItemType::TvShow) {
            item.CoverUrl2X3() = MakeUrlWithSizes(posterUrl, TStringBuf("329"), TStringBuf("506"));
            item.ThumbnailUrl2X3Small() = MakeUrlWithSizes(posterUrl, TStringBuf("88"), TStringBuf("135"));
        }
    }
    if (TStringBuf thumbUrl = elem["thumb_originals"].GetArray()[0]["path"].GetString()) {
        if (type == EItemType::Movie || type == EItemType::TvShow) {
            item.CoverUrl16X9() = MakeUrlWithSizes(thumbUrl, TStringBuf("1920"), TStringBuf("1080"));
        } else if (type == EItemType::TvShowEpisode) {
            item.ThumbnailUrl16X9() = MakeUrlWithSizes(thumbUrl, TStringBuf("640"), TStringBuf("360"));
        }
    }

    item.Name() = elem["title"].GetString();
    item.Description() = elem["description"].GetString();

    if (TMaybe<TDuration> d = ParseDurationString(elem["duration"].GetString())) {
        item.Duration() = d->Seconds();
    }

    const auto& elemGenres = elem["genres"];
    if (!elemGenres.ArrayEmpty()) {
        TVector<TString> genres(Reserve(elemGenres.ArraySize()));
        for (const auto& genreValue : elemGenres.GetArray()) {
            if (const auto genre = iviGenres.GetIviGenreById(genreValue.ForceIntNumber()))
                genres.push_back(*genre);
        }

        if (!genres.empty())
            item.Genre() = JoinStrings(genres, ", ");
    }

    double val;
    if (TryFromString<double>(elem["kp_rating"].GetString(), val)) {
        item.Rating() = val;
    }

    /**
     * TODO:
     * item.ReviewAvailable(); - kinopoisk API? videosearch?
     * item.Progress();        - will be supported right after authorized access, use /video/watchtime/
     */

    item.SeasonsCount() = ParseSeasonsCount(elem);

    /**
     * TODO: use /videoinfo -> year
     * OR: obtain in from another source
     */
    ui32 year;
    if (TryFromString<ui32>(elem["release_date"].GetString().NextTok('-'), year)) {
        item.ReleaseYear() = year;
    }

    item.Available() = true;

    if (const auto& kinopoiskId = elem["kp_id"]; kinopoiskId.IsIntNumber())
        item.MiscIds().Kinopoisk() = ToString(kinopoiskId.GetIntNumber());

    const auto& minAge = elem["restrict"];
    if (minAge.IsIntNumber()) {
        item.MinAge() = minAge.GetIntNumber();
        item.AgeLimit() = ToString(minAge.GetIntNumber());
    }

    /**
     * TODO:
     * item.ViewCount();
     */
}

// TIviCredentials -------------------------------------------------------------
TIviCredentials::TIviCredentials()
    : Session(GetEnv("IVI_SESSION"))
    , ApplicationName(GetEnv("IVI_APPLICATION_NAME", "ivi-yandex-bass"))
{
    if (Session.empty())
        LOG(WARNING) << "Ivi session parameter was not provided, using empty session" << Endl;
}

// static
const TIviCredentials& TIviCredentials::Instance() {
    return *Singleton<TIviCredentials>();
}

const TString& TIviCredentials::GetSession() const {
    return Session;
}

void TIviCredentials::AddSession(TCgiParameters& cgis) const {
    if (Session)
        cgis.InsertUnescaped(TStringBuf("session"), Session);
}

const TString& TIviCredentials::GetApplicationName() const {
    return ApplicationName;
}

// TIviContentInfoProvider -----------------------------------------------------
TIviContentInfoProvider::TIviContentInfoProvider(std::unique_ptr<ISourceRequestFactory> source, TIviGenres& genres)
    : Source(std::move(source))
    , Genres(genres) {
    Y_ASSERT(Source);
}

std::unique_ptr<ISerialDescriptorHandle>
TIviContentInfoProvider::MakeSerialDescriptorRequest(TVideoItemConstScheme tvShowItem,
                                                     NHttpFetcher::IMultiRequest::TRef multiRequest) {
    Y_ASSERT(Source);
    auto request = Source->AttachRequest("/compilationinfo/v5/", multiRequest);
    Y_ASSERT(request);

    const TStringBuf serialId = tvShowItem.ProviderItemId();
    request->AddCgiParam(TStringBuf("id"), serialId);
    request->AddCgiParam(TStringBuf("app_version"), IVI_APP_VERSION);

    return std::make_unique<TSerialDescriptorHandle>(tvShowItem, std::move(request));
}

std::unique_ptr<IAllSeasonsDescriptorHandle>
TIviContentInfoProvider::MakeAllSeasonsDescriptorRequest(const TSerialDescriptor& serialDescr,
                                                         NHttpFetcher::IMultiRequest::TRef multiRequest) {
    Y_ASSERT(Source);
    return std::make_unique<TAllSeasonsDescriptorHandle>(serialDescr, *Source, Genres, multiRequest);
}

std::unique_ptr<ISeasonDescriptorHandle>
TIviContentInfoProvider::MakeSeasonDescriptorRequest(const TSerialDescriptor& serialDescr,
                                                     const TSeasonDescriptor& /* seasonDescr */,
                                                     NHttpFetcher::IMultiRequest::TRef multiRequest) {
    Y_ASSERT(Source);
    return std::make_unique<TSeasonDescriptorHandle>(serialDescr, *Source, Genres, multiRequest);
}

bool TIviContentInfoProvider::IsAuxInfoRequestFeasible(TLightVideoItemConstScheme item) {
    return TAuxInfoHandle::IsFeasible(item);
}

std::unique_ptr<IVideoItemHandle>
TIviContentInfoProvider::MakeAuxInfoRequest(TLightVideoItemConstScheme item,
                                            NHttpFetcher::IMultiRequest::TRef multiRequest) {
    if (!TAuxInfoHandle::IsFeasible(item))
        return {};
    Y_ASSERT(Source);
    return std::make_unique<TAuxInfoHandle>(*Source, item, multiRequest);
}

std::unique_ptr<IVideoItemHandle>
TIviContentInfoProvider::MakeContentInfoRequestImpl(TLightVideoItemConstScheme item,
                                                    NHttpFetcher::IMultiRequest::TRef multiRequest) {
    Y_ASSERT(Source);
    TVector<std::unique_ptr<IVideoItemHandle>> handles;
    handles.push_back(std::make_unique<TContentInfoHandle>(*Source, Genres,  item, multiRequest));
    if (TAuxInfoHandle::IsFeasible(item))
        handles.push_back(std::make_unique<TAuxInfoHandle>(*Source, item, multiRequest));
    return std::make_unique<TCombinedInfoHandle>(std::move(handles));
}

} // namespace NVideoCommon
