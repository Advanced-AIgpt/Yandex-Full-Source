#include "amediateka_provider.h"

#include "web_search.h"

#include <alice/bass/setup/setup.h>

#include <alice/bass/libs/logging_v2/logger.h>
#include <alice/bass/libs/video_common/amediateka_utils.h>
#include <alice/bass/libs/video_common/video.sc.h>

#include <util/generic/noncopyable.h>
#include <util/generic/singleton.h>
#include <library/cpp/string_utils/url/url.h>
#include <util/system/env.h>
#include <util/system/yassert.h>

using namespace NAlice::NVideoCommon;
using namespace NVideoCommon;

/**
 * Amediateka video provider (https://www.amediateka.ru/)
 * API: https://api.amediateka.ru/
 * API reference: https://wiki.yandex-team.ru/assistant/backend/private/#provajjderamediateka
 */

namespace NBASS {
namespace NVideo {

namespace {

const THashMap<EVideoGenre, TString> AMEDIATEKA_VIDEO_GENRES_MAP {
    { EVideoGenre::Action, "Боевики" },
    { EVideoGenre::Adventure, "Приключения" },
    { EVideoGenre::Biopic, "Биографические" },
    { EVideoGenre::Comedy, "Комедии" },
    { EVideoGenre::Crime, "Криминал" },
    { EVideoGenre::Detective, "Детективы" },
    { EVideoGenre::Documentary, "Документальные" },
    { EVideoGenre::Drama, "Драмы" },
    { EVideoGenre::Erotica, "Эротика" },
    { EVideoGenre::Fantasy, "Фэнтези" },
    { EVideoGenre::Historical, "Исторические" },
    { EVideoGenre::Horror, "Ужасы" },
    { EVideoGenre::Melodramas, "Мелодрамы" },
    { EVideoGenre::Musical, "Музыка" },
    { EVideoGenre::ScienceFiction, "Фантастика" },
    { EVideoGenre::Show, "Шоу" },
    { EVideoGenre::SportVideo, "Спорт" },
    { EVideoGenre::Thriller, "Триллеры" },
    { EVideoGenre::War, "Военные" }
};

void AddSearchLimits(const TVideoClipsRequest& request, TCgiParameters& cgis) {
    cgis.InsertUnescaped(TStringBuf("offset"), ToString(request.From));
    cgis.InsertUnescaped(TStringBuf("limit"), ToString(request.To - request.From));
}

class TAmediatekaSearchHandle : public TSetupRequest<TVideoGalleryScheme> {
public:
    TAmediatekaSearchHandle(const TVideoClipsRequest& request, TContext& context)
        : TSetupRequest<TVideoGalleryScheme>(ConstructRequestId("amediateka_search", request, TCgiParameters()))
        , Context(context)
        , SearchQuery(request.Slots.BuildSearchQueryForWeb())
        , Request(request)
    {
    }

    NHttpFetcher::THandle::TRef Fetch(NHttpFetcher::IMultiRequest::TRef multiRequest) override {
        NHttpFetcher::TRequestPtr req = AttachProviderRequest(Context, Context.GetSources().VideoAmediateka(TStringBuf("/v1/search.json")), multiRequest);

        TCgiParameters cgi;
        TAmediatekaCredentials::Instance().AddClientParams(cgi);
        cgi.InsertUnescaped(TStringBuf("q"), Request.GetSearchQuery());
        req->AddCgiParams(cgi);

        return req->Fetch();
    }

