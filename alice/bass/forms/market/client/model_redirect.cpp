#include <alice/bass/forms/market/client.h>
#include <alice/bass/forms/market/util/string.h>

#include <library/cpp/string_utils/quote/quote.h>

namespace NBASS {

namespace NMarket {

TReportResponse::TModelRedirect::TModelRedirect(NSc::TValue data, EMarketType marketType)
    : TBaseRedirect(data)
    , ModelId(0)
    , SkuId(0)
    , Hid(Nothing())
{
    NSc::TValue& redirect = Data["redirect"];

    const auto& params = redirect["params"];
    if (params.Has("modelid")) {
        (marketType == EMarketType::BLUE ? SkuId : ModelId) =
            FromStringWithLogging<TModelId>(params["modelid"][0].GetString(), 0);
    } else if (params.Has("skuId")) {
        SkuId = FromStringWithLogging<ui64>(params["skuId"][0].GetString(), 0);
    }
    if (params.Has("hid")) {
        Hid = FromStringWithLogging<ui64>(params["hid"][0].GetString(), 0);
    }
}

TModelId TReportResponse::TModelRedirect::GetModelId() const
{
    return ModelId;
}

ui64 TReportResponse::TModelRedirect::GetSkuId() const
{
    return SkuId;
}

TMaybe<ui64> TReportResponse::TModelRedirect::GetHid() const
{
    return Hid;
}

} // namespace NMarket

} // namespace NBASS
