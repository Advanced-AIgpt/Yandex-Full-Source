#pragma once

#include "defs.h"

#include <alice/bass/libs/fetcher/neh.h>
#include <alice/bass/libs/fetcher/request.h>

#include <alice/bass/util/error.h>

namespace NSc {
class TValue;
}

namespace NBASS {
class TContext;

struct TRequestContentOptions {
    enum class EType { Buy, Play };

    TRequestContentOptions() = default;

    TRequestContentOptions(EType type, bool startPurchaseProcess)
        : Type(type)
        , StartPurchaseProcess(startPurchaseProcess)
    {
    }

    EType Type = EType::Play;
    bool StartPurchaseProcess = true;
};

namespace NVideo {
class IBillingAPI {
public:
    virtual ~IBillingAPI() = default;

    NHttpFetcher::THandle::TRef RequestContent(const TRequestContentOptions& options, const NSc::TValue& contentItem,
                                               TShowPayScreenCommandDataConstScheme contentPlayPayload) {
        return RequestContent(options, contentItem, *contentPlayPayload->GetRawValue());
    }

    NHttpFetcher::THandle::TRef RequestContent(const TRequestContentOptions& options, const NSc::TValue& contentItem,
                                               TRequestContentPayloadConstScheme contentPlayPayload) {
        return RequestContent(options, contentItem, *contentPlayPayload->GetRawValue());
    }

    virtual NHttpFetcher::THandle::TRef GetPlusPromoAvailability() = 0;

protected:
    virtual NHttpFetcher::THandle::TRef RequestContent(const TRequestContentOptions& options,
                                                       const NSc::TValue& contentItem,
                                                       const NSc::TValue& contentPlayPayload) = 0;
};

class TBillingAPI : public IBillingAPI {
public:
    explicit TBillingAPI(TContext& ctx);
    TBillingAPI(TContext& ctx, NHttpFetcher::IMultiRequest::TRef multiRequest);

    // IBillingAPI overrides:
    NHttpFetcher::THandle::TRef GetPlusPromoAvailability() override;

protected:
    // IBillingAPI overrides:
    NHttpFetcher::THandle::TRef RequestContent(const TRequestContentOptions& options, const NSc::TValue& contentItem,
                                               const NSc::TValue& contentPlayPayload) override;

private:
    void SetupRequest(NHttpFetcher::TRequest& request);
    void AddCodecHeadersIntoBillingRequest(NHttpFetcher::TRequestPtr& request) const;

private:
    TContext& Ctx;
    NHttpFetcher::IMultiRequest::TRef MultiRequest;
};
} // namespace NVideo
} // namespace NBASS
