#pragma once

#include "types/filter.h"
#include "types/promotions.h"

#include <alice/bass/forms/market/bass_response.sc.h>
#include <alice/bass/forms/market/client/bool_scheme_traits.h>
#include <alice/bass/forms/market/client/checkout.sc.h>
#include <alice/bass/forms/market/client/mds.sc.h>
#include <alice/bass/forms/market/client/report.sc.h>
#include <alice/bass/forms/market/slots.sc.h>

#include <alice/bass/libs/logging_v2/logger.h>

#include <library/cpp/scheme/domscheme_traits.h>
#include <library/cpp/scheme/util/scheme_holder.h>

#include <util/generic/maybe.h>
#include <util/generic/strbuf.h>
#include <util/string/cast.h>
#include <util/system/types.h>

namespace NBASS {

namespace NMarket {

const i32 PpCgiParam = 420;

const TStringBuf DEFAULT_CURRENCY = "RUR";

using TModelId = ui64;
using TPrice = double;
// todo MALISA-240 подумать над тем, чтоб использовать сразу TCgiParameters
using TCgiGlFilters = THashMap<TString, TVector<TString>>;
using TFormalizedGlFilters = TSchemeHolder<NBassApi::TFormalizedGlFilters<TBoolSchemeTraits>>;
using TFormUpdate = TSchemeHolder<NBassApi::TFormUpdate<TBoolSchemeTraits>>;
using TBeruOrderCardData = TSchemeHolder<NBassApi::TBeruOrderCardData<TBoolSchemeTraits>>;
using TWarningsScheme = NDomSchemeRuntime::TArray<TBoolSchemeTraits, NBassApi::TWarning<TBoolSchemeTraits>>;

using TAddressScheme = NBassApi::TAddress<TBoolSchemeTraits>;
using TAddressSchemeConst = NBassApi::TAddressConst<TBoolSchemeTraits>;
using TCheckouterData = TSchemeHolder<NBassApi::TCheckouterData<TBoolSchemeTraits>>;
using TDeliveryScheme = NBassApi::TDelivery<TBoolSchemeTraits>;
using TDeliveryOptionScheme = NBassApi::TDeliveryOption<TBoolSchemeTraits>;
using TStateCartScheme = NBassApi::TStateCart<TBoolSchemeTraits>;
using TCheckoutState = NBassApi::TMarketCheckoutState<TBoolSchemeTraits>;

namespace NSlots {
    using TProduct = TSchemeHolder<NBassApi::TProductSlot<TBoolSchemeTraits>>;
    using TChoicePriceScheme = NBassApi::TChoicePrice<TBoolSchemeTraits>;
    using TChoicePriceSchemeConst = NBassApi::TChoicePriceConst<TBoolSchemeTraits>;
} // namespace NSlots

const TPrice UNDEFINED_PRICE_VALUE = -1;

enum class ECheckouterPlatform {
    ALICE_SEARCH_IOS = 5,
    ALICE_SEARCH_ANDROID,
    ALICE_STATION,
    ALICE_WINDOWS,
    ALICE_BROWSER,

    UNKNOWN = -1,
};

enum class EClids {
    HOW_MUCH = 888,
    CHOICE_GREEN = 850,
    PRODUCT_DETAILS = 851,
    SEARCH_BY_PICTURE_IN_ALICE = 852,
    SEARCH_BY_PICTURE = 853,
    CHOICE_BLUE = 950,
    RECURRING_PURCHASE = 951,
    ORDERS_STATUS = 952,
    BERU_BONUSES = 953,
    SHOPPING_LIST = 954,
    OTHER = 0
};

enum class EScenarioType {
    HOW_MUCH              /* "market.how_much" */,
    CHOICE                /* "market.choice" */,
    RECURRING_PURCHASE    /* "market.recurring_purchase" */,
    ORDERS_STATUS         /* "market.orders_staus" */,
    BERU_BONUSES          /* "market.beru_my_bonuses_list" */,
    SHOPPING_LIST         /* "market.shopping_list" */,
    OTHER                 /* "market.other" */
};
constexpr EScenarioType DEFAULT_SCENARIO_TYPE = EScenarioType::HOW_MUCH;

enum class EMarketType {
    BLUE,
    GREEN
};
enum class EMarketGoodState {
    NEW        /* "new" */,
    CUTPRICE   /* "cutprice" */,
};
constexpr EMarketType DEFAULT_MARKET_TYPE = EMarketType::GREEN;

enum class EReferer {
    UPPER_BERU_ADV_IN_HOW_MUCH,
    LOWER_BERU_ADV_IN_HOW_MUCH,
};

class TCategory {
public:
    TCategory() : Hid(0) {}
    TCategory(ui64 hid) : Hid(hid) {}
    TCategory(ui64 hid, ui64 nid, TStringBuf slug, TStringBuf name = TStringBuf(""))
        : Hid(hid)
        , Nid(nid)
        , Slug(ToString(slug))
    {
        if (name) {
            Name = ToString(name);
        }
    }
    ui64 GetHid() const { return Hid; }
    bool DoesNidExist() const { return Nid.Defined(); }
    ui64 GetNid() const { return Nid.GetOrElse(0); }
    TStringBuf GetSlug() const { return Slug; }
    bool DoesNameExist() const { return Name.Defined(); }
    const TString& GetName() const { return Name.GetRef(); }

private:
    ui64 Hid;
    TMaybe<ui64> Nid = Nothing();
    TString Slug;
    TMaybe<TString> Name;
};

struct TQueryInfo {
    explicit TQueryInfo(TStringBuf query, const NSc::TValue price, const TCgiGlFilters& filters)
        : Query(query)
        , Price(price)
        , GlFilters(filters)
    {
    }

