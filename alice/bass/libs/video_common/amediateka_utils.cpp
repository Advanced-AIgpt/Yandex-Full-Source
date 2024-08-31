#include "amediateka_utils.h"

#include "utils.h"

#include <alice/bass/libs/logging_v2/logger.h>
#include <alice/bass/util/error.h>

#include <util/string/builder.h>
#include <library/cpp/cgiparam/cgiparam.h>
#include <util/string/subst.h>
#include <library/cpp/string_utils/url/url.h>
#include <util/system/env.h>

namespace NVideoCommon {

namespace {

EItemType ParseItemType(const NSc::TValue& providerItem) {
    TStringBuf value = providerItem["object"].GetString();
    if (value == TStringBuf("film"))
        return EItemType::Movie;
    if (value == TStringBuf("serial"))
        return EItemType::TvShow;
    if (value == TStringBuf("episode"))
        return EItemType::TvShowEpisode;
    LOG(WARNING) << "Unknown amediateka item type: " << value << Endl;
    return EItemType::Null;
}

TString MakeUrlWithSizes(TStringBuf inputUrl, TStringBuf width, TStringBuf height) {
    TString url(inputUrl);
    SubstGlobal(url, "{width}", width);
    SubstGlobal(url, "{height}", height);
    SubstGlobal(url, "{crop}", TStringBuf());
    return url;
}

class TAnnotateContentHandle : public IVideoItemHandle {
public:
    TAnnotateContentHandle(ISourceRequestFactory& source, TStringBuf handler,
                           NHttpFetcher::IMultiRequest::TRef multiRequest) {
        TCgiParameters cgis;
        TAmediatekaCredentials::Instance().AddClientParams(cgis);

        NHttpFetcher::TRequestPtr r = source.AttachRequest(handler, multiRequest);
        r->AddCgiParams(cgis);
        Handle = r->Fetch();
    }

    // TAnnotateContentHandle overrides:
    virtual const NSc::TValue& GetResponseItem(const NSc::TValue& value) const = 0;

    // IVideoItemHandle overrides:
    TResult WaitAndParseResponse(TVideoItem& response) override {
        NHttpFetcher::TResponse::TRef httpResponse = Handle->Wait();

        if (httpResponse->IsError()) {
            const TString err = TStringBuilder() << httpResponse->GetErrorText() << Endl;
            LOG(ERR) << "amediateka item " << Id << " error: " << err << Endl;
            return TError{err, httpResponse->Code};
        }

        const NSc::TValue item = GetResponseItem(NSc::TValue::FromJson(httpResponse->Data));

        ParseAmediatekaContentItem(item, response.Scheme());
        return {};
    }

private:
    const TString Id;
    NHttpFetcher::THandle::TRef Handle;
};

class TAnnotateFilmContentHandle : public TAnnotateContentHandle {
public:
    TAnnotateFilmContentHandle(ISourceRequestFactory& source, TStringBuf id,
                               NHttpFetcher::IMultiRequest::TRef multiRequest)
        : TAnnotateContentHandle(source, MakePath(id), multiRequest) {
    }

    static TString MakePath(TStringBuf id) {
        return TStringBuilder() << TStringBuf("/v1/films/") << id << TStringBuf(".json");
    }

    // TAnnotateContentHandle overrides:
    const NSc::TValue& GetResponseItem(const NSc::TValue& value) const override {
        return value[TStringBuf("film")];
    }
};

class TAnnotateSerialContentHandle : public TAnnotateContentHandle {
public:
    TAnnotateSerialContentHandle(ISourceRequestFactory& source, TStringBuf id,
                                 NHttpFetcher::IMultiRequest::TRef multiRequest)
        : TAnnotateContentHandle(source, MakePath(id), multiRequest) {
    }

    static TString MakePath(TStringBuf id) {
        return TStringBuilder() << TStringBuf("/v1/serials/") << id << TStringBuf(".json");
    }

    // TAnnotateContentHandle overrides:
    const NSc::TValue& GetResponseItem(const NSc::TValue& value) const override {
        return value[TStringBuf("serial")];
    }
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
            LOG(ERR) << "Failed to obtain serial info: " << response->GetErrorText() << Endl;
            return Nothing();
        }

        const TStringBuf serialId = TvShowItem->ProviderItemId();

