#pragma once

#include "client.h"
#include "types.h"

#include <alice/bass/libs/globalctx/globalctx.h>
#include <alice/bass/libs/scheduler/scheduler.h>

#include <library/cpp/threading/hot_swap/hot_swap.h>

#include <util/datetime/base.h>
#include <util/generic/noncopyable.h>
#include <util/generic/ptr.h>
#include <util/generic/singleton.h>
#include <util/system/spinlock.h>

namespace NBASS {

namespace NMarket {

template <typename TDataGetter>
class TDynamicDataHolder: public TThrRefBase, NNonCopyable::TNonCopyable {
    using TData = typename TDataGetter::TData;
    using TSelf = TDynamicDataHolder<TDataGetter>;
    using TSelfPtr = TIntrusivePtr<TSelf>;

public:
    static TSelfPtr Instance()
    {
        auto* ptr = GetSingletonHotSwapPtr();
        return ptr->AtomicLoad();
    }
    static void Start(IGlobalContext& globalCtx) {
        globalCtx.Scheduler().Schedule([&globalCtx]() { return TSelf::Update(globalCtx); });
    }
    static TDuration Update(IGlobalContext& globalCtx) {
        TSourcesRequestFactory sources(globalCtx.Sources(), globalCtx.Config());
        TDataGetter getter(sources);
        auto data = getter.GetData();
        if (data.Defined()) {
            auto* ptr = GetSingletonHotSwapPtr();
            ptr->AtomicStore(new TSelf(data.GetRef()));
        }
        return TDuration::Minutes(10);
    }

    const TData& GetData() const
    {
        return Data;
    }

private:
    TDynamicDataHolder(const TData& data)
        : Data(data)
    {
    }

    static THotSwap<TSelf>* GetSingletonHotSwapPtr()
    {
        return Singleton<THotSwap<TSelf>>();
    }

    TData Data;
};

using TStopWords = TDynamicDataHolder<TStopWordsGetter>;
using TStopCategories = TDynamicDataHolder<TMdsCategoriesGetter<ECategoriesType::Stop>>;
using TDeniedCategories = TDynamicDataHolder<TMdsCategoriesGetter<ECategoriesType::Denied>>;
using TAllowedCategories = TDynamicDataHolder<TMdsCategoriesGetter<ECategoriesType::Allowed>>;
using TAllowedCategoriesOnMarket = TDynamicDataHolder<TMdsCategoriesGetter<ECategoriesType::AllowedOnMarket>>;
using TDeniedCategoriesOnMarket = TDynamicDataHolder<TMdsCategoriesGetter<ECategoriesType::DeniedOnMarket>>;
using TPromotionsDynamicData = TDynamicDataHolder<TPromotionsGetter>;
using TPhrases = TDynamicDataHolder<TPhrasesGetter>;

class TDynamicDataFacade {
public:
    static bool IsFreeDeliveryDate(const TInstant& date = TInstant::Now());
    static const TPromotions::TVendorFreeDelivery* ParticipatesInVendorFreeDeliveryPromotion(
        ui32 offerVendorId,
        TInstant date = TInstant::Now());
    static const TPromotions::TSloboda::TProductFacts* GetFacts(TStringBuf product);
    static TVector<TStringBuf> GetFactProducts();
    static bool ContainsVulgarQuery(TStringBuf original);
    static bool IsSupportedCategory(ui64 hid);
    static bool IsSupportedCategory(const NSc::TArray& categories);
    static bool IsDeniedCategory(ui64 hid, EMarketType marketType, bool allowWhiteList, bool allowBlackList, bool isOnMarket, ui32 expVersion);
    static bool IsDeniedCategory(const NSc::TArray& categories, EMarketType marketType, bool allowWhiteList, bool allowBlackList, bool isOnMarket, ui32 expVersion);
    static TVector<TString> GetPhraseVariants(TStringBuf phraseName, ui32 expVersion);
};

} // namespace NMarket

} // namespace NBASS
