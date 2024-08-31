#pragma once

#include "video_provider.h"

#include <alice/bass/libs/fetcher/request.h>

#include <alice/bass/setup/setup.h>

namespace NBASS::NVideo {

class TWebSearchByProviderHandle : public NBASS::TSetupRequest<TWebSearchByProviderResponse> {
public:
    TWebSearchByProviderHandle(TStringBuf patchedQuery, const TVideoClipsRequest& request, TStringBuf providerName,
                               TContext& context);
    NHttpFetcher::THandle::TRef Fetch(NHttpFetcher::IMultiRequest::TRef multiRequest) override;
    TResultValue Parse(NHttpFetcher::TResponse::TConstRef httpResponse, TWebSearchByProviderResponse* response, NSc::TValue* factorsData) override;

protected:
    virtual bool ParseItemFromDoc(const NSc::TValue doc, TVideoItem* item) = 0;

protected:
    const TString ProviderName;
    TContext& Context;
    const TVideoClipsRequest Request;
    const TString PatchedQuery;
};

} // namespace NBASS::NVideo