    TResultValue Parse(NHttpFetcher::TResponse::TConstRef resp, TVideoGalleryScheme* response, NSc::TValue* /* factorsData */) override {
        if (resp->IsError()) {
            TString err = TStringBuilder() << "amediateka response: " << resp->GetErrorText();
            LOG(ERR) << err << Endl;
            return TError(TError::EType::SYSTEM, err);
        }

        NSc::TValue json = NSc::TValue::FromJson(resp->Data);
        if (json["results"].GetArray().empty()) {
            LOG(WARNING) << TStringBuf("amediateka response: empty") << Endl;
            return TResultValue();
        }

        /**
         * TODO: Fill here:
         * response->Background16x9();
         */

        for (const auto& elem : json["results"].GetArray()) {
            if (!Context.HasExpFlag(FLAG_VIDEO_ENABLE_SHOWING_VIDEOS_COMING_SOON) && elem["soon"].GetBool())
                continue;

            TVideoItemScheme item = response->Items().Add();
            ParseAmediatekaContentItem(elem, item);
        }
        return TResultValue();
    }

private:
    TContext& Context;
    TString SearchQuery;
    const TVideoClipsRequest Request;
};

class TAmediatekaWebSearchHandle : public TWebSearchByProviderHandle {
public:
    TAmediatekaWebSearchHandle(const TVideoClipsRequest& request, TContext& context)
        : TWebSearchByProviderHandle(PatchQuery(request.Slots.BuildSearchQueryForWeb()), request, PROVIDER_AMEDIATEKA,
                                     context) {
    }

    static TString PatchQuery(TStringBuf query) {
        return TStringBuilder() << query << TStringBuf(" host:www.amediateka.ru");
    }

    bool ParseItemFromDoc(const NSc::TValue doc, TVideoItem* item) override {
        if (!item)
            return false;

        const TStringBuf url = doc["url"].GetString();
        return ParseAmediatekaItemFromUrl(url, *item);
    }
};

class TAmediatekaContentRequestHandle : public IVideoClipsHandle {
public:
    TAmediatekaContentRequestHandle(const TVideoClipsRequest& request, const TCgiParameters& cgis, TContext& context)
        : Context(context)
        , SearchQuery(request.Slots.BuildSearchQueryForWeb())
    {
        const TItemTypeFlags type = request.ItemType;
        if (type.HasFlags(EItemType::Movie)) {
            FilmsHandle = SendContentRequest(request, cgis, TStringBuf("/v1/films.json"));
        }
        if (type.HasFlags(EItemType::TvShow)) {
            SerialsHandle = SendContentRequest(request, cgis, TStringBuf("/v1/serials.json"));
        }
    }

    TResultValue WaitAndParseResponse(TVideoGalleryScheme* response) override {
        if (FilmsHandle) {
            if (TResultValue error = ParseResponse(FilmsHandle, TStringBuf("films"), response)) {
                return error;
            }
        }
        if (SerialsHandle) {
            if (TResultValue error = ParseResponse(SerialsHandle, TStringBuf("serials"), response)) {
                return error;
            }
        }
        return TResultValue();
    }

private:
    NHttpFetcher::THandle::TRef SendContentRequest(const TVideoClipsRequest& request, TCgiParameters cgis, TStringBuf path) {
        NHttpFetcher::TRequestPtr req = AttachProviderRequest(Context, Context.GetSources().VideoAmediateka(path), request.MultiRequest);

        AddSearchLimits(request, cgis);
        req->AddCgiParams(cgis);

        return req->Fetch();
    }

    TResultValue ParseResponse(NHttpFetcher::THandle::TRef handle, TStringBuf key, TVideoGalleryScheme* response) {
        if (!handle) {
            TResultValue();
        }
        NHttpFetcher::TResponse::TRef r = handle->Wait();
        NSc::TValue value;
        if (TResultValue error = ParseVideoResponseJson(*r, &value)) {
            return error;
        }

        for (const NSc::TValue& elem : value[key].GetArray()) {
            if (!Context.HasExpFlag(FLAG_VIDEO_ENABLE_SHOWING_VIDEOS_COMING_SOON) && elem["soon"].GetBool())
                continue;

            TVideoItemScheme item = response->Items().Add();
            ParseAmediatekaContentItem(elem, item);
        }
        return TResultValue();
    }

private:
    TContext& Context;
    TString SearchQuery;

