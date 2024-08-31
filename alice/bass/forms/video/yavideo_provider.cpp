#include "yavideo_provider.h"

#include "defs.h"

#include <alice/bass/forms/video/entity_search.h>
#include <alice/bass/forms/video/utils.h>
#include <alice/bass/libs/logging_v2/logger.h>
#include <alice/bass/libs/video_common/video.sc.h>
#include <alice/library/network/headers.h>
#include <alice/library/util/search_convert.h>

#include <library/cpp/string_utils/url/url.h>
#include <library/cpp/timezone_conversion/civil.h>

#include <util/string/builder.h>
#include <util/string/cast.h>
#include <library/cpp/cgiparam/cgiparam.h>
#include <library/cpp/string_utils/quote/quote.h>
#include <util/string/split.h>
#include <util/string/subst.h>
#include <util/string/vector.h>

using namespace NAlice::NVideoCommon;
using namespace NVideoCommon;

namespace NBASS {
namespace NVideo {

namespace {

    bool ShouldRestrictRequestToYoutube(const TVideoClipsRequest& request) {
        return request.Slots.OriginalProvider.GetString() == PROVIDER_YOUTUBE;
    }

    void SetRequestParamsForYavideoSearch(TContext& context, const NHttpFetcher::TRequestPtr& req) {
        req->AddCgiParam(TStringBuf("disable_renderer"), TStringBuf("1")); // get raw json response
        req->AddCgiParam(TStringBuf("ipreg"), ToString(context.UserRegion()));
        TCgiParameters pars = GetRequiredVideoCgiParams(context);
        req->AddCgiParams(pars);
        req->AddHeader(NAlice::NNetwork::HEADER_X_YANDEX_APP_INFO, context.GetAppInfoHeader());
        req->AddHeader(NAlice::NNetwork::HEADER_USER_AGENT, context.Meta().RawUserAgent());

        if (context.HasExpFlag(NAlice::NVideoCommon::FLAG_VIDEO_DISABLE_OAUTH) && context.UserTicket().Defined()) {
            req->AddHeader(NAlice::NNetwork::HEADER_X_YA_USER_TICKET, *context.UserTicket());
        } else if (context.IsAuthorizedUser()) {
            req->AddHeader("Authorization", context.UserAuthorizationHeader());
        }

        req->AddHeader(NAlice::NNetwork::HEADER_X_YANDEX_INTERNAL_REQUEST, "1");
        if (context.HasExpFlag(NAlice::NExperiments::TUNNELLER_ANALYTICS_INFO)) {
            req->AddCgiParams(NAlice::NAnalyticsInfo::ConstructVideoSearchRequestCgiParameters(
                    context.ExpFlag(NAlice::NExperiments::TUNNELLER_PROFILE_VIDEO)));
        }
    }

    NVideoCommon::TResult FillEntityGalleryWithYavideoSearchResponse(const NSc::TValue& jsonData,
                                                                        NBASS::TContext& ctx,
                                                                        NVideoCommon::TVideoGalleryScheme* response,
                                                                        size_t checkTopOrganicResultsForKp,
                                                                        size_t checkReducedTopOrganicResultsForKp,
                                                                        size_t checkChainedTopOrganicResultsForKp,
                                                                        size_t checkScaledTopOrganicResultsForKp) {
        THashMap<TString, TString> kpidToEntref;
        THashMap<TString, TString> kpidToHorizontalPoster;
        THashMap<TString, NSc::TValue> kpidToLicenses;
        TVector<TString> allKpIds = GetIdsFromEntitySearchResponse(jsonData, kpidToEntref, kpidToHorizontalPoster, kpidToLicenses,
            checkTopOrganicResultsForKp, checkReducedTopOrganicResultsForKp, checkChainedTopOrganicResultsForKp,
            checkScaledTopOrganicResultsForKp, ctx.HasExpFlag(FLAG_VIDEO_ADD_MAIN_OBJECT_FROM_YAVIDEO) || ctx.MetaClientInfo().IsLegatus());
        if (allKpIds.empty()) {
            TStringBuf err = "No valid kinopoisk ids in entity search response.";
            LOG(WARNING) << err << Endl;
            return NVideoCommon::TError{err};
        }
        TVector<TVideoItem> videoItems;
        if (!TryGetVideoItemsFromYdbByKinopoiskIds(ctx, allKpIds, videoItems)) {
            TStringBuf err = "Incorrect result of GetVideoItemsListFromYdb.";
            LOG(WARNING) << err << Endl;
            return NVideoCommon::TError{err};
        }
        FillEntrefsByKpids(videoItems, kpidToEntref);
        FillHorizontalPostersByKpids(videoItems, kpidToHorizontalPoster);
        FillLicensesByKpids(videoItems, kpidToLicenses);

        SetItemsSource(videoItems, VIDEO_SOURCE_YAVIDEO);
        for (NVideo::TVideoItem &item : videoItems) {
            response->Items().Add().GetRawValue()->Swap(item.Value());
        }
        const TString entrefPath = "searchdata/scheme/entity_data/parent_collection/path/0/entref";
        if (!response->Items().Empty()) {
            response->Entref() = jsonData.TrySelect(entrefPath).GetString();
        }

        return TResult();
    }