        NSc::TValue json = NSc::TValue::FromJson(response->Data);
        const NSc::TArray& seasons = json["serial"]["seasons"].GetArray();
        for (ui32 i = 0; i < seasons.size(); ++i) {
            const NSc::TValue& seasonJson = seasons[i];

            TSeasonDescriptor season;
            if (!seasonJson["id"].GetString().empty())
                season.Id.ConstructInPlace(seasonJson["id"].GetString());
            season.SerialId = TString{serialId};
            season.Index = i;
            season.Soon = seasonJson["soon"].GetBool();
            const auto providerNumber = seasonJson["number"].GetIntNumber();
            season.ProviderNumber = (providerNumber > 0) ? providerNumber : (i + 1);

            serial.Seasons.push_back(std::move(season));
        }
        serial.Id = serialId;

        if (TvShowItem->HasMinAge())
            serial.MinAge = TvShowItem->MinAge();

        return {};
    }

private:
    const TVideoItem TvShowItem;
    NHttpFetcher::THandle::TRef Handle;
};

class TSeasonDescriptorHandle : public ISeasonDescriptorHandle {
public:
    TSeasonDescriptorHandle(NHttpFetcher::TRequestPtr request, bool enableShowingItemsComingSoon)
        : Handle(request->Fetch())
        , EnableShowingItemsComingSoon(enableShowingItemsComingSoon)
        , DownloadedAt(TInstant::Now()) {
    }

    TResult WaitAndParseResponse(TSeasonDescriptor& season) override {
        NHttpFetcher::TResponse::TRef response = Handle->Wait();

        if (response->IsError()) {
            TString err = TStringBuilder() << "failed to obtain episode info: " << response->GetErrorText();
            LOG(ERR) << "amediateka error: " << err << Endl;
            return TError{err};
        }

        season.DownloadedAt = DownloadedAt;
        if (season.UpdateAt && season.UpdateAt < season.DownloadedAt)
            season.UpdateAt = Nothing();

        NSc::TValue json = NSc::TValue::FromJson(response->Data);
        for (const auto& episode : json["episodes"].GetArray()) {
            if (episode["soon"].GetBool()) {
                TInstant availableStart;
                const auto& as = episode["available_start"];
                if (as.IsString() && TInstant::TryParseIso8601(as.GetString(), availableStart) &&
                    availableStart > season.DownloadedAt) {
                    if (!season.UpdateAt || season.UpdateAt > availableStart)
                        season.UpdateAt = availableStart;
                }
                if (!EnableShowingItemsComingSoon)
                    continue;
            }

            season.EpisodeIds.push_back(TString{episode["id"].GetString()});

            TVideoItem item;
            ParseAmediatekaContentItem(episode, item.Scheme());
            season.EpisodeItems.push_back(std::move(item));
        }
        season.EpisodesCount = json["episodes"].GetArray().size();
        return {};
    }

private:
    NHttpFetcher::THandle::TRef Handle;
    bool EnableShowingItemsComingSoon;

    TInstant DownloadedAt;
};

} // namespace

bool ParseAmediatekaItemFromUrl(TStringBuf url, TVideoItem& item) {
    TStringBuf path = GetPathAndQuery(url);

    if (!path.SkipPrefix(TStringBuf("/")))
        return false;

    const TStringBuf contentType = path.NextTok('/');
    const TStringBuf textId = path.NextTok('/');

    if (textId.empty())
        return false;

    TStringBuilder b;
    if (contentType == TStringBuf("serial"))
        item->Type() = ToString(EItemType::TvShow);
    else if (contentType == TStringBuf("film"))
        item->Type() = ToString(EItemType::Movie);
    else
        return false;

    item->HumanReadableId() = textId;
    return true;
}

bool ParseAmediatekaAgeRestriction(TStringBuf ageString, ui16& age) {
    ageString.ChopSuffix("+");
    return TryFromString(ageString, age);
}

