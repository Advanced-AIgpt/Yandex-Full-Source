#include "forms.h"
#include "login.h"
#include "market_beru_my_bonuses_list.h"
#include <alice/bass/forms/market/client/beru_bonuses_client.h>

#include <alice/library/analytics/common/product_scenarios.h>

#include <util/generic/vector.h>

namespace NBASS {

namespace NMarket {

TMarketBeruMyBonusesListImpl::TMarketBeruMyBonusesListImpl(TMarketContext& ctx)
    : Ctx(ctx)
    , Form(FromString<EMarketBeruBonusesForm>(Ctx.FormName()))
    , GeoSupport(Ctx.Ctx().GlobalCtx())
{
}

TResultValue TMarketBeruMyBonusesListImpl::Do()
{
    Ctx.Ctx().GetAnalyticsInfoBuilder().SetProductScenarioName(NAlice::NProductScenarios::BERU);
    if (!GeoSupport.IsBeruSupportedForGeoId(Ctx.UserRegion(), Ctx.GetScenarioType())) {
        Ctx.RenderMarketNotSupportedInLocation(EMarketType::BLUE, GeoSupport.GetRegionName(Ctx.UserRegion()));
        Ctx.RenderEmptySerp();
        return TResultValue();
    }

    TCheckoutUserInfo user(Ctx);
    user.Init(true);

    if (user.IsGuest()) {
        return HandleGuest(Ctx, Form == EMarketBeruBonusesForm::Login);
    }

    TBeruBonusesClient bonusClient(Ctx.GetSources(), Ctx);

    auto response = bonusClient.GetAllMyBonuses(user.GetUid());

    NSc::TValue resultBonusesData;
    auto& coinsArray = resultBonusesData["coins"].SetArray();

    for (const TCoin& coin : response) {
        coinsArray.Push(coin.ToTValue());
    }

    if (!response.empty()) {
        Ctx.AddTextCardBlock("market_beru_my_bonuses_list__yes_answer", resultBonusesData);
    } else {
        Ctx.AddTextCardBlock("market_beru_my_bonuses_list__no_answer");
    }

    return TResultValue();
}

}

}
