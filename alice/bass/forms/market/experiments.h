#pragma once

#include <alice/bass/forms/context/context.h>

namespace NBASS {

namespace NMarket {

class TMarketExperiments {
public:
    explicit TMarketExperiments(const TContext& ctx) : Ctx(ctx) { DetectExpVersion(); };

    bool Debug() const;
    bool DebugLog() const;

    ui32 ExpVersion() const;

    bool Market() const;
    bool MarketBeru() const;
    bool MarketNative() const;
    bool MarketNativeOff() const;
    bool MarketNativeOpen() const;
    bool MarketNativeOnMarket() const;
    bool MarketNativeBlueOpen() const;
    bool MarketStartChoiceAgainOpen() const;
    bool HowMuch() const;
    bool RecurringPurchase() const;
    bool ShoppingList() const;
    bool OrdersStatus() const;
    bool BeruMyBonusesList() const;

    bool ActivationUrls() const;
    bool AdsUrl() const;
    bool GalleryOpenUrl() const;
    bool GalleryOpenShop() const;
    bool ChoiceExtGalleryOpenMarket() const;
    bool Screenless() const;
    bool AllowWhiteList() const;
    bool AllowBlackList() const;
    bool AllowWhiteListMarket() const;
    bool AllowBlackListMarket() const;
    bool AdvBeruScenarioInHowMuch() const;
    bool UseVoiceInBeruAdvInHowMuch() const;
    bool DefaultOffer() const;
    bool ProductOffersCard() const;
    bool ProductOffersCardOpenMarket() const;
    bool UseExtendedWhiteGallery() const;
    bool UseV2Gallery() const;
    bool UseV3Gallery() const;
    bool DisableListening() const;
    bool AddToCart() const;
    size_t MaxOffersCount() const;
    bool ShoppingListDisableBeru() const;
    bool MdsPhrases() const;
    bool WithoutParametricSearch() const;
    bool WithoutShopsSearch() const;
    bool ShopplingListFuzzy() const;

    bool MarketNativeAllowWhereBuy() const;
    bool MarketNativeAllowChoose() const;
    bool MarketNativeAllowWhereSearch() const;
    bool MarketNativeAllowSearch() const;
    bool MarketNativeAllowWish() const;
    bool MarketNativeDenyChooseToBuy() const;

    bool HowMuchExtGallery() const;
    bool HowMuchExtGalleryOpenMarket() const;
    bool HowMuchExtGalleryAddButton() const;
    bool HowMuchActivateProtocolScenarioByVins() const;

private:
    const TContext& Ctx;
    ui32 expVersion = 0;

    void DetectExpVersion();
};

} // namespace NMarket

} // namespace NBASS
