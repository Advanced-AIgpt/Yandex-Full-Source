#include "ivi_provider.h"

#include "web_search.h"

#include <alice/bass/forms/search/serp.h>

#include <alice/bass/libs/globalctx/globalctx.h>
#include <alice/bass/libs/logging_v2/logger.h>
#include <alice/bass/libs/socialism/socialism.h>
#include <alice/bass/libs/source_request/handle.h>
#include <alice/bass/libs/video_common/ivi_genres.h>
#include <alice/bass/libs/video_common/ivi_utils.h>
#include <alice/bass/libs/video_common/video.sc.h>

#include <alice/bass/setup/setup.h>

#include <util/generic/noncopyable.h>
#include <util/generic/singleton.h>
#include <util/string/builder.h>
#include <util/string/cast.h>
#include <util/string/split.h>
#include <library/cpp/string_utils/url/url.h>
#include <util/system/compiler.h>
#include <util/system/env.h>
#include <util/system/yassert.h>

using namespace NVideoCommon;

// Ivi video provider (http://www.ivi.ru/)
// API: https://api.ivi.ru/mobileapi
// API reference (login required): http://docs.ivi.ru/mapidoc/

namespace NBASS {
namespace NVideo {

namespace {

constexpr int IVI_NEW_CARTOONS_CATALOGUE_ID = 1983;
constexpr int IVI_NEW_FILMS_CATALOGUE_ID = 1982;
constexpr int IVI_NEW_SERIES_CATALOGUE_ID = 1984;
constexpr int IVI_MOVIE_CATEGORY_ID = 14;
constexpr int IVI_TVSHOW_CATEGORY_ID = 15;
constexpr int IVI_CARTOON_CATEGORY_ID = 17;
const THashSet<i64> IVI_FAMILY_SEARCH_CATEGORIES = { 502, 503, 504 };

const THashMap<EVideoGenre, TVector<ui16>> IVI_VIDEO_GENRES_MAP {
    { EVideoGenre::Action, { 93, 94, 244 } },
    { EVideoGenre::Adventure, { 100, 101, 255 } },
    { EVideoGenre::Adult, { 124 } },
    { EVideoGenre::Anime, { 125, 184 } },
    { EVideoGenre::Arthouse, { 161 } },
    { EVideoGenre::Biopic, { 226, 230, 237, 243 } },
    // ByComics
    { EVideoGenre::Childrens, { 123, 160, 220, 221 } },
    { EVideoGenre::Comedy, { 95, 110, 251 } },
    // Concert
    { EVideoGenre::Crime, { 217, 218, 252 } },
    { EVideoGenre::Detective, { 96, 97, 247 } },
    { EVideoGenre::Disaster, { 263 } },
    { EVideoGenre::Documentary, { 108, 109, 222, 248 } },
    { EVideoGenre::Drama, { 105, 188, 249 } },
    // Epic
    { EVideoGenre::Erotica, { 162, 169 } },
    { EVideoGenre::Family, { 198, 199, 200, 219 } },
    { EVideoGenre::Fantasy, { 204, 236, 260 } },
    { EVideoGenre::Historical, { 193, 250 } },
    { EVideoGenre::Horror, { 99, 235, 258 } },
    { EVideoGenre::Melodramas, { 107, 106, 196, 253 } },
    { EVideoGenre::Musical, { 189, 227, 232, 239, 254 } },
    { EVideoGenre::Noir, { 229 } },
    // Porno
    { EVideoGenre::Romantic, { 106 } },
    // ScienceVideo
    { EVideoGenre::ScienceFiction, { 190, 259 } },
    // Show
    { EVideoGenre::SportVideo, { 122, 228, 234, 256 } },
    { EVideoGenre::Supernatural, { 195, 197, 201 } },
    { EVideoGenre::Thriller, { 127, 128, 257 } },
    { EVideoGenre::War, { 102, 103, 246 } },
    { EVideoGenre::Westerns, { 225 } }
    // Zombie
};

TMaybe<int> ContentTypeToId(EContentType contentType) {
    if (contentType == EContentType::Movie) {
        return IVI_MOVIE_CATEGORY_ID;
    } else if (contentType == EContentType::TvShow) {
        return IVI_TVSHOW_CATEGORY_ID;
    } else if (contentType == EContentType::Cartoon) {
        return IVI_CARTOON_CATEGORY_ID;
    }

    return Nothing();
}

TMaybe<int> ContentTypeToId(TEnumSlot<EContentType> contentType) {
    auto type = contentType.GetMaybe();
    return type ? ContentTypeToId(*type) : Nothing();
}

void AddSearchLimits(const TVideoClipsRequest& request, TCgiParameters& cgis) {
    cgis.InsertUnescaped(TStringBuf("from"), ToString(request.From));
    cgis.InsertUnescaped(TStringBuf("to"), ToString(request.To-1)); // do not include right boundary
}

class TIviContentRequestHandle : public TSetupRequest<TVideoGalleryScheme> {
public:
    TIviContentRequestHandle(const TVideoClipsRequest& request, EIviRequestPath requestPath,
                             const TCgiParameters& addCgis, TContext& context)
        : TSetupRequest<TVideoGalleryScheme>(ConstructRequestId("ivi_search", request, addCgis, requestPath))
        , Context(context)
        , RequestPath(requestPath)
        , SearchQuery(request.Slots.BuildSearchQueryForWeb())
        , ContentTypeID(ContentTypeToId(request.Slots.ContentType))
        , Request(request)
        , Cgi(addCgis)
    {
    }