    NHttpFetcher::THandle::TRef FilmsHandle;
    NHttpFetcher::THandle::TRef SerialsHandle;
};

class TItemContentRequests {
public:
    TItemContentRequests(TContext& context)
        : Context(context)
        , MultiRequest(NHttpFetcher::MultiRequest())
    {
    }

    void AddRequest(TVideoItemScheme inputItem, TVideoItemScheme resultItem) {
        TStringBuf id = inputItem.HumanReadableId();

        EItemType type;
        if (!TryFromString(inputItem.Type(), type)) {
            return;
        }

        TStringBuilder path;
        switch (type) {
        case EItemType::Movie:
            path << TStringBuf("/v1/films/");
            break;
        case EItemType::TvShow:
            path << TStringBuf("/v1/serials/");
            break;
        default:
            return;
        }
        path << id << TStringBuf(".json");

        auto res = Requests.emplace(id, TRequestContext(type, resultItem));
        if (res.second) {
            TRequestContext& rctx = res.first->second;

            TCgiParameters cgis;
            TAmediatekaCredentials::Instance().AddClientParams(cgis);

            NHttpFetcher::TRequestPtr r = Context.GetSources().VideoAmediateka(path).AttachRequest(MultiRequest);
            r->AddCgiParams(cgis);
            rctx.Handle = r->Fetch();
        }
    }

    void WaitAndFillResults() {
        MultiRequest->WaitAll();

        for (auto& nameRequest : Requests) {
            TRequestContext& rctx = nameRequest.second;
            NHttpFetcher::TResponse::TRef response = rctx.Handle->Wait();
            if (response->IsError()) {
                LOG(ERR) << "amediateka item " << nameRequest.first << " error: " << response->GetErrorText() << Endl;
                rctx.ResultItem.Clear();
                continue;
            }

            NSc::TValue json = NSc::TValue::FromJson(response->Data);
            TStringBuf key;
            switch (rctx.ContentType) {
            case EItemType::Movie:
                key = TStringBuf("film");
                break;
            case EItemType::TvShow:
                key = TStringBuf("serial");
                break;
            default:
                continue;
            }
            ParseAmediatekaContentItem(json[key], rctx.ResultItem);
        }
    }

private:
    TContext& Context;
    NHttpFetcher::IMultiRequest::TRef MultiRequest;

    struct TRequestContext {
        const EItemType ContentType;

        NHttpFetcher::THandle::TRef Handle;
        TVideoItemScheme ResultItem;

