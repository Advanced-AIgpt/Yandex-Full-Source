#include "market_orders_status.h"

#include "login.h"
#include "forms.h"
#include "types.h"
#include "context.h"
#include "settings.h"

#include "client/report_client.h"
#include "client/checkouter_client.h"

#include <alice/library/analytics/common/product_scenarios.h>

#include <util/generic/vector.h>

namespace NBASS {

namespace NMarket {

NSc::TValue ParcelStatusToTValue(TStringBuf status, bool isCommon) {
    NSc::TValue result;
    result["is_common"] = isCommon;
    result["status"] = status;

    return result;
}

void PushStatus(
    const NSc::TValue& track,
    NSc::TValue& allParcelsStatuses,
    TStringBuf status,
    THashMap<i32, TString>& checkpointStatusesMap
)
{
    auto& checkpoints = track["checkpoints"];
    bool pushed = false;

    if (checkpoints.ArraySize() > 0) {
        auto lastCheckpointId = checkpoints[checkpoints.ArraySize() - 1]["deliveryStatus"].GetIntNumber();
        auto lastCheckpointStatus = checkpointStatusesMap[lastCheckpointId];
        if (lastCheckpointStatus != "") {
            allParcelsStatuses.Push(ParcelStatusToTValue(lastCheckpointStatus, false));
            pushed = true;
        }
    }

    if (!pushed) {
        allParcelsStatuses.Push(ParcelStatusToTValue(status, true));
    }
}

THashSet<i32> GetAllCheckpointsId(const TCheckouterOrders& allOrders)
{
    THashSet<i32> result;

    for (const auto& currOrder: allOrders.GetOrders()) {
        const NSc::TValue& deliveryParcels = currOrder.GetDeliveryParcels();
        for (const auto& parcel: deliveryParcels.GetArray()) {
            for (const auto& track: parcel["tracks"].GetArray()) {
                if (track["deliveryServiceType"].GetString() == "CARRIER") {
                    auto& checkpoints = track["checkpoints"];
                    if (checkpoints.ArraySize() > 0) {
                        result.insert(checkpoints[checkpoints.ArraySize() - 1]["deliveryStatus"].GetIntNumber());
                    }
                }
            }
        }
    }

    return result;
}

void FillNotDeliveredOrder(
    NSc::TValue& deliveryParcels,
    NSc::TValue& currOrderToTValue,
    TStringBuf status,
    THashMap<i32, TString>& checkpointStatusesMap
)
{
    NSc::TValue allParcelsStatuses;
    allParcelsStatuses.SetArray();

    bool findCarrierTrack = false;
    for (const auto& parcel: deliveryParcels.GetArray()) {
        for (const auto& track: parcel["tracks"].GetArray()) {
            if (track["deliveryServiceType"].GetString() == "CARRIER") {
                findCarrierTrack = true;
                PushStatus(track, allParcelsStatuses, status, checkpointStatusesMap);
            }
        }
    }

    if (!findCarrierTrack) {
        allParcelsStatuses.Push(ParcelStatusToTValue(status, true));
    }

    currOrderToTValue["parcels_count_more_than_one"] = allParcelsStatuses.ArraySize() > 1;
    currOrderToTValue["parcel_status"] = allParcelsStatuses[0];
}

TMarketOrdersStatusImpl::TMarketOrdersStatusImpl(TMarketContext& ctx)
    : Ctx(ctx)
    , Form(FromString<EMarketOrdersStatusForm>(Ctx.FormName()))
    {}

TResultValue TMarketOrdersStatusImpl::Do()
{
    Ctx.Ctx().GetAnalyticsInfoBuilder().SetProductScenarioName(NAlice::NProductScenarios::MARKET_ORDERS_STATUS);
    TString uid = GetUid();
    if (uid.empty()) {
        return HandleGuest(Ctx, Form == EMarketOrdersStatusForm::Login);
    }

    TCheckouterClient checkouterClient(Ctx);
    TReportClient reportClient(Ctx);

    const auto& allOrders = checkouterClient.GetOrdersByUid(uid, MARKET_ORDERS_PAGE_SIZE).GetResponse();
    THashSet<i32> allIdForOrders = GetAllCheckpointsId(allOrders);
    THashMap<i32, TString> checkpointStatusesMap = reportClient.GetAllDeliveryStatuses(allIdForOrders);

    if (!allOrders.HasOrders()) {
        Ctx.AddTextCardBlock("market_order_status__user_have_not_orders");
        Ctx.AddOpenBlueSuggest();
    } else {
        NSc::TValue resOrdersData;

        auto& notDeliveredOrdersArr = resOrdersData["not_delivered_orders"].SetArray();

        for (const auto& currOrder: allOrders.GetOrders()) {
            NSc::TValue currOrderToTValue;
            currOrderToTValue["id"] = currOrder.GetId();
            TStringBuf status = currOrder.GetStatus();
            currOrderToTValue["shop_order_id"] =
                !currOrder.IsFulfilment() && currOrder.GetDeliveryPartnerType() != "YANDEX_MARKET"
                ? currOrder.GetShopOrderId()
                : "";
            if (status != "DELIVERED" && status != "CANCELLED" && !currOrder.IsCancelledByUser()) {
                NSc::TValue deliveryParcels = currOrder.GetDeliveryParcels();
                FillNotDeliveredOrder(deliveryParcels, currOrderToTValue, status, checkpointStatusesMap);
                notDeliveredOrdersArr.Push(currOrderToTValue);
            } else if (!resOrdersData.Has("last_delivered_order")) {
                currOrderToTValue["status"] =
                    currOrder.IsCancelledByUser()
                    ? "CANCEL_BY_USER"
                    : status;
                resOrdersData["last_delivered_order"] = currOrderToTValue;
            }
        }

        if (notDeliveredOrdersArr.ArrayEmpty()) {
            Ctx.AddTextCardBlock("market_order_status__user_have_orders_only_finished", resOrdersData);
        } else {
            Ctx.AddTextCardBlock("market_order_status__user_have_orders_unfinished", resOrdersData);
        }
    }

    return TResultValue();
}

TString TMarketOrdersStatusImpl::GetUid() const
{
    if (Ctx.HasUid()) {
        return ToString(Ctx.GetUid());
    }
    TCheckoutUserInfo user(Ctx);
    user.Init(true);
    return user.IsGuest() ? TString() : user.GetUid();
}

} // namespace NMarket

} // namespace NBASS
