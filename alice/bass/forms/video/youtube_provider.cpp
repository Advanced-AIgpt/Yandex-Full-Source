#include "youtube_provider.h"

#include "defs.h"
#include "utils.h"

#include <alice/bass/libs/logging_v2/logger.h>
#include <alice/bass/libs/socialism/socialism.h>
#include <alice/bass/libs/source_request/handle.h>
#include <alice/bass/libs/video_common/video.sc.h>
#include <alice/bass/libs/video_common/youtube_utils.h>

#include <alice/bass/setup/setup.h>

#include <library/cpp/timezone_conversion/civil.h>

#include <util/string/builder.h>
#include <library/cpp/cgiparam/cgiparam.h>

using namespace NVideoCommon;

namespace NBASS {
namespace NVideo {
namespace {
    constexpr TStringBuf HOST_YOUTUBE = "www.youtube.com";

    TString MakeYoutubeUrl(TStringBuf id, TContext& context) {
        // TODO: yulika@, vi002@: Do we need to do smth with devices, that do not support video at all?
        if (context.MetaClientInfo().IsSmartSpeaker()) {
            return TStringBuilder() << "youtube://" << id;
        }
        return TStringBuilder() << "https://www.youtube.com/watch?v=" << id;
    }

    class TYouTubeSearchHandle : public TSetupRequest<TVideoGalleryScheme> {
    public:
        TYouTubeSearchHandle(const TVideoClipsRequest& request, TCgiParameters cgi, TContext& context)
            : TSetupRequest<TVideoGalleryScheme>("youtube_search")
            , Request(request)
            , Context(context)
            , Cgi(cgi)
        {
        }

        NHttpFetcher::THandle::TRef Fetch(NHttpFetcher::IMultiRequest::TRef multiRequest) override {
            NHttpFetcher::TRequestPtr req = AttachProviderRequest(Context, Context.GetSources().VideoYouTube(TStringBuf("/search/")), multiRequest);

            GetYouTubeCredentials().AddGoogleAPIsKey(Cgi);
            Cgi.InsertUnescaped(TStringBuf("q"), Request.Slots.BuildSearchQueryForInternetVideos());
            Cgi.InsertUnescaped(TStringBuf("part"), TStringBuf("snippet"));
            Cgi.InsertUnescaped(TStringBuf("maxResults"), ToString(Request.To));
            Cgi.InsertUnescaped(TStringBuf("type"), TStringBuf("video"));

            req->AddCgiParams(Cgi);

            return req->Fetch();
        }

        TResultValue Parse(NHttpFetcher::TResponse::TConstRef resp, TVideoGalleryScheme* response, NSc::TValue* /* factorsData */) override {
            if (resp->IsError()) {
                TString err = TStringBuilder() << "top response: " << resp->GetErrorText();
                LOG(ERR) << err << Endl;
                return TError(TError::EType::SYSTEM, err);
            }

            const NSc::TValue json = NSc::TValue::FromJson(resp->Data);
            if (json.IsNull() || !json.IsDict()) {
                LOG(WARNING) << "yandex video response: no answer or not a dict" << Endl;
                return TResultValue();
            }

            for (const auto& elem : json["items"].GetArray()) {
                TVideoItemScheme item = response->Items().Add();

                item.Name() = elem["snippet"]["title"].GetString();
                item.ProviderItemId() = elem["id"]["videoId"].GetString();

                item.ProviderName() = PROVIDER_YOUTUBE;
                item.SourceHost() = HOST_YOUTUBE;
                item.Type() = ToString(EItemType::Video);

                item.ThumbnailUrl16X9() = elem["snippet"]["thumbnails"]["high"]["url"].GetString();
                item.ThumbnailUrl16X9Small() = elem["snippet"]["thumbnails"]["high"]["url"].GetString();

                const auto when = TInstant::ParseIso8601(elem["snippet"]["publishedAt"]);
                NDatetime::TCivilSecond cs = NDatetime::Convert(when, "UTC");
                item.ReleaseYear() = cs.year();

                item.Available() = true;
            }
            return TResultValue();
        }

    private:
        const TVideoClipsRequest Request;
        TContext& Context;
        TCgiParameters Cgi;
    };