        TRequestContext(EItemType contentType, TVideoItemScheme resultItem)
            : ContentType(contentType)
            , ResultItem(resultItem)
        {}
    };
    THashMap<TStringBuf, TRequestContext> Requests;
};

} // namespace

// TAmediatekaSourceRequestFactory ---------------------------------------------
TAmediatekaSourceRequestFactory::TAmediatekaSourceRequestFactory(TSourcesRequestFactory sources)
    : TProviderSourceRequestFactory(sources, &TSourcesRequestFactory::VideoAmediateka) {
}

TAmediatekaSourceRequestFactory::TAmediatekaSourceRequestFactory(TContext& ctx)
    : TProviderSourceRequestFactory(ctx, &TSourcesRequestFactory::VideoAmediateka) {
}

// TAmediatekaClipsProvider ----------------------------------------------------
std::unique_ptr<IVideoClipsHandle>
TAmediatekaClipsProvider::MakeSearchRequest(const TVideoClipsRequest& request) const {
    return FetchSetupRequest<NVideo::TVideoGalleryScheme>(MakeHolder<TAmediatekaSearchHandle>(request, Context),
                                                          Context, request.MultiRequest);
}

std::unique_ptr<IWebSearchByProviderHandle>
TAmediatekaClipsProvider::MakeWebSearchRequest(const TVideoClipsRequest& request) const {
    return FetchSetupRequest<NVideo::TWebSearchByProviderResponse>(
        MakeHolder<TAmediatekaWebSearchHandle>(request, Context), Context, request.MultiRequest);
}

std::unique_ptr<IVideoClipsHandle>
TAmediatekaClipsProvider::MakeNewVideosRequest(const TVideoClipsRequest& request) const {
    TCgiParameters cgis;
    TAmediatekaCredentials::Instance().AddClientParams(cgis);
    cgis.InsertUnescaped(TStringBuf("sort_by"), TStringBuf("priority_and_available_start"));
    return std::make_unique<TAmediatekaContentRequestHandle>(request, cgis, Context);
}

std::unique_ptr<IVideoClipsHandle>
TAmediatekaClipsProvider::MakeTopVideosRequest(const TVideoClipsRequest& request) const {
    TCgiParameters cgis;
    TAmediatekaCredentials::Instance().AddClientParams(cgis);
    //TODO: check it
    //cgis.InsertUnescaped(TStringBuf("filter_flag"), TStringBuf("featured_new"));
    cgis.InsertUnescaped(TStringBuf("sort_by"), TStringBuf("kinopoisk_rating"));
    return std::make_unique<TAmediatekaContentRequestHandle>(request, cgis, Context);
}

std::unique_ptr<IVideoClipsHandle>
TAmediatekaClipsProvider::MakeRecommendationsRequest(const TVideoClipsRequest& request) const {
    TCgiParameters cgis;
    cgis.InsertUnescaped(TStringBuf("client_id"), TStringBuf("amediateka"));
    cgis.InsertUnescaped(TStringBuf("filter_flag"), TStringBuf("featured_front_page"));
    cgis.InsertUnescaped(TStringBuf("sort_by"), TStringBuf("watch_count"));
    return std::make_unique<TAmediatekaContentRequestHandle>(request, cgis, Context);
}

std::unique_ptr<IVideoClipsHandle>
TAmediatekaClipsProvider::MakeVideosByGenreRequest(const TVideoClipsRequest& request) const {
    auto genreIt = AMEDIATEKA_VIDEO_GENRES_MAP.find(request.Slots.VideoGenre.GetEnumValue());
    if (genreIt == AMEDIATEKA_VIDEO_GENRES_MAP.end()) {
        return MakeDummyRequest<TVideoGalleryScheme>();
    }

    TCgiParameters cgis;
    cgis.InsertUnescaped(TStringBuf("client_id"), TStringBuf("amediateka"));
    cgis.InsertUnescaped(TStringBuf("filter_genres"), genreIt->second);
    cgis.InsertUnescaped(TStringBuf("sort_by"), TStringBuf("priority_and_available_start"));
    return std::make_unique<TAmediatekaContentRequestHandle>(request, cgis, Context);
}

IVideoClipsProvider::TPlayResult
TAmediatekaClipsProvider::GetPlayCommandDataImpl(TVideoItemConstScheme item,
                                                 TPlayVideoCommandDataScheme commandData) const {
    TStringBuilder path;

    EItemType type = FromString<EItemType>(item.Type());
    TString idToPlay = TString{item.ProviderItemId().Get()};

    switch (type) {
    case EItemType::Movie:
        path << TStringBuf("/external/v1/films/");
        break;

    case EItemType::TvShowEpisode:
        path << TStringBuf("/external/v1/episodes/");
        break;

    default: {
        TString errText = TStringBuilder() << "Unsupported content type " << ToString(type);
        LOG(ERR) << errText << Endl;
        return TPlayError{EPlayError::VIDEOERROR, errText};
    }
    }
    path << idToPlay;
    path << TStringBuf("/streams.json");

    TCgiParameters cgis;
    const auto& crds = TAmediatekaCredentials::Instance();
    crds.AddClientParams(cgis);

    TStringBuf token;
    if (crds.GetAccessToken()) {
        token = crds.GetAccessToken();
    } else {
        token = GetAuthToken();
    }

    if (token.empty()) {
        TString err = TStringBuilder() << "Cannot start playing without token";
        LOG(ERR) << "amediateka error: " << err << Endl;
        return TPlayError{EPlayError::UNAUTHORIZED, err};
    }
    cgis.InsertUnescaped(TStringBuf("access_token"), token);
    cgis.InsertUnescaped(TStringBuf("customer_ip"), Context.UserIP());
    cgis.InsertUnescaped(TStringBuf("protocol"), TStringBuf("dash"));
    cgis.InsertUnescaped(TStringBuf("drm"), TStringBuf("widevine"));
    cgis.InsertUnescaped(TStringBuf("platform"), TStringBuf("desktop"));

    NHttpFetcher::TResponse::TRef response =
        CreateProviderRequest(Context, Context.GetSources().VideoAmediateka(path))->AddCgiParams(cgis).Fetch()->Wait();
    if (response->IsError()) {
        LOG(ERR) << "amediateka error: " << response->GetErrorText() << Endl;
        return TPlayError{EPlayError::UNEXPLAINABLE, response->GetErrorText()};
    }

    NSc::TValue json = NSc::TValue::FromJson(response->Data);
    const NSc::TValue url = json["streams"].GetArray()[0]["url"];
    if (url.IsNull()) {
        constexpr TStringBuf error = "Failed to obtain stream url from amediateka response";
        LOG(ERR) << error << ": " << json.ToJson() << Endl;
        return TPlayError{EPlayError::STREAMS_NOT_FOUND, error};
    }
    commandData->Uri() = url.GetString();
    return {};
}

std::unique_ptr<IVideoItemHandle>
TAmediatekaClipsProvider::DoMakeContentInfoRequest(TVideoItemConstScheme item,
                                                   NHttpFetcher::IMultiRequest::TRef multiRequest,
                                                   bool /* forceUnfiltered */) const {
    TAmediatekaContentInfoProvider provider(std::make_unique<TAmediatekaSourceRequestFactory>(Context),
                                            Context.HasExpFlag(FLAG_VIDEO_ENABLE_SHOWING_VIDEOS_COMING_SOON));
    return provider.MakeContentInfoRequest(item, multiRequest);
}

TResultValue TAmediatekaClipsProvider::DoGetSerialDescriptor(TVideoItemConstScheme tvShowItem,
                                                             TSerialDescriptor* serialDescr) const {
    TAmediatekaContentInfoProvider provider(std::make_unique<TAmediatekaSourceRequestFactory>(Context),
                                            Context.HasExpFlag(FLAG_VIDEO_ENABLE_SHOWING_VIDEOS_COMING_SOON));

    auto multiRequest = NHttpFetcher::WeakMultiRequest();
    Y_ASSERT(multiRequest);

    auto request = provider.MakeSerialDescriptorRequest(tvShowItem, multiRequest);
    Y_ASSERT(request);

    if (const auto error = request->WaitAndParseResponse(*serialDescr))
        return TError{TError::EType::VIDEOERROR, error->Msg};
    return {};
}

std::unique_ptr<ISeasonDescriptorHandle>
TAmediatekaClipsProvider::DoMakeSeasonDescriptorRequest(const TSerialDescriptor& serialDescr,
                                                        const TSeasonDescriptor& seasonDescr,
                                                        NHttpFetcher::IMultiRequest::TRef multiRequest) const {
    TAmediatekaContentInfoProvider provider(std::make_unique<TAmediatekaSourceRequestFactory>(Context),
                                            Context.HasExpFlag(FLAG_VIDEO_ENABLE_SHOWING_VIDEOS_COMING_SOON));
    return provider.MakeSeasonDescriptorRequest(serialDescr, seasonDescr, multiRequest);
}

std::unique_ptr<IRequestHandle<TString>>
TAmediatekaClipsProvider::FetchAuthToken(TStringBuf /* passportUid */,
                                         NHttpFetcher::IMultiRequest::TRef /* multiRequest */) const {
    // Token is no longer needed since all the required data is provided by billing.
    return MakeDummyRequest<TString>();
}

} // namespace NVideo
} // namespace NBASS
