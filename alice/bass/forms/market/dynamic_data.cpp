#include "dynamic_data.h"

namespace NBASS {

namespace NMarket {

bool TDynamicDataFacade::IsFreeDeliveryDate(const TInstant& date)
{
    const auto promotionsInstance = TPromotionsDynamicData::Instance();
    if (!promotionsInstance) {
        LOG(DEBUG) << "Can not check promotions" << Endl;
        return false;
    }
    const auto interval = promotionsInstance->GetData().FreeDeliveryInterval;
    return interval.Defined() && interval->From < date && date < interval->To;
}

const TPromotions::TVendorFreeDelivery* TDynamicDataFacade::ParticipatesInVendorFreeDeliveryPromotion(
    ui32 offerVendorId,
    TInstant date)
{
    const auto promotionsInstance = TPromotionsDynamicData::Instance();
    if (!promotionsInstance) {
        LOG(DEBUG) << "Can not check promotions" << Endl;
        return nullptr;
    }
    const auto& promotions = promotionsInstance->GetData();
    if (!promotions.FreeDeliveryByVendor.contains(offerVendorId)) {
        return nullptr;
    }
    const auto& promo = promotions.FreeDeliveryByVendor.at(offerVendorId);
    if (promo.Interval.From < date && date < promo.Interval.To) {
        return &promo;
    }
    return nullptr;
}

const TPromotions::TSloboda::TProductFacts* TDynamicDataFacade::GetFacts(TStringBuf product)
{
    const auto promotionsInstance = TPromotionsDynamicData::Instance();
    if (!promotionsInstance) {
        LOG(DEBUG) << "Can not check promotions" << Endl;
        return nullptr;
    }
    const auto& promotions = promotionsInstance->GetData();
    return promotions.Sloboda.GetFacts(product);
}

TVector<TStringBuf> TDynamicDataFacade::GetFactProducts()
{
    const auto promotionsInstance = TPromotionsDynamicData::Instance();
    if (!promotionsInstance) {
        LOG(DEBUG) << "Can not check promotions" << Endl;
        return {};
    }
    const auto& promotions = promotionsInstance->GetData();
    return promotions.Sloboda.GetFactNames();
}

bool TDynamicDataFacade::ContainsVulgarQuery(TStringBuf original)
{
    const auto stopWords = TStopWords::Instance();
    if (!stopWords) {
        LOG(DEBUG) << "Can not check stop words" << Endl;
        return false;
    }
    for (const auto& it : StringSplitter(original).Split(' ').SkipEmpty()) {
        if (stopWords->GetData().contains(it.Token())) {
            return true;
        }
    }
    return false;
}

bool TDynamicDataFacade::IsSupportedCategory(ui64 hid)
{
    const auto stopCategories = TStopCategories::Instance();
    if (!stopCategories) {
        LOG(DEBUG) << "Can not check stop categories" << Endl;
        return true;
    }
    return !(stopCategories->GetData().GetData().contains(hid));
}

bool TDynamicDataFacade::IsSupportedCategory(const NSc::TArray& categories)
{
    const auto stopCategories = TStopCategories::Instance();
    if (!stopCategories) {
        LOG(DEBUG) << "Can not check stop categories" << Endl;
        return true;
    }
    for (const auto& category : categories) {
        auto hid = category["id"].GetIntNumber();
        if (stopCategories->GetData().GetData().contains(hid)) {
            return false;
        }
    }
    return true;
}

bool TDynamicDataFacade::IsDeniedCategory(ui64 hid, EMarketType marketType, bool allowWhiteList, bool allowBlackList, bool isOnMarket, ui32 expVersion)
{
    if (marketType != EMarketType::GREEN) {
        return false;
    }
    if (allowWhiteList) {
        const bool allowedCategoriesLoaded = isOnMarket
            ? static_cast<bool>(TAllowedCategoriesOnMarket::Instance())
            : static_cast<bool>(TAllowedCategories::Instance());
        if (allowedCategoriesLoaded) {
            const auto& allowedCategoriesData = isOnMarket
                ? TAllowedCategoriesOnMarket::Instance()->GetData().GetData(expVersion)
                : TAllowedCategories::Instance()->GetData().GetData(expVersion);
            if (!allowedCategoriesData.empty()) {
                return !allowedCategoriesData.contains(hid);
            }
        } else {
            LOG(DEBUG) << "Can not check allowed categories" << Endl;
        }
    }
    if (allowBlackList) {
        const bool deniedCategoriesLoaded = isOnMarket
            ? static_cast<bool>(TDeniedCategoriesOnMarket::Instance())
            : static_cast<bool>(TDeniedCategories::Instance());
        if (deniedCategoriesLoaded) {
            const auto& deniedCategoriesData = isOnMarket
                ? TDeniedCategoriesOnMarket::Instance()->GetData().GetData(expVersion)
                : TDeniedCategories::Instance()->GetData().GetData(expVersion);
            return deniedCategoriesData.contains(hid);
        } else {
            LOG(DEBUG) << "Can not check denied categories" << Endl;
        }
    }
    return false;
}

static bool hasCategories(const NSc::TArray& categories, const THashSet<ui64>& typedCategories)
{
    for (const auto& category : categories) {
        auto hid = category["id"].GetIntNumber();
        if (typedCategories.contains(hid)) {
            return true;
        }
    }
    return false;
}

bool TDynamicDataFacade::IsDeniedCategory(
    const NSc::TArray& categories,
    EMarketType marketType,
    bool allowWhiteList,
    bool allowBlackList,
    bool isOnMarket,
    ui32 expVersion)
{
    if (marketType != EMarketType::GREEN) {
        return false;
    }
    if (allowWhiteList) {
        const bool allowedCategoriesLoaded = isOnMarket
            ? static_cast<bool>(TAllowedCategoriesOnMarket::Instance())
            : static_cast<bool>(TAllowedCategories::Instance());
        if (allowedCategoriesLoaded) {
            const auto& allowedCategoriesData = isOnMarket
                ? TAllowedCategoriesOnMarket::Instance()->GetData().GetData(expVersion)
                : TAllowedCategories::Instance()->GetData().GetData(expVersion);
            if (!allowedCategoriesData.empty()) {
                return !hasCategories(categories, allowedCategoriesData);
            }
        } else {
            LOG(DEBUG) << "Can not check allowed categories" << Endl;
        }
    }
    if (allowBlackList) {
        const bool deniedCategoriesLoaded = isOnMarket
            ? static_cast<bool>(TDeniedCategoriesOnMarket::Instance())
            : static_cast<bool>(TDeniedCategories::Instance());
        if (deniedCategoriesLoaded) {
            const auto& deniedCategoriesData = isOnMarket
                ? TDeniedCategoriesOnMarket::Instance()->GetData().GetData(expVersion)
                : TDeniedCategories::Instance()->GetData().GetData(expVersion);
            return hasCategories(categories, deniedCategoriesData);
        } else {
            LOG(DEBUG) << "Can not check denied categories" << Endl;
        }
    }
    return false;
}

TVector<TString> TDynamicDataFacade::GetPhraseVariants(TStringBuf phraseName, ui32 expVersion)
{
    const auto& phrases = TPhrases::Instance();
    if (!phrases) {
        return {};
    }
    const auto& phrasesData = phrases->GetData().GetData(expVersion)["phrases"];
    if (!phrasesData.Has(phraseName)) {
        return {};
    }
    const NSc::TArray& phraseVariants = phrasesData[phraseName].GetArray();
    TVector<TString> result(Reserve(phraseVariants.size()));
    for (const auto& variant : phraseVariants) {
        result.push_back(variant.ForceString());
    }
    return result;
}

} // namespace NMarket

} // namespace NBASS