    class TYaVideoSearchHandle : public TSetupRequest<TVideoGalleryScheme> {
    public:
        TYaVideoSearchHandle(const TVideoClipsRequest& request, TCgiParameters cgi, TContext& context)
            : TSetupRequest<TVideoGalleryScheme>(ConstructRequestId("yavideo_search", request, cgi))
            , Request(request)
            , Context(context)
            , Cgi(cgi)
        {
        }

        NHttpFetcher::THandle::TRef Fetch(NHttpFetcher::IMultiRequest::TRef multiRequest) override {
            NHttpFetcher::TRequestPtr req;
            if (!Context.HasExpFlag(FLAG_VIDEO_DISABLE_KNOSS_BALANCER)) {
                req = AttachProviderRequest(Context, Context.GetSources().VideoYandexSearch(), multiRequest);
            } else {
                req = AttachProviderRequest(Context, Context.GetSources().VideoYandexSearchOld(), multiRequest);
            }
            SetRequestParamsForYavideoSearch(Context, req);

            const auto& slots = Request.Slots;

            TStringBuilder text;
            text << slots.BuildSearchQueryForInternetVideos();

            if (ShouldRestrictRequestToYoutube(Request)) {
                text << TStringBuf(" host:www.youtube.com");
                if (slots.ContentType == NVideo::EContentType::Cartoon) {
                    Cgi.InsertUnescaped(TStringBuf("wizextra"), TStringBuf("video_replace_skip"));
                }
            } else {
                if (!Context.HasExpFlag(FLAG_VIDEO_ENABLE_ALL_HOSTS)) {
                    Cgi.InsertUnescaped(TStringBuf("rearr"), TStringBuf("forced_player_device=quasar"));

                    if (Context.HasExpFlag(FLAG_VIDEO_ENABLE_VH_SEARCH)) {
                        Cgi.InsertUnescaped("relev", "unban_vh_quasar=true");
                    } else {
                        // https://st.yandex-team.ru/VIDEOPOISK-11074#1533230168000
                        Cgi.InsertUnescaped("rearr", "scheme_Local/VideoSnippets/ReplaceVhDups=0");
                    }
                    Cgi.InsertUnescaped("gta", "qproxy");
                    Cgi.InsertUnescaped("rearr", "forced_qproxy_player=1");
                }

                if (slots.ProviderWasChanged &&
                    slots.OriginalProvider.Defined() &&
                    slots.OriginalProvider.GetString() != PROVIDER_YAVIDEO)
                {
                    text << " " << slots.OriginalProvider.GetString();
                }
            }

            if (Context.HasExpFlag(FLAG_VIDEO_REARR_ADD_DEVICE_ZERO)) {
                Cgi.InsertUnescaped(TStringBuf("rearr"), TStringBuf("scheme_Local/VideoPlayers/AddDevice=0"));
            }

            Cgi.InsertUnescaped(TStringBuf("assistant"), TStringBuf("1"));
            Cgi.InsertUnescaped(TStringBuf("g"), TStringBuilder() << TStringBuf("1.dg.") << Request.To << TStringBuf(".1.-1"));
            Cgi.InsertUnescaped(TStringBuf("text"), text);
            Cgi.InsertUnescaped(TStringBuf("noredirect"), TStringBuf("1"));

            DebugRequestText = text;

            AddYaVideoAgeFilterParam(Context, Cgi);

            req->AddCgiParams(Cgi);
            TString url = ToString(req->Url());
            DebugUrl = ToString(GetPathAndQuery(url));
            RequestSourceBuilder = MakeAnalyticsInfoVideoRequestSourceEvent(Context, DebugUrl, DebugRequestText);

            return req->Fetch();
        }

