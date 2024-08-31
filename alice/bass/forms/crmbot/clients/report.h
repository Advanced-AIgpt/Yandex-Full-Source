#pragma once

#include "base_client.h"

#include <alice/bass/forms/market/client/checkout.sc.h>
#include <alice/bass/forms/crmbot/clients/report_url_redirect.sc.h>

namespace NBASS::NCrmbot {

class TReportSearchRequestQuery {
public:
    using TSearchResponse = TSchemeHolder<NBassApi::TReportSearchResponse<NMarket::TBoolSchemeTraits>>;
    using TUrlRedirectResponse = TSchemeHolder<NBassApi::TReportUrlRedirect<NMarket::TBoolSchemeTraits>>;

    using TMarketContext = NMarket::TMarketContext;

private:
    using TRunningRequest = NMarket::TResponseHandle<TSearchResponse>;
    using TRequestOrResult = std::variant<TSearchResponse, TUrlRedirectResponse, TRunningRequest>;

public:
    explicit TReportSearchRequestQuery(const TMarketContext& ctx);

    void SetText(const TStringBuf& text);
    void AllowRedirects(bool allow);
    void AddRedirectCgi(const NSc::TValue& params);

    static constexpr ui32 max_redirects = 1;

    bool HasSearchResponse() const {
        return std::holds_alternative<TSearchResponse>(request_or_result);
    }
    bool HasRedirectUrl() const {
        return std::holds_alternative<TUrlRedirectResponse>(request_or_result);
    }
    bool IsFinished() const {
        return HasSearchResponse() || HasRedirectUrl();
    }
    TSearchResponse GetSearchResponse() {
        return std::get<TSearchResponse>(request_or_result);
    }
    TStringBuf GetRedirectUrl() {
        return std::get<TUrlRedirectResponse>(request_or_result)->Redirect().Url();
    }
private:
    TCgiParameters cgi;
    THashMap<TString, TString> headers;

    TString request;
    TRequestOrResult request_or_result;
    ui32 n_redirects;

    friend class TReportClient;
};

class TReportClient : public TBaseClient {
public:
    using TMarketContext = NMarket::TMarketContext;

    explicit TReportClient(NMarket::TMarketContext& ctx) : TBaseClient(ctx.GetSources(), ctx) {};

    void MakeRequest(TReportSearchRequestQuery& state, ui32 max_redirects = 3);

    /* The next functions are complicated and designed to be called one after another.
     * Usage example can be found in MakeRequest() implementation*/
    /* 1. Returns immediately after making request (stores handle in *state* parameter). */
    void MakeRequestAsync(TReportSearchRequestQuery& state);
    /* 2. Waits for response from the handle in *state*.
     * If the response recieved contains actual results, stores them in *state* and returns immediately.
     * If the response recieved contains redirect with URL stores it in *state* and returns immediately.
     * If the response recieved contains redirect with CGI parameters updates *state*'s cgi, makes another request,
     * stores the handle in *state* and returns immediately.
     * This function can be chained TReportSearchRequestQuery::max_redirects times. */
    void WaitRequestAsync(TReportSearchRequestQuery& state);
    /* 3. Waits for response from the handle in *state* and saves result.
     * Will throw if recieves redirect in response. */
    void WaitRequest(TReportSearchRequestQuery& state) const;

private:
    using TRunningRequest = TReportSearchRequestQuery::TRunningRequest;

    NSc::TValue DoWaitRequest(TReportSearchRequestQuery& state) const;
    NSc::TValue CheckResponse(NHttpFetcher::TResponse::TRef resp) const;
};

}