    NHttpFetcher::THandle::TRef Fetch(NHttpFetcher::IMultiRequest::TRef multiRequest) override {
        TStringBuf handler;
        switch(RequestPath) {
            case EIviRequestPath::Catalogue:
                handler = TStringBuf("/catalogue/v5");
                break;
            case EIviRequestPath::CollectionCatalog:
                handler = TStringBuf("/collection/catalog/v5/");
                break;
            case EIviRequestPath::Compilations:
                handler = TStringBuf("/compilations/v5");
                break;
            case EIviRequestPath::Search:
                handler = TStringBuf("/search/v5/");
                break;
            case EIviRequestPath::Videos:
                handler = TStringBuf("/videos/v5");
                break;
        }

        NHttpFetcher::TRequestPtr req =
            AttachProviderRequest(Context, Context.GetSources().VideoIvi(handler), multiRequest);

        req->AddHeader(TStringBuf("X-Forwarded-For"), Context.UserIP());
        req->AddHeader(TStringBuf("X-Request-Id"), Context.ReqId());

        Cgi.InsertUnescaped(TStringBuf("app_version"), IVI_APP_VERSION);

        // IVI cannot filter by category and genre at the same time
        if (!Request.Slots.VideoGenre.Defined() && ContentTypeID.Defined()) {
            Cgi.InsertUnescaped(TStringBuf("category"), ToString(ContentTypeID));
        }

        if ((RequestPath == EIviRequestPath::Catalogue) || (RequestPath == EIviRequestPath::Search)) {
            if (Context.GetContentRestrictionLevel() == EContentRestrictionLevel::Children) {
                for (auto& ageCategory: IVI_FAMILY_SEARCH_CATEGORIES) {
                    Cgi.InsertUnescaped(TStringBuf("age"), ToString(ageCategory));
                }
            }
        }

        AddSearchLimits(Request, Cgi);
        req->AddCgiParams(Cgi);
        return req->Fetch();
    }