        TResultValue Parse(NHttpFetcher::TResponse::TConstRef httpResponse, TVideoGalleryScheme* response,
                           NSc::TValue* /* factorsData */) override
        {
            if (httpResponse->IsError()) {
                TString err = TStringBuilder() << "yavideo response: " << httpResponse->GetErrorText();
                LOG(ERR) << err << Endl;
                bool isHttpError = httpResponse->Result == NHttpFetcher::TResponse::EResult::HttpError;
                AddResponseToAnalyticsInfoVideoRequestSourceEvent(
                    RequestSourceBuilder,
                    isHttpError ? httpResponse->Code : httpResponse->GetSystemErrorCode(),
                    isHttpError
                );
                return TError(TError::EType::VIDEOERROR, err);
            }

            TContextWrapper contextWrapper(Context);

            AddResponseToAnalyticsInfoVideoRequestSourceEvent(
                RequestSourceBuilder,
                httpResponse->Code,
                true // httpResponse->Result == EResult::Ok
            );

            NSc::TValue responseJson;
            if (!NSc::TValue::FromJson(responseJson, httpResponse->Data)) {
                TStringBuf err = "Can not convert JSON to NSc::TValue.";
                LOG(ERR) << err << Endl;
                return TError(TError::EType::VIDEOERROR, err);
            }

            if (const auto error = ParseJsonResponse(responseJson, contextWrapper, response,
                                                     VIDEO_SOURCE_YAVIDEO)) {
                return TError(TError::EType::VIDEOERROR, error->Msg);
            }

            response->DebugInfo().YaVideoRequest() = DebugRequestText;
            response->DebugInfo().Url() = DebugUrl;
            return TResultValue();
        }

    private:
        const TVideoClipsRequest Request;
        TContext& Context;
        TCgiParameters Cgi;
        TString DebugRequestText;
        TString DebugUrl;
        THolder<NAlice::NScenarios::IAnalyticsInfoBuilder::IVideoRequestSourceBuilder> RequestSourceBuilder;
    };

    class TTopClipsHandle : public IVideoClipsHandle {
    public:
        TTopClipsHandle(const TVideoClipsRequest& request, TContext& context)
            : Request(request)
            , Context(context)
        {
            NHttpFetcher::TRequestPtr videoRequest;
            if (!Context.HasExpFlag(FLAG_VIDEO_DISABLE_KNOSS_BALANCER)) {
                videoRequest = AttachProviderRequest(Context, Context.GetSources().VideoYandexSearch(), request.MultiRequest);
            } else {
                videoRequest = AttachProviderRequest(Context, Context.GetSources().VideoYandexSearchOld(), request.MultiRequest);
            }

            TCgiParameters cgi;
            AddYaVideoAgeFilterParam(Context, cgi);

            auto insertRearrParam = [&cgi](const TString& value) {
                cgi.InsertUnescaped("rearr", value);
            };
            insertRearrParam("forced_player_device=quasar");
            insertRearrParam("scheme_Local/VideoSnippets/ReplaceVhDups=0");
            insertRearrParam("forced_qproxy_player=1");
            cgi.InsertUnescaped(TStringBuf("text"), TStringBuf("видео"));
            SetRequestParamsForYavideoSearch(Context, videoRequest);

            videoRequest->AddCgiParams(cgi);
            TString url = ToString(videoRequest->Url());
            DebugUrl = ToString(GetPathAndQuery(url));
            DebugRequestText = "видео";
            RequestSourceBuilder = MakeAnalyticsInfoVideoRequestSourceEvent(Context, DebugUrl, DebugRequestText);

            Handle = videoRequest->Fetch();
        }

