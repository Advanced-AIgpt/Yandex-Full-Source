#include "beru_bonuses_client.h"

#include "base_client.h"
#include <alice/bass/forms/market/client/beru_bonuses.sc.h>
#include <alice/bass/forms/market/checkout_user_info.h>
#include <library/cpp/scheme/util/scheme_holder.h>

#include <util/generic/vector.h>

namespace NBASS {

namespace NMarket {

namespace {

using TCoinsResponseScheme = NBassApi::TBonusCoins<TBoolSchemeTraits>;

}

TVector<TCoin> TBeruBonusesClient::GetAllMyBonuses(TString nowUid)
{
    TCgiParameters cgi;
    cgi.InsertEscaped("uid", nowUid);

    const auto& response = Run<TSchemeHolder<TCoinsResponseScheme>>(Sources.MarketBeruMyBonusesList(), cgi).Wait();
    const auto& responseData = response.GetResponse();
    const auto& resCoins = responseData->Coins();

    TVector<TCoin> allCoins;
    for (const auto& coin : resCoins)
    {
        allCoins.push_back(TCoin(coin.Title(),
                                 coin.Subtitle(),
                                 coin.Image())
        );
    }

    return allCoins;
}

NSc::TValue TCoin::ToTValue() const
{
    NSc::TValue convRes;
    convRes["name"] = Name;
    convRes["subname"] = Subname;
    convRes["image"] = Image;

    return convRes;
}

}

}