    TResultValue Parse(NHttpFetcher::TResponse::TConstRef httpResponse, TVideoGalleryScheme* response, NSc::TValue* /* factorsData */) override {
        if (httpResponse->IsError()) {
            TString err = TStringBuilder() << "ivi response: " << httpResponse->GetErrorText();
            LOG(ERR) << err << Endl;
            return TError(TError::EType::SYSTEM, err);
        }
        NSc::TValue json = NSc::TValue::FromJson(httpResponse->Data);
        const NSc::TValue& result = json["result"];
        if (result.GetArray().empty()) {
            LOG(WARNING) << "ivi response: empty" << Endl;
            return TResultValue();
        }

        /**
         * TODO: Fill here:
         * response->Background16x9();
         */

        /**
         * TODO: Most of the info is NOT available by /search handler, but individually by /videoinfo?id=

         * We'll have to send a bundle of individual requests or use
         * another data source that allows to obtain such data at once
         */
        for (const NSc::TValue& elem : result.GetArray()) {
            if (!CheckContentType(elem))
                continue;

            TVideoItemScheme item = response->Items().Add();
            ParseIviContentItem(Context.GlobalCtx().IviGenres(), elem, item);
        }
        return TResultValue();
    }
private:
    bool CheckContentType(const NSc::TValue& elem) {
        if (!ContentTypeID.Defined()) {
            return true;
        }
        for (auto& elemContentType: elem["categories"].GetArray()) {
            if (elemContentType == ContentTypeID)
                return true;
        }
        return false;
    }

private:
    TContext& Context;
    EIviRequestPath RequestPath;
    TString SearchQuery;
    TMaybe<int> ContentTypeID;
    const TVideoClipsRequest Request;
    TCgiParameters Cgi;
};

class TIviWebSearchHandle : public TWebSearchByProviderHandle {
public:
    TIviWebSearchHandle(const TVideoClipsRequest& request, TContext& context)
        : TWebSearchByProviderHandle(PatchQuery(request.Slots.BuildSearchQueryForWeb()), request, PROVIDER_IVI,
                                     context) {
    }

    static TString PatchQuery(TStringBuf query) {
        return TStringBuilder() << query << TStringBuf(" host:www.ivi.ru");
    }

    bool ParseItemFromDoc(const NSc::TValue doc, TVideoItem* item) override {
        if (!item)
            return false;

        const TStringBuf url = doc["url"].GetString();
        return ParseIviItemFromUrl(url, *item);
    }
};

class TIviRecommendationHandle : public IVideoClipsHandle {
public:
    TIviRecommendationHandle(const TVideoClipsRequest& request, TContext& context)
        : Context(context)
    {
        const TContentTypeFlags type = request.ContentType;
        for (const auto flag : SupportedContentTypes) {
            if (type.HasFlags(flag))
                Handles.push_back(SendRecommendationRequest(request, flag));
        }
    }

    TResultValue WaitAndParseResponse(TVideoGalleryScheme* response) override {
        for (auto& handle : Handles) {
            if (const auto error = ParseResponse(handle, response))
                return error;
        }

        return TResultValue();
    }

private:
    NHttpFetcher::THandle::TRef SendRecommendationRequest(const TVideoClipsRequest& request,
                                                          EContentType contentType) {
        TMaybe<int> contentId = ContentTypeToId(contentType);
        Y_ASSERT(contentId);

        NHttpFetcher::TRequestPtr req =
            AttachProviderRequest(Context, Context.GetSources().VideoIvi(TStringBuf("/hydra/get/recommendation/v5/")),
                                  request.MultiRequest);

        TCgiParameters cgis;
        TIviCredentials::Instance().AddSession(cgis);
        cgis.InsertUnescaped(TStringBuf("top"), ToString(request.To)); // no from/to here, only top is available
        cgis.InsertUnescaped(TStringBuf("scenario_id"), TStringBuf("MAIN_PAGE"));
        cgis.InsertUnescaped(TStringBuf("category"), ToString(*contentId));
        cgis.InsertUnescaped(TStringBuf("app_version"), IVI_APP_VERSION);
        req->AddCgiParams(cgis);

        return req->Fetch();
    }

    TResultValue ParseResponse(NHttpFetcher::THandle::TRef handle, TVideoGalleryScheme* response) {
        if (!handle) {
            TResultValue();
        }
        NHttpFetcher::TResponse::TRef r = handle->Wait();
        NSc::TValue value;
        if (TResultValue error = ParseVideoResponseJson(*r, &value)) {
            return error;
        }

        for (const NSc::TValue& elem : value["result"].GetArray()) {
            TVideoItemScheme item = response->Items().Add();
            ParseIviContentItem(Context.GlobalCtx().IviGenres(), elem, item);
        }
        return TResultValue();
    }

private:
    static constexpr auto SupportedContentTypes = {EContentType::Movie, EContentType::TvShow, EContentType::Cartoon};