    TString Query;
    NSc::TValue Price;
    TCgiGlFilters GlFilters;
};

struct TRedirectCgiParams {
    TRedirectCgiParams() = default;
    TRedirectCgiParams(TStringBuf reportState, TStringBuf wasRedir)
        : ReportState(reportState)
        , WasRedir(wasRedir)
    {
    }

    TString ReportState = ""; // "rs" cgi parameter
    TString WasRedir = "";
};

struct TSkuOrderItem {
    TSkuOrderItem(ui64 sku, ui64 modelId)
        : Sku(sku)
        , ModelId(modelId)
    {
    }

    ui64 Sku;
    ui64 ModelId;
};

class TCheckouterOrder {
public:
    explicit TCheckouterOrder(const NSc::TValue& order)
    {
        Id = order["id"].GetIntNumber();
        ShopOrderId = order["shopOrderId"].GetString();
        Phone = order["buyer"]["phone"].GetString();
        DeliveryRegionId = order["delivery"]["regionId"].GetIntNumber();
        Status = order["status"].GetString();
        SubStatus = order["substatus"].GetString();
        DeliveryParcels = order["delivery"]["parcels"];
        HasСancellationRequest = order.Has("cancellationRequest");
        Fulfilment = order["fulfilmentl"].GetBool();
        DeliveryPartnerType = order["delivery"]["deliveryPartnerType"].GetString();

        auto deliveryType = order["delivery"]["type"].GetString();
        if (deliveryType == TStringBuf("DELIVERY")) {
            Address = order["delivery"]["buyerAddress"];
        } else if (deliveryType == TStringBuf("PICKUP")) {
            PickupAddress = order["delivery"]["address"];
            PickupOutletId = order["delivery"]["outletId"].GetIntNumber();
            PickupOutletName = ToString(order["delivery"]["outlet"]["name"].GetString());
        }
        for (const auto& item : order["items"].GetArray()) {
            OrderItems.push_back(TSkuOrderItem(item["sku"].ForceIntNumber(), item["modelId"].GetIntNumber()));
        }
    }

    ui64 GetId() const { return Id; }
    TStringBuf GetShopOrderId() const { return ShopOrderId; }
    TStringBuf GetPhone() const { return Phone; }
    TStringBuf GetStatus() const { return Status; }
    TStringBuf GetSubStatus() const { return SubStatus; }
    NSc::TValue GetDeliveryParcels() const { return DeliveryParcels; }
    bool IsCancelledByUser() const { return HasСancellationRequest; }
    bool IsFulfilment() const { return Fulfilment; }
    TStringBuf GetDeliveryPartnerType() const { return DeliveryPartnerType; }

    bool HasBuyerAddress() const { return Address.Defined(); }
    NSc::TValue GetAddress() const
    {
        Y_ASSERT(HasBuyerAddress());
        return Address ? *Address : NSc::TValue();
    }

    bool HasPickupAddress() const { return PickupAddress.Defined(); }
    NSc::TValue GetPickupAddress() const
    {
        Y_ASSERT(HasPickupAddress());
        return PickupAddress ? *PickupAddress : NSc::TValue();
    }
    i64 GetPickupOutletId() const
    {
        Y_ASSERT(HasPickupAddress());
        return PickupOutletId;
    }

    TStringBuf GetPickupOutletName() const
    {
        Y_ASSERT(HasPickupAddress());
        return PickupOutletName;
    }

    i64 GetDeliveryRegionId() const
    {
        return DeliveryRegionId;
    }

    const TVector<TSkuOrderItem>& GetOrderItems() const
    {
        return OrderItems;
    }

private:
    ui64 Id;
    TString ShopOrderId;
    TString Phone;
    TMaybe<NSc::TValue> Address = Nothing();
    TMaybe<NSc::TValue> PickupAddress = Nothing();
    i64 PickupOutletId;
    TString PickupOutletName;
    i64 DeliveryRegionId;
    TVector<TSkuOrderItem> OrderItems;
    TString Status;
    TString SubStatus;
    NSc::TValue DeliveryParcels;
    bool HasСancellationRequest;
    bool Fulfilment;
    TString DeliveryPartnerType;
};

class TCheckouterOrders {
public:
    explicit TCheckouterOrders(const NSc::TValue& orders)
    {
        LOG(INFO) << "TCheckouterOrders: " << orders << Endl;
        for (const auto& order : orders["orders"].GetArray()) {
            Orders.push_back(TCheckouterOrder(order));
        }
    }

    TVector<TSkuOrderItem> GetSkus() const
    {
        TVector<TSkuOrderItem> ordersItems;
        for (const auto& order : Orders) {
            ordersItems.insert(ordersItems.end(), order.GetOrderItems().begin(), order.GetOrderItems().end());
        }
        return ordersItems;
    }

    bool HasOrders() const { return !Orders.empty(); }

    const TVector<TCheckouterOrder>& GetOrders() const { return Orders; }

private:
    TVector<TCheckouterOrder> Orders;
};

struct TCheckoutResponse: public TCheckouterOrders {
public:
    explicit TCheckoutResponse(const NSc::TValue& orders)
        : TCheckouterOrders(orders)
    {
        CheckedOut = orders["checkedOut"].GetBool();
    }

    bool IsCheckedOut() const { return CheckedOut; }

private:
    bool CheckedOut;
};

struct TMuid {
    TMuid() {}
    explicit TMuid(const NSc::TValue& muid)
        : Muid(muid["muid"].GetString())
        , Cookie(muid["cookie"].GetString())
    {
    }

    NSc::TValue ToJson() const
    {
        NSc::TValue result;
        result["muid"] = Muid;
        result["cookie"] = Cookie;
        return result;
    }

    TString Muid;
    TString Cookie;
};

} // namespace NMarket

} // namespace NBASS