        TResultValue WaitAndParseResponse(TVideoGalleryScheme* response) override {
            NHttpFetcher::TResponse::TRef resp = Handle->Wait();
            if (resp->IsError()) {
                TString err = TStringBuilder() << "top response: " << resp->GetErrorText();
                LOG(ERR) << err << Endl;
                bool isHttpError = resp->Result == NHttpFetcher::TResponse::EResult::HttpError;
                AddResponseToAnalyticsInfoVideoRequestSourceEvent(
                    RequestSourceBuilder,
                    isHttpError ? resp->Code : resp->GetSystemErrorCode(),
                    isHttpError
                );
                return TError(TError::EType::SYSTEM, err);
            }

            AddResponseToAnalyticsInfoVideoRequestSourceEvent(
                RequestSourceBuilder,
                resp->Code,
                true // resp->Result == EResult::Ok
            );
            const NSc::TValue json = NSc::TValue::FromJson(resp->Data);
            if (json.IsNull() || !json.IsDict()) {
                LOG(ERR) << "yandex video response: no answer or not a dict: " << resp->Data << Endl;
                return TResultValue();
            }

            TContextWrapper contextWrapper(Context);
            if (const auto error = ParseJsonResponse(json, contextWrapper, response,
                                                         VIDEO_SOURCE_YAVIDEO_TOUCH)) {
                return TError(TError::EType::VIDEOERROR, error->Msg);
            }

            LOG(INFO) << "VideoSearch reqid=" << json["searchdata"]["reqid"] << Endl;
            return TResultValue();
        }

    private:
        const TVideoClipsRequest Request;
        TContext& Context;
        TString DebugRequestText;
        TString DebugUrl;
        NHttpFetcher::THandle::TRef Handle;
        THolder<NAlice::NScenarios::IAnalyticsInfoBuilder::IVideoRequestSourceBuilder> RequestSourceBuilder;
    };

    static const THashMap<TStringBuf, size_t> DEFAULT_FLAG_VALUES = {
            {FLAG_VIDEO_CHECK_TOP_ORGANIC_RESULTS_FOR_KP, 3},
            {FLAG_VIDEO_CHECK_REDUCED_TOP_ORGANIC_RESULTS_FOR_KP, 15},
            {FLAG_VIDEO_CHECK_CHAINED_TOP_ORGANIC_RESULTS_FOR_KP, 0},
            {FLAG_VIDEO_CHECK_SCALED_TOP_ORGANIC_RESULTS_FOR_KP, 0}
    };
    class TRecommendationsHandle : public IVideoClipsHandle {
    public:
        TRecommendationsHandle(const TVideoClipsRequest& request, TContext& context)
            : Request(request)
            , Context(context)
            , CheckTopOrganicResultsForKp(GetFlagForCheckingTopOrganicResultsForKP(context, FLAG_VIDEO_CHECK_TOP_ORGANIC_RESULTS_FOR_KP))
            , CheckReducedTopOrganicResultsForKp(GetFlagForCheckingTopOrganicResultsForKP(context, FLAG_VIDEO_CHECK_REDUCED_TOP_ORGANIC_RESULTS_FOR_KP))
            , CheckChainedTopOrganicResultsForKp(GetFlagForCheckingTopOrganicResultsForKP(context, FLAG_VIDEO_CHECK_CHAINED_TOP_ORGANIC_RESULTS_FOR_KP))
            , CheckScaledTopOrganicResultsForKp(GetFlagForCheckingTopOrganicResultsForKP(context, FLAG_VIDEO_CHECK_SCALED_TOP_ORGANIC_RESULTS_FOR_KP))
        {
            NHttpFetcher::TRequestPtr videoRequest;
            if (!Context.HasExpFlag(FLAG_VIDEO_DISABLE_KNOSS_BALANCER)) {
                videoRequest = AttachProviderRequest(Context, Context.GetSources().VideoYandexRecommendation(), request.MultiRequest);
            } else {
                videoRequest = AttachProviderRequest(Context, Context.GetSources().VideoYandexSearchOld(), request.MultiRequest);
            }

            TCgiParameters cgi;
            AddYaVideoAgeFilterParam(Context, cgi);

            SetRequestParamsForYavideoSearch(Context, videoRequest);

            TStringBuilder requestText;
            requestText << Request.Slots.BuildSearchQueryForInternetVideos();
            if (requestText.empty()) {
                requestText << (isFilmsRequest() ? "что посмотреть" : "видео");
            }
            if (Request.Slots.OriginalProvider.Defined() &&
                Request.Slots.OriginalProvider.GetString() != PROVIDER_YAVIDEO &&
                Request.Slots.OriginalProvider.GetString() != PROVIDER_KINOPOISK)
                // ES is working bad with "xxx kinopoisk"
            {
                requestText << " " << Request.Slots.OriginalProvider.GetString();
            }

            cgi.InsertUnescaped(TStringBuf("text"), requestText);
            if (CheckTopOrganicResultsForKp) {
                cgi.InsertUnescaped(TStringBuf("unban_hosts"), "1");
                cgi.InsertUnescaped("rearr", "scheme_Local/FilterBannedVideo/TvodForAll=1");
            }

            if (Context.MetaClientInfo().IsLegatus()) {
                cgi.InsertUnescaped(TStringBuf("client"), "legatus");
            }

            videoRequest->AddCgiParams(cgi);

            TString url = ToString(videoRequest->Url());
            DebugUrl = ToString(GetPathAndQuery(url));
            DebugRequestText = requestText;
            RequestSourceBuilder = MakeAnalyticsInfoVideoRequestSourceEvent(Context, DebugUrl, DebugRequestText);
            Handle = videoRequest->Fetch();
        }

