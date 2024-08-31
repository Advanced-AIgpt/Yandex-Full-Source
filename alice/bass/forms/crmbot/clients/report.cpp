#include "report.h"

#include <alice/bass/forms/crmbot/personal_data_helper.h>

namespace NBASS::NCrmbot {

TReportSearchRequestQuery::TReportSearchRequestQuery(const TMarketContext& ctx) : n_redirects(0)
{
    cgi = TCgiParameters();
    cgi.InsertUnescaped(TStringBuf("allow-collapsing"), TStringBuf("1")); // разрешить офферам схлопываться в модели
    cgi.InsertUnescaped(TStringBuf("rgb"), TStringBuf("blue"));
    cgi.InsertUnescaped(TStringBuf("place"), TStringBuf("prime"));
    cgi.InsertUnescaped(TStringBuf("pp"), TStringBuf("18"));              // используем pp=18
    cgi.InsertUnescaped(TStringBuf("bsformat"), TStringBuf("2"));         // отдавать выдачу в json
    cgi.InsertUnescaped(TStringBuf("uuid"), ctx.Meta().UUID());
    cgi.InsertUnescaped(TStringBuf("wprid"), ctx.RequestId());             // requid источника, который делает запрос
    cgi.InsertUnescaped(TStringBuf("onstock"), "1");

    TPersonalDataHelper personalData = TPersonalDataHelper(ctx.Ctx());
    auto rids = personalData.GetRegionId();
    if (rids.Defined()) {
        cgi.InsertUnescaped(TStringBuf("rids"), ToString(rids)); // регион
    } else {
        cgi.InsertUnescaped(TStringBuf("rids"), ToString(213));
    }
    TStringBuf ip = personalData.GetIP();
    if (!ip.empty()) {
        cgi.InsertUnescaped(TStringBuf("ip"), ip);
    }

    headers = THashMap<TString, TString>();
}

void TReportSearchRequestQuery::SetText(const TStringBuf& text)
{
    cgi.InsertUnescaped("text", text);
}

void TReportSearchRequestQuery::AllowRedirects(bool allow)
{
    cgi.InsertUnescaped(TStringBuf("cvredirect"), (allow) ? TStringBuf("1") : TStringBuf("0"));
}

void TReportSearchRequestQuery::AddRedirectCgi(const NSc::TValue& params) {
    n_redirects++;
    if (n_redirects > max_redirects) {
        ythrow TErrorException(NBASS::TError::EType::MARKETERROR, TStringBuf("too many redirects"));
    }

    if (!params.IsDict()) {
        ythrow TErrorException(
            NBASS::TError::EType::MARKETERROR,
            TStringBuf("redirect params are not stored in dict"));
    }

    const auto& keys = params.DictKeys(false /* not sorted */);
    for (const auto& key : keys) {
        cgi.EraseAll(key);
        const NSc::TValue& val = params[key];
        if (val.IsArray()) {
            for (const auto& p: val.GetArray()) {
                if (key == "lr") {
                    cgi.ReplaceUnescaped("rids", p.GetString());
                } else {
                    cgi.InsertEscaped(key, p.GetString());
                }
            }
        } else {
            cgi.InsertEscaped(key, val.GetString());
        }
    }
    if (cgi.Get("pp") == "18") {
        cgi.EraseAll("show-url");  // never use "show-url" with pp="18"
    }
}


void TReportClient::MakeRequestAsync(TReportSearchRequestQuery& state)
{
    const auto& source = Sources.MarketBlue();

    state.request_or_result = RunWithTrace<TReportSearchRequestQuery::TSearchResponse>(
        "market_report", source, state.cgi, NSc::TValue(), state.headers);
}


NSc::TValue TReportClient::CheckResponse(NHttpFetcher::TResponse::TRef resp) const
{
    if (resp->Result != NHttpFetcher::TResponse::EResult::Ok) {
        if (resp->Result == NHttpFetcher::TResponse::EResult::Timeout) {
            ythrow TErrorException(NBASS::TError::EType::MARKETERROR, TStringBuf("timeout error"));
        } else {
            ythrow TErrorException(NBASS::TError::EType::MARKETERROR, TStringBuf("failed report request"));
        }
    }

    auto resultValue = NSc::TValue::FromJson(resp->Data);
    if (resultValue.IsNull()) {
        ythrow TErrorException(
            NBASS::TError::EType::MARKETERROR,
            TStringBuf("cannot parse data"));
    }

    return resultValue;
}

NSc::TValue TReportClient::DoWaitRequest(TReportSearchRequestQuery& state) const
{
    if (!std::holds_alternative<TRunningRequest>(state.request_or_result)) {
        ythrow TErrorException(NBASS::TError::EType::MARKETERROR, TStringBuf("No running requests found"));
    }

    auto resp = std::get<TRunningRequest>(state.request_or_result).DoWait();

    return CheckResponse(resp);
}

void TReportClient::WaitRequestAsync(TReportSearchRequestQuery& state)
{
    NSc::TValue respJson = DoWaitRequest(state);

    if (respJson.Has(TStringBuf("redirect"))) {
        const auto& redirect = respJson.Get(TStringBuf("redirect"));

        if (redirect.Has(TStringBuf("url"))) {
            state.request_or_result = NMarket::THttpResponseConverter<
                TReportSearchRequestQuery::TUrlRedirectResponse>().FromJson(respJson);
            return;
        }

        state.AddRedirectCgi(redirect["params"]);

        const auto& source = Sources.MarketBlue();
        state.request_or_result = RunWithTrace<TReportSearchRequestQuery::TSearchResponse>(
            "market_report", source, state.cgi, NSc::TValue(), state.headers);
    } else {
        state.request_or_result = NMarket::THttpResponseConverter<
            TReportSearchRequestQuery::TSearchResponse>().FromJson(respJson);
    }
}

void TReportClient::WaitRequest(TReportSearchRequestQuery& state) const
{
    NSc::TValue respJson = DoWaitRequest(state);

    if (respJson.Has(TStringBuf("redirect"))) {
        ythrow TErrorException(
            NBASS::TError::EType::MARKETERROR,
            TStringBuf("unexpected report redirect"));
    }

    state.request_or_result = NMarket::THttpResponseConverter<
        TReportSearchRequestQuery::TSearchResponse>().FromJson(respJson);
}

void TReportClient::MakeRequest(TReportSearchRequestQuery& state, ui32 max_redirects)
{
    MakeRequestAsync(state);
    while (!state.IsFinished()) {
        if (max_redirects == 0) {
            state.AllowRedirects(false);
        }
        WaitRequestAsync(state);
        max_redirects--;
    }
};

}
