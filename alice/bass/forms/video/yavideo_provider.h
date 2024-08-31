#pragma once

#include "utils.h"
#include "video_provider.h"

#include <alice/bass/libs/video_common/yavideo_utils.h>
#include <alice/library/analytics/common/utils.h>
#include <alice/library/analytics/common/names.h>
#include <alice/library/experiments/flags.h>

namespace NBASS {
namespace NVideo {

class TYaVideoClipsProvider : public TVideoClipsHttpProviderBase {
public:
    explicit TYaVideoClipsProvider(TContext& context)
        : TVideoClipsHttpProviderBase(context)
    {
    }

    // IVideoClipsProvider overrides:
    std::unique_ptr<IVideoItemHandle> DoMakeContentInfoRequest(TVideoItemConstScheme item,
                                                               NHttpFetcher::IMultiRequest::TRef multiRequest,
                                                               bool forceUnfiltered = false) const override;
    std::unique_ptr<IVideoClipsHandle> MakeSearchRequest(const TVideoClipsRequest& request) const override;
    std::unique_ptr<IVideoClipsHandle> MakeNewVideosRequest(const TVideoClipsRequest& request) const override;
    std::unique_ptr<IVideoClipsHandle> MakeRecommendationsRequest(const TVideoClipsRequest& request) const override;
    std::unique_ptr<IVideoClipsHandle> MakeTopVideosRequest(const TVideoClipsRequest& request) const override;
    std::unique_ptr<IVideoClipsHandle> MakeVideosByGenreRequest(const TVideoClipsRequest& request) const override;

    TStringBuf GetProviderName() const override {
        return NVideoCommon::PROVIDER_YAVIDEO;
    }

    bool IsUnauthorized() const override {
        return false;
    }

protected:
    // IVideoClipsProvider overrides:
    TPlayResult GetPlayCommandDataImpl(TVideoItemConstScheme item,
                                       TPlayVideoCommandDataScheme commandData) const override;
};

class TContextWrapper final : public TYaVideoContentGetterDelegate {
public:
    explicit TContextWrapper(TContext& context, bool forceUnfiltered = false)
        : Context(context), ForceUnfiltered(forceUnfiltered)
    {}

    NHttpFetcher::TRequestPtr AttachProviderRequest(NHttpFetcher::IMultiRequest::TRef multiRequest) const override;

    bool IsSmartSpeaker() const override {
        return Context.MetaClientInfo().IsSmartSpeaker();
    }
    bool IsTvDevice() const override {
        return Context.MetaClientInfo().IsTvDevice();
    }
    bool SupportsBrowserVideoGallery() const override {
        return false;
    }
    bool HasExpFlag(TStringBuf name) const override {
        return Context.HasExpFlag(name);
    }
    const TString& UserTld() const override {
        return Context.UserTld();
    }
    void FillCgis(TCgiParameters& cgis) const override {
        AddYaVideoAgeFilterParam(Context, cgis, ForceUnfiltered);
    }
    void FillTunnellerResponse(const NSc::TValue& jsonData) const override {
        if (Context.HasExpFlag(NAlice::NExperiments::TUNNELLER_ANALYTICS_INFO) &&
                jsonData.IsDict() && jsonData.Has(NAlice::NAnalyticsInfo::TUNNELLER_RAW_RESPONSE)) {
            Context.GetAnalyticsInfoBuilder().AddTunnellerRawResponse(
                    TString{jsonData[NAlice::NAnalyticsInfo::TUNNELLER_RAW_RESPONSE].GetString()});
        }
    }
    void FillRequestForAnalyticsInfo(const TString& request, const TString& text, const ui32 code, const bool success) override {
        TStringBuf sanitizedUrl;
        TStringBuf query;
        TStringBuf fragment;
        SeparateUrlFromQueryAndFragment(request, sanitizedUrl, query, fragment);

        Context.GetAnalyticsInfoBuilder().AddVideoRequestSourceEvent(Context.GetRequestStartTime(), ToString(GetPathAndQuery(ToString(sanitizedUrl))))
            ->SetResponseCode(code, success)
            .SetRequestText(text)
            .SetRequestUrl(ToString(GetPathAndQuery(ToString(request))))
            .Build();
    }

private:
    TContext& Context;
    bool ForceUnfiltered;
};

} // namespace NVideo
} // namespace NBASS