void ParseAmediatekaContentItem(const NSc::TValue& elem, TVideoItemScheme& item) {
    item.ProviderName() = PROVIDER_AMEDIATEKA;
    item.ProviderItemId() = elem["id"].GetString();

    EItemType type = ParseItemType(elem);
    item.Type() = ToString(type);

    if (const auto& slug = elem["slug"]; slug.IsString()) {
        item.HumanReadableId() = slug;

        if (type == EItemType::Movie || type == EItemType::TvShow) {
            item.DebugInfo().WebPageUrl() = TStringBuilder() << AMEDIATEKA_HOST << '/' <<
                (type == EItemType::Movie ? "film/" : "serial/") <<
                slug.GetString();
        }
    }

    for (const NSc::TValue& image : elem["images"].GetArray()) {
        TStringBuf imageType = image["type"].GetString();
        TStringBuf sizedUrl = image["sized_url"].GetString();

        if (imageType == TStringBuf("v_banner_9_16")) {
            if (type == EItemType::Movie || type == EItemType::TvShow) {
                item.CoverUrl2X3() = MakeUrlWithSizes(sizedUrl, TStringBuf("328"), TStringBuf("492"));
                item.ThumbnailUrl2X3Small() = MakeUrlWithSizes(sizedUrl, TStringBuf("88"), TStringBuf("132"));
            }
        } else if (imageType == TStringBuf("h_banner_16_9")) {
            if (type == EItemType::Movie || type == EItemType::TvShow) {
                item.CoverUrl16X9() = MakeUrlWithSizes(sizedUrl, TStringBuf("1920"), TStringBuf("1080"));
            } else if (type == EItemType::TvShowEpisode) {
                item.ThumbnailUrl16X9() = MakeUrlWithSizes(sizedUrl, TStringBuf("504"), TStringBuf("284"));
            }
        }
    }
    item.Name() = elem["name"];
    item.Description() = elem["description"];
    item.Duration() = elem["duration"].GetIntNumber();

    if (elem["genres"].ArraySize()) {
        const TString singularGenre = SingularizeGenre(elem["genres"][0].ForceString());
        if (singularGenre) {
            item.Genre() = singularGenre;
        }
    }

    item.Rating() = elem["kinopoisk_rating"]["value"].GetNumber();
    /**
     * TODO:
     * item.ReviewAvailable(); - kinopoisk API? videosearch?
     * item.Progress();        - will be supported right after authorized access, use /video/watchtime/
     */

    item.SeasonsCount() = elem["number_of_seasons"].GetIntNumber();

    ui32 year;
    if (TryFromString(elem["year"].GetString(), year)) {
        item.ReleaseYear() = year;
    }

    item.Directors() = elem["director"].GetString();
    item.Actors() = elem["actors"].GetString();

    item.Available() = true;

    if (const auto& imdbId = elem["imdb_id"]; imdbId.IsString())
        item.MiscIds().Imdb() = imdbId.GetString();
    if (const auto& kinopoiskId = elem["kinopoisk_id"]; kinopoiskId.IsString())
        item.MiscIds().Kinopoisk() = kinopoiskId.GetString();

    ui16 minAge;
    if (ParseAmediatekaAgeRestriction(elem["restriction"].GetString(), minAge)) {
        item.MinAge() = minAge;
        item.AgeLimit() = ToString(minAge);
    }

    item.Soon() = elem["soon"].GetBool(); // false by default and it's fine.

    /**
     * TODO:
     * item.ViewCount();
     */
}

// TAmediatekaCredentials ------------------------------------------------------
// static
const TAmediatekaCredentials& TAmediatekaCredentials::Instance() {
    return *Singleton<TAmediatekaCredentials>();
}

const TString& TAmediatekaCredentials::GetApplicationName() const {
    return ApplicationName;
}

const TString& TAmediatekaCredentials::GetAccessToken() const {
    return AccessToken;
}

void TAmediatekaCredentials::AddClientParams(TCgiParameters& cgis) const {
    cgis.ReplaceUnescaped(TStringBuf("client_id"), ClientId);
    cgis.ReplaceUnescaped(TStringBuf("client_secret"), ClientSecret);
}

TAmediatekaCredentials::TAmediatekaCredentials()
    : ApplicationName(GetEnv("AMEDIATEKA_APPLICATION_NAME"))
    , ClientId(GetEnv("AMEDIATEKA_CLIENT_ID"))
    , ClientSecret(GetEnv("AMEDIATEKA_CLIENT_SECRET"))
    , AccessToken(GetEnv("AMEDIATEKA_ACCESS_TOKEN")) {
    if (ApplicationName.empty())
        ApplicationName = "amediateka-yandex-bass";
    if (ClientId.empty() && ClientSecret.empty()) {
        LOG(WARNING)
            << "Amediateka credentials were not provided, will use default account without video playing ability"
            << Endl;
        ClientId = "amediateka";
    }
}

