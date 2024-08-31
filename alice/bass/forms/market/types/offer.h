#pragma once

#include "picture.h"
#include "warning.h"

#include <alice/bass/forms/market/types.h>
#include <library/cpp/scheme/scheme.h>

namespace NBASS {

namespace NMarket {

class TOffer {
public:
    explicit TOffer(const NSc::TValue& data);

    TStringBuf GetTitle() const { return Title; }
    TStringBuf GetWareId() const { return WareId; }
    TPrice GetPrice() const { return Price; }
    TPrice GetMinPrice() const { return MinPrice; }
    TPrice GetPriceBeforeDiscount() const { return PriceBeforeDiscount; }
    const TPicture& GetPicture() const { return Picture; }
    TStringBuf GetShop() const { return Shop; }
    TStringBuf GetShopUrl() const { return ShopUrl; }
    TStringBuf GetCpc() const { return Cpc; }
    TStringBuf GetCurrency() const { return Currency; }
    TMaybe<ui64> GetSku() const { return Sku; }
    const TVector<TWarning>& GetWarnings() const { return Warnings; }

private:
    TString Title;
    TString WareId;
    TPrice Price;
    TPrice MinPrice;
    TPrice PriceBeforeDiscount;
    TPicture Picture;
    TString Shop;
    TString ShopUrl;
    TString Cpc;
    TString Currency;
    TMaybe<ui64> Sku;
    TVector<TWarning> Warnings;

private:
    static TString GetShopUrl(const NSc::TValue& data);
};

} // namespace NMarket

} // namespace NBASS
