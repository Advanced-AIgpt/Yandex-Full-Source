#include <alice/bass/forms/market/client.h>
#include <alice/bass/forms/market/types.h>
#include <alice/bass/forms/market/util/report.h>

namespace NBASS {

namespace NMarket {

TReportResponse::TResult::TResult(NSc::TValue data)
    : Data(data)
    , Type(EType::NONE)
{
    auto entityType = Data["entity"].GetString();
    if (entityType == TStringBuf("product")) {
        Type = EType::MODEL;
    } else if (entityType == TStringBuf("offer")) {
        Type = EType::OFFER;
    }
}

TReportResponse::TResult::EType TReportResponse::TResult::GetType() const
{
    return Type;
}

TModel TReportResponse::TResult::GetModel() const
{
    Y_ASSERT(GetType() == EType::MODEL);

    return TModel(Data);
}

TVector<TOffer> TReportResponse::TResult::GetModelOffers() const
{
    Y_ASSERT(GetType() == EType::MODEL);
    const auto& offersData = Data["offers"]["items"];
    TVector<TOffer> offers{Reserve(offersData.ArraySize())};

    for (const auto& offer : offersData.GetArray()) {
        offers.push_back(TOffer(offer));
    }
    return offers;
}

TOffer TReportResponse::TResult::GetOffer() const
{
    Y_ASSERT(GetType() == EType::OFFER);
    return TOffer(Data);
}

const NSc::TArray& TReportResponse::TResult::GetCategories() const
{
    Y_ASSERT(EqualToOneOf(GetType(), EType::OFFER, EType::MODEL));
    return Data["categories"].GetArray();
}

TVector<TWarning> TReportResponse::TResult::GetWarnings() const
{
    Y_ASSERT(EqualToOneOf(GetType(), EType::OFFER, EType::MODEL));
    return TWarning::InitVector(Data["warnings"]);
}

bool TReportResponse::TResult::HasBlueOffer() const {
    return Data["skuStats"]["afterFiltersCount"].GetIntNumber() != 0;
}

} // namespace NMarket

} // namespace NBASS
