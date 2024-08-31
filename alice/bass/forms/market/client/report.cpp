#include <alice/bass/forms/market/client.h>
#include <alice/bass/forms/market/client/report.sc.h>
#include <library/cpp/scheme/util/scheme_holder.h>

namespace NBASS {

namespace NMarket {

TReportResponse::TReportResponse(
        const NHttpFetcher::TResponse::TRef response,
        const TStringBuf place,
        EMarketType marketType)
    : TBaseJsonResponse(response)
    , RedirectType(TReportResponse::ERedirectType::NONE)
    , Place(place)
    , MarketType(marketType)
    , FormalizedGlFilters(InitFormalizedGlFilters())
{
    if (HasError()) {
        return;
    }

    NSc::TValue& data = Data.GetRef();
    if (!data.Has("redirect")) {
        RedirectType = TReportResponse::ERedirectType::NONE;
        return;
    }

    NSc::TValue& redirect = data["redirect"];
    const auto target = redirect["target"].GetString();
    if (target == TStringBuf("product")) {
        RedirectType = ERedirectType::MODEL;
    } else if (target == TStringBuf("catalog")) {
        RedirectType = ERedirectType::PARAMETRIC;
    } else if (target == TStringBuf("search")
        && redirect["params"].Has("hid")
        && redirect["params"].Has("nid"))
    {
        RedirectType = ERedirectType::PARAMETRIC;
    } else if (target == TStringBuf("search") && redirect["params"].Has("lr")) {
        RedirectType = ERedirectType::REGION;
    } else {
        RedirectType = ERedirectType::UNKNOWN;
    }
}

TReportResponse::ERedirectType TReportResponse::GetRedirectType() const
{
    Y_ASSERT(Place == TStringBuf("prime"));
    return RedirectType;
}

TReportResponse::TParametricRedirect TReportResponse::GetParametricRedirect() const
{
    Y_ASSERT(Place == TStringBuf("prime"));
    Y_ASSERT(RedirectType == TReportResponse::ERedirectType::PARAMETRIC);
    return TReportResponse::TParametricRedirect(Data.GetRef());
}

TReportResponse::TModelRedirect TReportResponse::GetModelRedirect() const
{
    Y_ASSERT(Place == TStringBuf("prime"));
    Y_ASSERT(RedirectType == TReportResponse::ERedirectType::MODEL);
    return TReportResponse::TModelRedirect(Data.GetRef(), MarketType);
}

TReportResponse::TRegionRedirect TReportResponse::GetRegionRedirect() const
{
    Y_ASSERT(EqualToOneOf(Place, TStringBuf("prime")));
    Y_ASSERT(RedirectType == TReportResponse::ERedirectType::REGION);
    return TReportResponse::TRegionRedirect(Data.GetRef());
}

TVector<TReportResponse::TResult> TReportResponse::GetResults() const
{
    Y_ASSERT(EqualToOneOf(
        Place, TStringBuf("prime"), TStringBuf("modelinfo"), TStringBuf("offerinfo")));
    Y_ASSERT(RedirectType == TReportResponse::ERedirectType::NONE);
    TVector<TReportResponse::TResult> results;
    for (const auto& item : Data.GetRef()["search"]["results"].GetArray()) {
        TResult result(item);
        if (result.GetType() == TReportResponse::TResult::EType::NONE) {
            continue;
        }
        results.emplace_back(std::move(result));
    }
    return results;
}

const NSc::TArray& TReportResponse::GetFilters() const
{
    Y_ASSERT(Place == TStringBuf("prime"));
    Y_ASSERT(RedirectType == TReportResponse::ERedirectType::NONE);
    return Data.GetRef()["filters"].GetArray();
}

const NSc::TValue* TReportResponse::GetFilter(const TStringBuf id) const
{
    Y_ASSERT(Place == TStringBuf("prime"));
    for (const auto& filter : GetFilters()) {
        if (filter["id"].GetString() == id) {
            return &filter;
        }
    }
    return nullptr;
}

const NSc::TArray& TReportResponse::GetIntents() const
{
    Y_ASSERT(Place == TStringBuf("prime"));
    Y_ASSERT(RedirectType == TReportResponse::ERedirectType::NONE);
    return Data.GetRef()["intents"].GetArray();
}

i64 TReportResponse::GetTotal() const
{
    Y_ASSERT(Place == TStringBuf("prime"));
    Y_ASSERT(RedirectType == TReportResponse::ERedirectType::NONE);
    return Data.GetRef()["search"]["total"].GetIntNumber();
}

TLazyValue<TFormalizedGlFilters> TReportResponse::InitFormalizedGlFilters() const
{
    return [this]() {
        Y_ASSERT(Place == TStringBuf("formalize_gl"));
        TFormalizedGlFilters data(Data.GetRef());
        if (!data->Validate()) {
            LOG(ERR) << "Invalid report response" << Endl;
            Y_ASSERT(false);
        }
        return data;
    };
}

const TFormalizedGlFilters& TReportResponse::GetFormalizedGlFilters() const
{
    return FormalizedGlFilters.GetRef();
}

TCgiGlFilters TReportResponse::GetFormalizedCgiGlFilters() const
{
    THashMap<TString, TVector<TString>> filters;
    for (const auto& kv : GetFormalizedGlFilters()->Filters()) {
        const auto& filter = kv.Value();
        const auto& isNumber = filter.Type() == TStringBuf("number");
        const auto& rawValues = filter.Values();
        TVector<TString> values(Reserve(rawValues.Size()));
        for (const auto& rawVal : rawValues) {
            if (isNumber) {
                double value = rawVal.Num();
                values.push_back(TStringBuilder() << value << "~" << value);
            } else {
                values.push_back(ToString(rawVal.Id()));
            }
        }
        filters[ToString(kv.Key())] = values;
    }
    return filters;
}

} // namespace NMarket

} // namespace NBASS