    TContext& Context;
    TVector<NHttpFetcher::THandle::TRef> Handles;
};

// FIXME (@a-sidorin): Re-visit IVI handles used for content info retrieval.
EIviRequestPath GetContentHandler(const TVideoClipsRequest& request) {
    bool hasFilm = request.ItemType.HasFlags(EItemType::Movie);
    bool hasSerial = request.ItemType.HasFlags(EItemType::TvShow);

    if (hasFilm == hasSerial) {
        return EIviRequestPath::Catalogue;
    }
    if (hasFilm) {
        return EIviRequestPath::Videos;
    }
    return EIviRequestPath::Compilations;
}

} // namespace

// TIviSourceRequestFactory ----------------------------------------------------
TIviSourceRequestFactory::TIviSourceRequestFactory(TSourcesRequestFactory sources)
    : TProviderSourceRequestFactory(sources, &TSourcesRequestFactory::VideoIvi) {
}

TIviSourceRequestFactory::TIviSourceRequestFactory(TContext& ctx)
    : TProviderSourceRequestFactory(ctx, &TSourcesRequestFactory::VideoIvi) {
}

// TIviClipsProvider -----------------------------------------------------------
std::unique_ptr<IVideoClipsHandle> TIviClipsProvider::MakeSearchRequest(const TVideoClipsRequest& request) const {
    TCgiParameters cgis;
    cgis.InsertUnescaped(TStringBuf("query"), request.GetSearchQuery());
    return FetchSetupRequest<NVideo::TVideoGalleryScheme>(
        MakeHolder<TIviContentRequestHandle>(request, EIviRequestPath::Search, cgis, Context), Context,
        request.MultiRequest);
}

std::unique_ptr<IVideoClipsHandle> TIviClipsProvider::MakeNewVideosRequest(const TVideoClipsRequest& request) const {
    TCgiParameters cgis;
    const TEnumSlot<EContentType> ct = request.Slots.ContentType;
    if (ct.Empty() || (ct == EContentType::Movie)) {
        cgis.InsertUnescaped(TStringBuf("id"), ToString(IVI_NEW_FILMS_CATALOGUE_ID));
    } else if (ct == EContentType::TvShow) {
        cgis.InsertUnescaped(TStringBuf("id"), ToString(IVI_NEW_SERIES_CATALOGUE_ID));
    } else if (ct == EContentType::Cartoon) {
        cgis.InsertUnescaped(TStringBuf("id"), ToString(IVI_NEW_CARTOONS_CATALOGUE_ID));
    }
    cgis.InsertUnescaped(TStringBuf("sort"), TStringBuf("priority_in_collection"));

    return FetchSetupRequest<NVideo::TVideoGalleryScheme>(
        MakeHolder<TIviContentRequestHandle>(request, EIviRequestPath::CollectionCatalog, cgis, Context), Context,
        request.MultiRequest);
}

std::unique_ptr<IVideoClipsHandle> TIviClipsProvider::MakeTopVideosRequest(const TVideoClipsRequest& request) const {
    TCgiParameters cgis;
    cgis.InsertUnescaped(TStringBuf("sort"), TStringBuf("kp"));
    return FetchSetupRequest<NVideo::TVideoGalleryScheme>(
        MakeHolder<TIviContentRequestHandle>(request, GetContentHandler(request), cgis, Context), Context,
        request.MultiRequest);
}

std::unique_ptr<IWebSearchByProviderHandle>
TIviClipsProvider::MakeWebSearchRequest(const TVideoClipsRequest& request) const {
    return FetchSetupRequest<NVideo::TWebSearchByProviderResponse>(MakeHolder<TIviWebSearchHandle>(request, Context),
                                                                   Context, request.MultiRequest);
}

std::unique_ptr<IVideoClipsHandle>
TIviClipsProvider::MakeRecommendationsRequest(const TVideoClipsRequest& request) const {
    return std::make_unique<TIviRecommendationHandle>(request, Context);
}

std::unique_ptr<IVideoClipsHandle>
TIviClipsProvider::MakeVideosByGenreRequest(const TVideoClipsRequest& request) const {
    auto genreIt = IVI_VIDEO_GENRES_MAP.find(request.Slots.VideoGenre.GetEnumValue());
    if (genreIt == IVI_VIDEO_GENRES_MAP.end())
        return MakeDummyRequest<TVideoGalleryScheme>();

    TCgiParameters cgis;
    for (auto& genreId : genreIt->second)
        cgis.InsertUnescaped(TStringBuf("genre"), ToString(genreId));

    return FetchSetupRequest<NVideo::TVideoGalleryScheme>(
        MakeHolder<TIviContentRequestHandle>(request, EIviRequestPath::Catalogue, cgis, Context), Context,
        request.MultiRequest);
}

std::unique_ptr<IRequestHandle<TString>>
TIviClipsProvider::FetchAuthToken(TStringBuf /* passportUid */,
                                  NHttpFetcher::IMultiRequest::TRef /* multiRequest */) const {
    // Token is no longer needed since all the required data is provided by billing.
    return MakeDummyRequest<TString>();
}

IVideoClipsProvider::TPlayResult
TIviClipsProvider::GetPlayCommandDataImpl(TVideoItemConstScheme item, TPlayVideoCommandDataScheme commandData) const {
    if (item.ProviderItemId().Get().empty()) {
        constexpr TStringBuf err = "Empty provider item id";
        LOG(ERR) << err << Endl;
        return TPlayError{EPlayError::VIDEOERROR, err};
    }

    EItemType type = FromString<EItemType>(item.Type());
    if (type != EItemType::Movie && type != EItemType::TvShowEpisode) {
        TString errText = TStringBuilder() << "Unexpected content type " << ToString(type);
        LOG(ERR) << errText << Endl;
        return TPlayError{EPlayError::VIDEOERROR, errText};
    }

    commandData->Uri() = TStringBuilder() << "ivi://" << item.ProviderItemId().Get();
    return {};
}

std::unique_ptr<IVideoItemHandle>
TIviClipsProvider::DoMakeContentInfoRequest(TVideoItemConstScheme item,
                                            NHttpFetcher::IMultiRequest::TRef multiRequest,
                                            bool /* forceUnfiltered */) const {
    TIviContentInfoProvider provider(std::make_unique<TIviSourceRequestFactory>(Context),
                                     Context.GlobalCtx().IviGenres());
    return provider.MakeContentInfoRequest(item, multiRequest);
}

TResultValue TIviClipsProvider::DoGetSerialDescriptor(TVideoItemConstScheme tvShowItem,
                                                      TSerialDescriptor* serial) const {
    TIviContentInfoProvider provider(std::make_unique<TIviSourceRequestFactory>(Context), Context.GlobalCtx().IviGenres());

    auto multiRequest = NHttpFetcher::WeakMultiRequest();
    Y_ASSERT(multiRequest);

    const auto request = provider.MakeSerialDescriptorRequest(tvShowItem, multiRequest);
    Y_ASSERT(request);
    if (const auto error = request->WaitAndParseResponse(*serial))
        return TError{TError::EType::VIDEOERROR, error->Msg};
    return {};
}

std::unique_ptr<ISeasonDescriptorHandle>
TIviClipsProvider::DoMakeSeasonDescriptorRequest(const TSerialDescriptor& serialDescr,
                                                 const TSeasonDescriptor& seasonDescr,
                                                 NHttpFetcher::IMultiRequest::TRef multiRequest) const {
    TIviContentInfoProvider provider(std::make_unique<TIviSourceRequestFactory>(Context), Context.GlobalCtx().IviGenres());
    return provider.MakeSeasonDescriptorRequest(serialDescr, seasonDescr, multiRequest);
}

} // namespace NVideo
} // namespace NBass
