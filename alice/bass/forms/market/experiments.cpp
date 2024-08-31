#include "experiments.h"

#include <alice/bass/libs/logging_v2/logger.h>

namespace NBASS {

namespace NMarket {

bool TMarketExperiments::Debug() const {
    return Ctx.HasExpFlag("market_debug");
}

bool TMarketExperiments::DebugLog() const {
    return Ctx.HasExpFlag("market_debug_log");
}

ui32 TMarketExperiments::ExpVersion() const {
    return expVersion;
}

bool TMarketExperiments::ActivationUrls() const {
    return Ctx.HasExpFlag("market_activation_urls");
}

bool TMarketExperiments::Market() const {
    return Ctx.HasExpFlag("market_enable") || !Ctx.HasExpFlag("market_disable");
}

bool TMarketExperiments::MarketBeru() const {
    return !Ctx.HasExpFlag("market_beru_disable");
}

bool TMarketExperiments::MarketNative() const {
    return Ctx.HasExpFlag("market_native");
}

bool TMarketExperiments::MarketNativeOff() const {
    return Ctx.HasExpFlag("market_native_off");
}

bool TMarketExperiments::MarketNativeOpen() const {
    return Ctx.HasExpFlag("market_native_open");
}

bool TMarketExperiments::MarketNativeOnMarket() const {
    return Market() && Ctx.HasExpFlag("market_native_market");
}

bool TMarketExperiments::MarketNativeBlueOpen() const {
    return Ctx.HasExpFlag("market_native_blue_open");
}

bool TMarketExperiments::MarketStartChoiceAgainOpen() const {
    return Ctx.HasExpFlag("market_start_choice_again_open");
}

bool TMarketExperiments::HowMuch() const {
    return !Ctx.HasExpFlag("how_much_disable");
}

bool TMarketExperiments::RecurringPurchase() const {
    return !Ctx.HasExpFlag("recurring_purchase_disable");
}

bool TMarketExperiments::ShoppingList() const {
    return !Ctx.HasExpFlag("shopping_list_disable");
}

bool TMarketExperiments::OrdersStatus() const {
    return !Ctx.HasExpFlag("market_orders_status_disable");
}

bool TMarketExperiments::BeruMyBonusesList() const {
    return Ctx.HasExpFlag("market_beru_my_bonuses_list");
}

bool TMarketExperiments::AdsUrl() const {
    return Ctx.HasExpFlag("market_ads_url");
}

bool TMarketExperiments::GalleryOpenUrl() const {
    return Ctx.HasExpFlag("market_gallery_open_url");
}

bool TMarketExperiments::GalleryOpenShop() const {
    return !Ctx.HasExpFlag("market_gallery_dont_open_shop");
}

bool TMarketExperiments::ChoiceExtGalleryOpenMarket() const {
    return Ctx.HasExpFlag("market_choice_ext_gallery_open_market");
}

bool TMarketExperiments::Screenless() const {
    return Ctx.HasExpFlag("market_screenless");
}

bool TMarketExperiments::AllowWhiteList() const {
    return Ctx.HasExpFlag("market_allow_white_list");
}

bool TMarketExperiments::AllowBlackList() const {
    return Ctx.HasExpFlag("market_allow_black_list");
}

bool TMarketExperiments::AllowWhiteListMarket() const {
    return Ctx.HasExpFlag("market_allow_white_list_market");
}

bool TMarketExperiments::AllowBlackListMarket() const {
    return Ctx.HasExpFlag("market_allow_black_list_market");
}

bool TMarketExperiments::AdvBeruScenarioInHowMuch() const {
    return MarketBeru() && !Ctx.HasExpFlag("market_disable_beru_in_how_much");
}

bool TMarketExperiments::UseVoiceInBeruAdvInHowMuch() const {
    return Ctx.HasExpFlag("market_beru_in_how_much_with_voice");
}

bool TMarketExperiments::DefaultOffer() const {
    return Ctx.HasExpFlag("market_default_offer");
}

bool TMarketExperiments::ProductOffersCard() const {
    return Ctx.HasExpFlag("market_offers_card");
}

bool TMarketExperiments::ProductOffersCardOpenMarket() const {
    return Ctx.HasExpFlag("market_offers_card_open_market");
}

bool TMarketExperiments::UseExtendedWhiteGallery() const {
    return Ctx.HasExpFlag("market_ext_gallery")
        || Ctx.HasExpFlag("market_v2_gallery")
        || Ctx.HasExpFlag("market_v3_gallery");
}

bool TMarketExperiments::UseV2Gallery() const {
    return Ctx.HasExpFlag("market_v2_gallery");
}

bool TMarketExperiments::UseV3Gallery() const {
    return Ctx.HasExpFlag("market_v3_gallery");
}

bool TMarketExperiments::DisableListening() const {
    return Ctx.HasExpFlag("market_disable_listening");
}

bool TMarketExperiments::AddToCart() const {
    return Ctx.HasExpFlag("market_add_to_cart");
}

size_t TMarketExperiments::MaxOffersCount() const {
    size_t resCount = 0;

    TString base = "market_max_offers_count";
    for (size_t maxCount = 1; maxCount < 13; ++maxCount) {
        if (Ctx.HasExpFlag(base + ToString(maxCount))) {
            resCount = maxCount;
        }
    }

    return resCount;
}

bool TMarketExperiments::WithoutParametricSearch() const {
    return Ctx.HasExpFlag("market_without_parametric_search");
}

bool TMarketExperiments::WithoutShopsSearch() const {
    return Ctx.HasExpFlag("market_without_shops_search");
}

bool TMarketExperiments::ShopplingListFuzzy() const
{
    return !Ctx.HasExpFlag("shopping_list_fuzzy_off");
}

bool TMarketExperiments::ShoppingListDisableBeru() const {
    return Ctx.HasExpFlag("shopping_list_disable_beru");
}

bool TMarketExperiments::MdsPhrases() const {
    return Ctx.HasExpFlag("market_mds_phrases");
}

bool TMarketExperiments::MarketNativeAllowWhereBuy() const {
    return Ctx.HasExpFlag("market_native_allow_where_buy");
}

bool TMarketExperiments::MarketNativeAllowChoose() const {
    return Ctx.HasExpFlag("market_native_allow_choose");
}

bool TMarketExperiments::MarketNativeAllowWhereSearch() const {
    return Ctx.HasExpFlag("market_native_allow_search");
}

bool TMarketExperiments::MarketNativeAllowSearch() const {
    return Ctx.HasExpFlag("market_native_allow_search");
}

bool TMarketExperiments::MarketNativeAllowWish() const {
    return Ctx.HasExpFlag("market_native_allow_whish");
}

bool TMarketExperiments::MarketNativeDenyChooseToBuy() const {
    return Ctx.HasExpFlag("market_native_deny_choose_to_buy");
}

bool TMarketExperiments::HowMuchExtGallery() const {
    return Ctx.HasExpFlag("market_how_much_ext_gallery");
}

bool TMarketExperiments::HowMuchExtGalleryOpenMarket() const {
    return Ctx.HasExpFlag("market_how_much_ext_gallery_open_market");
}

bool TMarketExperiments::HowMuchExtGalleryAddButton() const {
    return Ctx.HasExpFlag("market_how_much_ext_gallery_add_button");
}

bool TMarketExperiments::HowMuchActivateProtocolScenarioByVins() const {
    return !Ctx.HasExpFlag("mm_disable_protocol_scenario=MarketHowMuch");
}

void TMarketExperiments::DetectExpVersion()
{
    Ctx.OnEachExpFlag([this](auto expFlag) {
        constexpr TStringBuf MARKET_EXP_FLAG_PREFIX = "market_exp=v";
        if (expFlag.StartsWith(MARKET_EXP_FLAG_PREFIX)) {
            const TStringBuf valueStr = expFlag.SubStr(MARKET_EXP_FLAG_PREFIX.size());
            ui32 value = 0;
            if (TryFromString(valueStr, value) && value > expVersion) {
                expVersion = value;
            }
        }
    });
    LOG(INFO) << TStringBuf("market_exp = v") << expVersion << Endl;
}

} // namespace NMarket

} // namespace NBASS
