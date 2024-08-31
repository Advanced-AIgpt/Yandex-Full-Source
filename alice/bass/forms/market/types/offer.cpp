#include "offer.h"

namespace NBASS {

namespace NMarket {

TOffer::TOffer(const NSc::TValue& data)
    : Title(data["titles"]["raw"].GetString())
    , WareId(data["wareId"].GetString())
    , Price(data["prices"]["value"].ForceNumber())
    , MinPrice(data["prices"]["min"].ForceNumber())
    , PriceBeforeDiscount(data["prices"]["discount"]["oldMin"].ForceNumber())
    , Picture(TPicture::GetMostSuitablePicture(data))
    , Shop(data["shop"]["name"].GetString())
    , ShopUrl(GetShopUrl(data))
    , Cpc(data["cpc"])
    , Currency(data["prices"]["currency"].GetString())
    , Sku(data["marketSku"].ForceIntNumber())
    , Warnings(TWarning::InitVector(data["warnings"]))
{
}

TString TOffer::GetShopUrl(const NSc::TValue& data)
{
    if (data["urls"]["encrypted"].IsString()) {
        return TStringBuilder() << TStringBuf("https://market-click2.yandex.ru") << data["urls"]["encrypted"].GetString();
    }
    return TString();
}

} // namespace NMarket

} // namespace NBASS