// TAmediatekaContentInfoProvider ----------------------------------------------
TAmediatekaContentInfoProvider::TAmediatekaContentInfoProvider(std::unique_ptr<ISourceRequestFactory> source,
                                                               bool enableShowingItemsComingSoon)
    : Source(std::move(source))
    , EnableShowingItemsComingSoon(enableShowingItemsComingSoon) {
    Y_ASSERT(Source);
}

std::unique_ptr<ISerialDescriptorHandle>
TAmediatekaContentInfoProvider::MakeSerialDescriptorRequest(TVideoItemConstScheme tvShowItem,
                                                            NHttpFetcher::IMultiRequest::TRef multiRequest) {
    Y_ASSERT(Source);

    const TStringBuf serialId = tvShowItem.ProviderItemId();

    TStringBuilder path;
    path << TStringBuf("/external/v1/serials/");
    path << serialId << TStringBuf(".json");

    auto request = Source->AttachRequest(path, multiRequest);
    Y_ASSERT(request);

    TCgiParameters clientCgis;
    const auto& crds = TAmediatekaCredentials::Instance();

    crds.AddClientParams(clientCgis);
    request->AddCgiParams(clientCgis);

    return std::make_unique<TSerialDescriptorHandle>(tvShowItem, std::move(request));
}

std::unique_ptr<ISeasonDescriptorHandle>
TAmediatekaContentInfoProvider::MakeSeasonDescriptorRequest(const TSerialDescriptor& /* serialDescr */,
                                                            const TSeasonDescriptor& seasonDescr,
                                                            NHttpFetcher::IMultiRequest::TRef multiRequest) {
    Y_ASSERT(Source);

    const TString path = TStringBuilder() << TStringBuf("/v1/seasons/") << seasonDescr.Id
                                          << TStringBuf("/episodes.json");

    auto request = Source->AttachRequest(path, multiRequest);
    Y_ASSERT(request);

    TCgiParameters clientCgis;
    const auto& crds = TAmediatekaCredentials::Instance();
    crds.AddClientParams(clientCgis);
    request->AddCgiParams(clientCgis);

    return std::make_unique<TSeasonDescriptorHandle>(std::move(request), EnableShowingItemsComingSoon);
}

std::unique_ptr<IVideoItemHandle>
TAmediatekaContentInfoProvider::MakeContentInfoRequestImpl(TLightVideoItemConstScheme item,
                                                           NHttpFetcher::IMultiRequest::TRef multiRequest) {
    Y_ASSERT(Source);

    EItemType type;
    if (!TryFromString(item.Type(), type)) {
        TString err = TStringBuilder() << "unknown content type " << item.Type() << ", will not annotate item "
                                       << item->ProviderItemId();
        LOG(ERR) << "amediateka error: " << err << Endl;
        return std::make_unique<TDummyContentRequestHandle<TVideoItem>>();
    }

    // Ручки /films и /serials одинаково хорошо работают как с обычными id, так и с короткими алиасами для отображения
    // на сайте (HumanReadableId)
    TStringBuf id = item->HasProviderItemId() ? item.ProviderItemId() : item.HumanReadableId();

    switch (type) {
        case EItemType::Movie:
            return std::make_unique<TAnnotateFilmContentHandle>(*Source, id, multiRequest);
        case EItemType::TvShow:
            return std::make_unique<TAnnotateSerialContentHandle>(*Source, id, multiRequest);
        case EItemType::Null:
        case EItemType::TvShowEpisode:
        case EItemType::Video:
        case EItemType::TvStream:
        case EItemType::CameraStream: {
            TString err = TStringBuilder() << "unsupported item type " << item.Type() << ", will not annotate item "
                                           << item.ProviderItemId();
            LOG(ERR) << "amediateka error: " << err << Endl;
            return std::make_unique<TDummyContentRequestHandle<TVideoItem>>();
        }
    }
}

} // namespace NVideoCommon