        TResultValue WaitAndParseResponse(TVideoGalleryScheme* response) override {
            NHttpFetcher::TResponse::TRef resp = Handle->Wait();
            if (resp->IsError()) {
                TString err = TStringBuilder() << "recommendations response: " << resp->GetErrorText();
                LOG(ERR) << err << Endl;
                bool isHttpError = resp->Result == NHttpFetcher::TResponse::EResult::HttpError;
                AddResponseToAnalyticsInfoVideoRequestSourceEvent(
                    RequestSourceBuilder,
                    isHttpError ? resp->Code : resp->GetSystemErrorCode(),
                    isHttpError
                );
                return TError(TError::EType::SYSTEM, err);
            }

            AddResponseToAnalyticsInfoVideoRequestSourceEvent(
                RequestSourceBuilder,
                resp->Code,
                true // resp->Result == EResult::Ok
            );
            const NSc::TValue json = NSc::TValue::FromJson(resp->Data);
            if (json.IsNull() || !json.IsDict()) {
                LOG(ERR) << "yandex video recommendation response: no answer or not a dict: " << resp->Data << Endl;
                return TResultValue();
            }
            LOG(INFO) << "VideoRecomendations reqid=" << json["searchdata"]["reqid"] << Endl;

            if (isFilmsRequest()) {
                if (const auto error = FillEntityGalleryWithYavideoSearchResponse(json, Context, response, CheckTopOrganicResultsForKp,
                                                                                  CheckReducedTopOrganicResultsForKp, CheckChainedTopOrganicResultsForKp,
                                                                                  CheckScaledTopOrganicResultsForKp)) {
                    return TError(TError::EType::VIDEOERROR, error->Msg);
                }
            } else {
                TContextWrapper contextWrapper(Context);
                if (const auto error = ParseJsonResponse(json, contextWrapper, response,
                                                             VIDEO_SOURCE_YAVIDEO_TOUCH)) {
                    return TError(TError::EType::VIDEOERROR, error->Msg);
                }
            }

            return TResultValue();
        }

    private:

        bool isFilmsRequest() const {
            return Request.Slots.FixedProvider.Empty()
                || Request.Slots.FixedProvider == NVideoCommon::PROVIDER_KINOPOISK;
        }

        size_t GetFlagForCheckingTopOrganicResultsForKP(const TContext& context, const TStringBuf& flagName) {
            size_t flagDefaultValue = 0; // default value
            if (DEFAULT_FLAG_VALUES.contains(flagName)) {
                flagDefaultValue = DEFAULT_FLAG_VALUES.at(flagName);
            }
            if (const auto flagValue = context.ExpFlag(flagName); flagValue.Defined()) {
                return FromStringWithDefault(flagValue.GetRef(), flagDefaultValue);
            }
            return flagDefaultValue;
        }

