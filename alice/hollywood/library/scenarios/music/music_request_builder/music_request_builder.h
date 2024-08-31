#pragma once

#include "music_request_mode.h"

#include <alice/hollywood/library/http_proxy/http_proxy.h>

#include <alice/hollywood/library/scenarios/music/proto/request.pb.h>

namespace NAlice::NHollywood::NMusic {

class TGuestRequestMetaProvider : public IRequestMetaProvider {
public:
    template<typename TStringU>
    TGuestRequestMetaProvider(const NScenarios::TRequestMeta& meta, TStringU&& guestOAuthToken)
        : Meta_(meta)
        , GuestOAuthToken_(std::forward<TStringU>(guestOAuthToken))
    {
    }

    const TString& GetRequestId() const override;
    const TString& GetClientIP() const override;
    const TString& GetOAuthToken() const override;
    const TString& GetUserTicket() const override;

private:
    const NScenarios::TRequestMeta& Meta_;
    TString GuestOAuthToken_;
};

class TMusicRequestBuilder : public THttpProxyNoRtlogRequestBuilder {
public:
    TMusicRequestBuilder(const TStringBuf path, const TMusicRequestMeta& requestMeta, TRTLogger& logger,
                         const TMusicRequestModeInfo& musicRequestModeInfo,
                         const TString name = Default<TString>());

    TMusicRequestBuilder(const TStringBuf path, const NScenarios::TRequestMeta& meta,
                         const TClientInfo& clientInfo, TRTLogger& logger,
                         const bool enableCrossDc, const TMusicRequestModeInfo& musicRequestModeInfo,
                         const TString name = Default<TString>());

    TMusicRequestBuilder(const TStringBuf path, TAtomicSharedPtr<IRequestMetaProvider> metaProvider,
                         const TClientInfo& clientInfo, TRTLogger& logger,
                         const bool enableCrossDc, const TMusicRequestModeInfo& musicRequestModeInfo,
                         const TString name = Default<TString>());

private:
    void Init(const TClientInfo& clientInfo, const bool enableCrossDc,
              const TMusicRequestModeInfo& musicRequestModeInfo, TRTLogger& logger);
};

} // namespace NAlice::NHollywood::NMusic
