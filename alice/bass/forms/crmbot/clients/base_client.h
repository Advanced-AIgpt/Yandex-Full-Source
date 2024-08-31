#pragma once

#include <alice/bass/forms/market/client/base_client.h>

namespace NBASS::NCrmbot {

class TBaseClient : public NBASS::NMarket::TBaseClient {
private:
    using Base = NBASS::NMarket::TBaseClient;
public:
    explicit TBaseClient(const TSourcesRequestFactory& sources, NMarket::TMarketContext& ctx)
        : NMarket::TBaseClient(sources, ctx)
    {
    }

    template <class TTypedResponse>
    NBASS::NMarket::TResponseHandle<TTypedResponse> RunWithTrace(
        TStringBuf targetModule,
        NHttpFetcher::TRequestPtr request) const
    {
        return Base::RunWithTrace<TTypedResponse>(TStringBuf("lilucrmchat"), targetModule, request);
    }

    template <class TTypedResponse>
    NBASS::NMarket::TResponseHandle<TTypedResponse> RunWithTrace(
        TStringBuf targetModule,
        const TSourceRequestFactory& source,
        const TCgiParameters& params,
        const NSc::TValue& body = NSc::TValue(),
        const THashMap<TString, TString>& headers = THashMap<TString, TString>(),
        TStringBuf method_ = TStringBuf()) const
    {
        return Base::RunWithTrace<TTypedResponse>(
            TStringBuf("lilucrmchat"),
            targetModule,
            source,
            params,
            body,
            headers,
            method_);
    }

};

}

