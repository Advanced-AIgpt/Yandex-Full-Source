#include "response.h"

#include <util/string/builder.h>

namespace NAlice::NHollywood::NMarket {

namespace {

TMaybe<TStringBuf> TryGetSingleParam(const NJson::TJsonValue::TArray& values)
{
    if (values.size() != 1) {
        return Nothing();
    }
    return values[0].GetString();
}

template<class TValue>
TMaybe<TValue> TryGetSingleParamWithType(const NJson::TJsonValue::TArray& values)
{
    const TMaybe<TStringBuf> raw = TryGetSingleParam(values);
    if (!raw) {
        return Nothing();
    }
    TValue result;
    if (TryFromString<TValue>(values[0].GetString(), result)) {
        return result;
    }
    return Nothing();
}

template<class TValue>
TValue FromStringOrDefault(const TStringBuf str, TValue default_)
{
    TValue result;
    if (TryFromString<TValue>(str, result)) {
        return result;
    }
    return default_;
}

} // namespace

const TVector<TCategory>& TBaseReportDocument::GetCategories() const
{
    if (!Categories) {
        const auto& rawCategories = Data["categories"].GetArray();
        Categories.ConstructInPlace(Reserve(rawCategories.size()));
        for (const auto& category : rawCategories) {
            Categories->emplace_back(
                category["id"].GetUInteger(),
                category["nid"].GetUInteger(),
                category["slug"].GetString());
        }
    }
    return Categories.GetRef();
}

TReportOffer::TPrices TReportOffer::CreatePrices() const
{
    TPrices prices;
    const auto& rawPrices = Data["prices"];
    prices.Currency = rawPrices["currency"].GetString();
    if (rawPrices.Has("min")) {
        prices.Min = FromStringOrDefault<TPrice>(rawPrices["min"].GetString(), 0);
    }
    if (rawPrices.Has("value")) {
        prices.Value = FromStringOrDefault<TPrice>(rawPrices["value"].GetString(), 0);
    }
    if (rawPrices.Has("discount")) {
        prices.BeforeDiscount =
            FromStringOrDefault<TPrice>(rawPrices["discount"]["oldMin"].GetString(), 0);
    }
    return prices;
}

TMaybe<TReportOffer> TReportModel::CreateDefaultOffer() const
{
    if (Data["offers"]["items"].GetArray().empty()) {
        return Nothing();
    }
    return TReportOffer(Data["offers"]["items"][0]);
}

TReportModel::TPrices TReportModel::CreatePrices() const
{
    TPrices prices;
    const auto& rawPrices = Data["prices"];
    prices.Min = FromStringOrDefault<TPrice>(rawPrices["min"].GetString(), 0);
    prices.Currency = rawPrices["currency"].GetString();
    if (rawPrices.Has("avg")) {
        prices.Avg = FromStringOrDefault<TPrice>(rawPrices["avg"].GetString(), 0);
    }
    if (DefaultOffer) {
        prices.Default = DefaultOffer->GetPrices().Value;
        prices.DefaultBeforeDiscount = DefaultOffer->GetPrices().BeforeDiscount;
    }
    return prices;
}

TReportPrimeRedirect::TReportPrimeRedirect(const NJson::TJsonValue& data)
{
    const auto& params = data["params"];

    if (const auto maybeLr = TryGetSingleParamWithType<NGeobase::TId>(params["lr"].GetArray())) {
        RegionId = *maybeLr;
    }
    if (const auto maybeText = TryGetSingleParam(params["text"].GetArray())) {
        Text = ToString(*maybeText);
    }

    const auto nid = TryGetSingleParamWithType<TNid>(params["nid"].GetArray());
    const auto hid = TryGetSingleParamWithType<THid>(params["hid"].GetArray());
    if (hid || nid) {
        if (nid.Empty()) {
            Category = TCategory(*hid);
        } else {
            const auto slug = TryGetSingleParam(params["slug"].GetArray()).GetOrElse({});
            Category = hid
                ? TCategory(*hid, *nid, slug)
                : TCategory(*nid, slug);
        }
    }

    for (const auto& filterStr : params["glfilter"].GetArray()) {
        TVector<TString> idWithVal{Reserve(2)};
        Split(filterStr.GetString(), ":", idWithVal);
        if (idWithVal.size() != 2) {
            continue;
        }
        TVector<TString> vals;
        Split(idWithVal[1], ",", vals);
        GlFilters[idWithVal[0]] = vals;
    }

    static constexpr TStringBuf redirectParams[] {
        TStringBuf("suggest_text"),
        TStringBuf("rs"),
        TStringBuf("was_redir"),
        TStringBuf("rt"),
    };
    for (const TStringBuf param : redirectParams) {
        const auto& rawValues = params[param].GetArray();
        if (!rawValues.empty()) {
            TVector<TStringBuf> values{Reserve(rawValues.size())};
            for (const auto& rawValue : rawValues) {
                values.push_back(rawValue.GetString());
            }
            RedirectParams.ReplaceUnescaped(param, values.begin(), values.end());
        }
    }

    const auto cvredirectValue = TryGetSingleParam(params["cvredirect"].GetArray());
    HasRedirect = cvredirectValue && *cvredirectValue == TStringBuf("2");
}

const TVector<TReportDocument>& TReportPrimeResponse::GetDocuments() const
{
    if (Documents_.Empty()) {
        Documents_ =
            TVector<TReportDocument>(Reserve(Data["search"]["results"].GetArray().size()));
        for (auto& doc : Data["search"]["results"].GetArray()) {
            TStringBuf type = doc["entity"].GetString();
            if (type == TStringBuf("product")) {
                Documents_->emplace_back(TReportModel(doc));
            } else if (type == TStringBuf("offer")) {
                Documents_->emplace_back(TReportOffer(doc));
            }
        }
    };
    return Documents_.GetRef();
}

} // namespace NAlice::NHollywood::NMarket
