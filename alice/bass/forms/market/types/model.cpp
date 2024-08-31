#include "model.h"
#include <util/string/cast.h>

namespace NBASS {

namespace NMarket {

TModel::TModel(const NSc::TValue& data)
    : Id(data["id"].GetIntNumber())
    , Slug(data["slug"].GetString())
    , Title(data["titles"]["raw"].GetString())
    , GlFilters(CreateGlFilters(data))
    , Picture(TPicture::GetMostSuitablePicture(data, GlFilters))
    , DefaultOffer(GetDefaultOffer(data))
    , MinPrice(data["prices"]["min"].ForceNumber())
    , AvgPrice(data["prices"]["avg"].ForceNumber(UNDEFINED_PRICE_VALUE))
    , DefaultPrice(GetModelDefaultPrice(data))
    , DefaultPriceBeforeDiscount(GetModelDefaultPriceBeforeDiscount(data))
    , Currency(data["prices"]["currency"].GetString())
    , Warnings(GetWarnings(data, DefaultOffer))
    , Rating(data.Has("rating") ? MakeMaybe(data["rating"].GetNumber()) : Nothing())
    , ReviewCount(GetReviewCount(data))
    , AdviserPercentage(GetAdviserPercentage(data))
    , Advantages(GetAdvantages(data))
{
}

TMaybe<TOffer> TModel::GetDefaultOffer(const NSc::TValue& data)
{
    const auto& offer = data.TrySelect("/offers/items[0]");
    return !offer.IsNull() ? MakeMaybe(TOffer(offer)) : Nothing();
}

TPrice TModel::GetModelDefaultPrice(const NSc::TValue& data)
{
    return data.TrySelect("/offers/items[0]/prices/value").ForceNumber(UNDEFINED_PRICE_VALUE);
}

TPrice TModel::GetModelDefaultPriceBeforeDiscount(const NSc::TValue& data)
{
    return data.TrySelect("/offers/items[0]/prices/discount/oldMin").ForceNumber(UNDEFINED_PRICE_VALUE);
}

TVector<TWarning> TModel::GetWarnings(const NSc::TValue& data, TMaybe<TOffer>& defaultOffer) {
    if (defaultOffer) {
        return defaultOffer->GetWarnings();
    } else {
        return TWarning::InitVector(data["warnings"]);
    }
}

TCgiGlFilters TModel::CreateGlFilters(const NSc::TValue& data)
{
    TCgiGlFilters filters;
    for (const auto& jsonedFilter : data["filters"].GetArray()) {
        if (jsonedFilter["kind"] == 2) {
            TVector<TString> checkedValues;
            const bool isNumber = jsonedFilter["type"].GetString() == TStringBuf("number");
            for (const auto& jsonedValue : jsonedFilter["values"].GetArray()) {
                if (jsonedValue["checked"].GetBool(false)) {
                    if (isNumber) {
                        TStringBuf min = jsonedValue["min"].GetString();
                        TStringBuf max = jsonedValue["max"].GetString();
                        // todo MALISA-240 логику формирования gl фильтров вынести в одно место
                        checkedValues.push_back(ToString(
                            min == max
                                ? min
                                : TStringBuilder() << min << '~' << max));
                    } else {
                        checkedValues.push_back(jsonedValue["id"].ForceString());
                    }
                }
            }
            if (!checkedValues.empty()) {
                filters[jsonedFilter["id"].ForceString()] = checkedValues;
            }
        }
    }
    return filters;
}

TMaybe<ui32> TModel::GetReviewCount(const NSc::TValue& data)
{
    if (data.Has("opinions")) {
        auto value = data["opinions"].GetIntNumber();
        if (value > 0) {
            return static_cast<ui32>(value);
        }
    }
    return Nothing();
}

TMaybe<ui8> TModel::GetAdviserPercentage(const NSc::TValue& data)
{
    for (const auto& reason : data["reasonsToBuy"].GetArray()) {
        if (reason["id"] == TStringBuf("customers_choice")) {
            double value = reason["value"].GetNumber();
            if (value > 0 && value <= 1) {
                return static_cast<ui8>(value * 100);
            }
        }
    }
    return Nothing();
}

TVector<TString> TModel::GetAdvantages(const NSc::TValue& data)
{
    TVector<const NSc::TValue*> rawAdvantages;
    for (const auto& rawAdvantage : data["reasonsToBuy"].GetArray()) {
        if (rawAdvantage["id"] == TStringBuf("best_by_factor")) {
            rawAdvantages.push_back(&rawAdvantage);
        }
    }
    SortBy(rawAdvantages, [](const NSc::TValue* item) { return (*item)["factor_priority"].ForceIntNumber(); });
    TVector<TString> result(Reserve(rawAdvantages.size()));
    for (const auto rawAdvantagePtr : rawAdvantages) {
        result.push_back((*rawAdvantagePtr)["factor_name"].ForceString());
    }
    return result;
}

} // namespace NMarket

} // namespace NBASS
