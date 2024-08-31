#pragma once

#include "defs.h"
#include "utils.h"

#include <util/generic/ptr.h>
#include <util/string/cast.h>
#include <library/cpp/cgiparam/cgiparam.h>
#include <util/string/split.h>
#include <util/string/subst.h>
#include <library/cpp/string_utils/url/url.h>
#include <util/string/vector.h>

namespace NBASS::NVideo {

class TYaVideoContentGetterDelegate {
public:
    virtual ~TYaVideoContentGetterDelegate() = default;

    virtual NHttpFetcher::TRequestPtr AttachProviderRequest(NHttpFetcher::IMultiRequest::TRef multiRequest) const = 0;
    virtual bool IsSmartSpeaker() const = 0;
    virtual bool IsTvDevice() const = 0;
    virtual bool SupportsBrowserVideoGallery() const = 0;
    virtual bool HasExpFlag(TStringBuf name) const = 0;
    virtual const TString& UserTld() const = 0;
    virtual void FillCgis(TCgiParameters& cgis) const = 0;
    virtual void FillTunnellerResponse(const NSc::TValue& jsonData) const = 0;
    virtual void FillRequestForAnalyticsInfo(const TString& request, const TString& text, const ui32 code, const bool success) = 0;
};

class TContentInfoHandle : public NVideoCommon::IVideoItemHandle {
public:
    TContentInfoHandle(TSimpleSharedPtr<TYaVideoContentGetterDelegate> contentGetterDelegate,
                       NVideoCommon::TVideoItemConstScheme item,
                       NHttpFetcher::IMultiRequest::TRef multiRequest);

    NVideoCommon::TResult WaitAndParseResponse(NVideoCommon::TVideoItem& item) override;

private:
    TSimpleSharedPtr<TYaVideoContentGetterDelegate> ContentGetterDelegate;
    TString DebugRequestText;
    TString DebugUrl;
    NHttpFetcher::THandle::TRef Handle;
};

TString BuildPlayerUri(TStringBuf playerCode, TStringBuf playerId,
                       const TYaVideoContentGetterDelegate& contentGetterDelegate);
TString BuildVideoUriForBrowser(TStringBuf url, TStringBuf title,
                                const TYaVideoContentGetterDelegate& contentGetterDelegate);
TString MakeWebYoutubeUrl(TStringBuf id);
TString MakeYoutubeUrl(TStringBuf id, TYaVideoContentGetterDelegate& contentGetterDelegate);

bool ParseVideoItemJsonResponse(const NSc::TValue& clip, TYaVideoContentGetterDelegate& contentGetterDelegate,
                                NVideoCommon::TVideoItemScheme& item, TStringBuf sourceTag);
NVideoCommon::TResult ParseJsonResponse(const NSc::TValue& jsonData,
                                        TYaVideoContentGetterDelegate& contentGetterDelegate,
                                        NVideoCommon::TVideoGalleryScheme* response, TStringBuf sourceTag);
} // namespace NBASS:NVideo