        const TVideoClipsRequest Request;
        TContext& Context;
        TString DebugRequestText;
        TString DebugUrl;
        NHttpFetcher::THandle::TRef Handle;
        THolder<NAlice::NScenarios::IAnalyticsInfoBuilder::IVideoRequestSourceBuilder> RequestSourceBuilder;
        size_t CheckTopOrganicResultsForKp;
        size_t CheckReducedTopOrganicResultsForKp;
        size_t CheckChainedTopOrganicResultsForKp;
        size_t CheckScaledTopOrganicResultsForKp;
    };

} // namespace anonymous

NHttpFetcher::TRequestPtr TContextWrapper::AttachProviderRequest(NHttpFetcher::IMultiRequest::TRef multiRequest) const {
    NHttpFetcher::TRequestPtr request;
    if (!Context.HasExpFlag(FLAG_VIDEO_DISABLE_KNOSS_BALANCER)) {
        request = NBASS::NVideo::AttachProviderRequest(Context, Context.GetSources().VideoYandexSearch(), multiRequest);
    } else {
        request = NBASS::NVideo::AttachProviderRequest(Context, Context.GetSources().VideoYandexSearchOld(), multiRequest);
    }
    SetRequestParamsForYavideoSearch(Context, request);
    return request;
}

std::unique_ptr<IVideoItemHandle>
TYaVideoClipsProvider::DoMakeContentInfoRequest(TVideoItemConstScheme item,
                                                NHttpFetcher::IMultiRequest::TRef multiRequest,
                                                bool forceUnfiltered) const {
    TSimpleSharedPtr<TYaVideoContentGetterDelegate> contentGetterDelegate =
        MakeSimpleShared<TContextWrapper>(Context, forceUnfiltered);
    return std::make_unique<TContentInfoHandle>(contentGetterDelegate, item, multiRequest);
}

std::unique_ptr<IVideoClipsHandle> TYaVideoClipsProvider::MakeSearchRequest(const TVideoClipsRequest& request) const {
    return FetchSetupRequest<NVideo::TVideoGalleryScheme>(
        MakeHolder<TYaVideoSearchHandle>(request, TCgiParameters(), Context), Context, request.MultiRequest);
}

std::unique_ptr<IVideoClipsHandle>
TYaVideoClipsProvider::MakeNewVideosRequest(const TVideoClipsRequest& request) const {
    TCgiParameters cgi;
    cgi.InsertUnescaped(TStringBuf("within"), TStringBuf("9"));
    return FetchSetupRequest<NVideo::TVideoGalleryScheme>(
        MakeHolder<TYaVideoSearchHandle>(request, cgi, Context), Context, request.MultiRequest);
}

std::unique_ptr<IVideoClipsHandle>
TYaVideoClipsProvider::MakeRecommendationsRequest(const TVideoClipsRequest& request) const {
    if (!Context.HasExpFlag(FLAG_VIDEO_DISABLE_YAVIDEO_RECOMMENDATIONS)) {
        return std::make_unique<TRecommendationsHandle>(request, Context);
    }
    return std::make_unique<TTopClipsHandle>(request, Context);
}

std::unique_ptr<IVideoClipsHandle>
TYaVideoClipsProvider::MakeTopVideosRequest(const TVideoClipsRequest& request) const {
    return std::make_unique<TTopClipsHandle>(request, Context);
}

std::unique_ptr<IVideoClipsHandle>
TYaVideoClipsProvider::MakeVideosByGenreRequest(const TVideoClipsRequest& request) const {
    return FetchSetupRequest<NVideo::TVideoGalleryScheme>(
        MakeHolder<TYaVideoSearchHandle>(request, TCgiParameters(), Context), Context, request.MultiRequest);
}

IVideoClipsProvider::TPlayResult
TYaVideoClipsProvider::GetPlayCommandDataImpl(TVideoItemConstScheme item,
                                              TPlayVideoCommandDataScheme commandData) const {
    bool omitRestrictionByExperiment = Context.HasExpFlag(TStringBuf("video_omit_youtube_restriction"));
    if (!omitRestrictionByExperiment && item.ProviderName() != PROVIDER_YOUTUBE) {
        return TPlayError{EPlayError::VIDEOERROR, TStringBuilder() << "Unsupported host " << item.SourceHost()};
    }
    if (item.ProviderItemId().Get().empty()) {
        return TPlayError{EPlayError::VIDEOERROR, "Empty provider_item_id"};
    }

    if (item.ProviderName() == PROVIDER_YOUTUBE) {
        TContextWrapper contextWrapper(Context);
        commandData->Uri() = MakeYoutubeUrl(item.ProviderItemId(), contextWrapper);
        return {};
    } else if (item.HasPlayUri()) {
        commandData->Uri() = item.PlayUri();
        return {};
    }

    TStringBuf err = "Could not play video item without youtube id or play_uri";
    LOG(ERR) << err << Endl;
    return TPlayError{EPlayError::VIDEOERROR, err};
}

} // namespace NVideo
} // namespace NBASS