    class TTopClipsHandle : public IVideoClipsHandle {
    public:
        TTopClipsHandle(const TVideoClipsRequest& request, TContext& context, const TStringBuf authToken)
            : Request(request)
            , Context(context)
        {
            NHttpFetcher::TRequestPtr req = AttachProviderRequest(Context, Context.GetSources().VideoYouTube(TStringBuf("/videos/")), request.MultiRequest);

            TCgiParameters cgi;
            GetYouTubeCredentials().AddGoogleAPIsKey(cgi);
            cgi.InsertUnescaped(TStringBuf("chart"), TStringBuf("mostPopular"));
            cgi.InsertUnescaped(TStringBuf("regionCode"), TStringBuf("RU"));
            cgi.InsertUnescaped(TStringBuf("part"), TStringBuf("snippet,contentDetails,statistics"));
            cgi.InsertUnescaped(TStringBuf("maxResults"), ToString(Request.To));

            if (context.HasExpFlag(NAlice::NVideoCommon::FLAG_ENABLE_YOUTUBE_USER_TOKEN) && !authToken.empty()) {
                TStringBuilder bearer;
                bearer << "Bearer " << authToken;
                req->AddHeader(TStringBuf("Authorization"), bearer);
            }

            req->AddCgiParams(cgi);

            Handle = req->Fetch();
        }

        TResultValue WaitAndParseResponse(TVideoGalleryScheme* response) override {
            NHttpFetcher::TResponse::TRef resp = Handle->Wait();
            if (resp->IsError()) {
                TString err = TStringBuilder() << "top response: " << resp->GetErrorText();
                LOG(ERR) << err << Endl;
                return TError(TError::EType::SYSTEM, err);
            }

            const NSc::TValue json = NSc::TValue::FromJson(resp->Data);
            if (json.IsNull() || !json.IsDict()) {
                LOG(WARNING) << "youtube response: no answer or not a dict" << Endl;
                return TResultValue();
            }

            for (const auto& elem : json["items"].GetArray()) {
                if (TMaybe<TVideoItem> item = TryParseYouTubeNode(elem))
                    response->Items().Add() = item->Scheme();
            }
            return TResultValue();
        }

    private:
        const TVideoClipsRequest Request;
        TContext& Context;
        NHttpFetcher::THandle::TRef Handle;
    };

} // namespace

std::unique_ptr<IVideoClipsHandle> TYouTubeClipsProvider::MakeSearchRequest(const TVideoClipsRequest& request) const {
    return FetchSetupRequest<NVideo::TVideoGalleryScheme>(
        MakeHolder<TYouTubeSearchHandle>(request, TCgiParameters(), Context), Context, request.MultiRequest);
}

std::unique_ptr<IVideoClipsHandle>
TYouTubeClipsProvider::MakeNewVideosRequest(const TVideoClipsRequest& request) const {
    return std::make_unique<TTopClipsHandle>(request, Context, GetAuthToken());
}

std::unique_ptr<IVideoClipsHandle>
TYouTubeClipsProvider::MakeRecommendationsRequest(const TVideoClipsRequest& request) const {
    return std::make_unique<TTopClipsHandle>(request, Context, GetAuthToken());
}

std::unique_ptr<IVideoClipsHandle>
TYouTubeClipsProvider::MakeTopVideosRequest(const TVideoClipsRequest& request) const {
    return std::make_unique<TTopClipsHandle>(request, Context, GetAuthToken());
}

IVideoClipsProvider::TPlayResult
TYouTubeClipsProvider::GetPlayCommandDataImpl(TVideoItemConstScheme item,
                                              TPlayVideoCommandDataScheme commandData) const {
    if (item.ProviderItemId().Get().empty()) {
        return TPlayError{EPlayError::VIDEOERROR, "Empty provider_item_id"};
    }
    commandData->Uri() = MakeYoutubeUrl(item.ProviderItemId(), Context);
    return {};
}

std::unique_ptr<IRequestHandle<TString>>
TYouTubeClipsProvider::FetchAuthToken(TStringBuf passportUid, NHttpFetcher::IMultiRequest::TRef multiRequest) const {
    if (Context.HasExpFlag(NAlice::NVideoCommon::FLAG_ENABLE_YOUTUBE_USER_TOKEN)) {
        return RequestToken(Context, multiRequest, passportUid, TStringBuf("google-youtube-yandex-bass"));
    }
    return std::make_unique<TDummyRequestHandle<TString>>();
}

std::unique_ptr<IVideoItemHandle>
TYouTubeClipsProvider::DoMakeContentInfoRequest(TVideoItemConstScheme item,
                                                NHttpFetcher::IMultiRequest::TRef multiRequest,
                                                bool /* forceUnfiltered */) const {
    if (!Context.HasExpFlag(NAlice::NVideoCommon::FLAG_VIDEO_ENABLE_YOUTUBE_CONTENT_INFO))
        return TVideoClipsHttpProviderBase::DoMakeContentInfoRequest(item, multiRequest); // Make a dummy request.

    return std::make_unique<TYouTubeContentRequestHandle>(
        Context.GetSources().VideoYouTube("/videos/"), multiRequest, item->ProviderItemId(),
        Context.HasExpFlag(NAlice::NVideoCommon::FLAG_ENABLE_YOUTUBE_USER_TOKEN), GetAuthToken());
}

} // namespace NVideo
} // namespace NBASS
