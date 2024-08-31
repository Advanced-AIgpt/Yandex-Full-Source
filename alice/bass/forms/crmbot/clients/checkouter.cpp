#include "checkouter.h"

#include <util/datetime/base.h>

namespace {

void AddOrderChangesCGI(
    TCgiParameters& cgi,
    std::initializer_list<NBASS::NCrmbot::EOrderChangeType> eventTypes,
    bool showReturnStatuses)
{
    cgi.InsertUnescaped(TStringBuf("showReturnStatuses"), ToString(showReturnStatuses));
    for (const auto& type : eventTypes) {
        cgi.InsertUnescaped("eventType", ToString(type));
    }
}

// These numbers are public contact phone numbers so they change extremely rarely
THashMap<ui32, TString> deliveryServicePhones = {
    {},
};

}

namespace NBASS::NCrmbot {

inline constexpr TDuration CHECKOUT_ORDERS_HISTORY_DEPTH = TDuration::Days(60);

TInstant TCheckouterOrder::DeliveryDateTo() const
{
    const auto& dates = Scheme()->Delivery()->Dates();
    TInstant date = SscanfDate(dates->ToDate());
    if (dates->HasToTime()) {
        date += SscanfTime(dates->ToTime());
    }
    return date;
}

TInstant TCheckouterOrder::DeliveryDateFrom() const
{
    const auto& dates = Scheme()->Delivery()->Dates();
    TInstant date = SscanfDate(dates->FromDate());
    if (dates->HasFromTime()) {
        date += SscanfTime(dates->FromTime());
    }
    return date;
}

TInstant TCheckouterOrder::ParseDatetime(TStringBuf text, TStringBuf format)
{
    // assumes local timezone
    tm tm = {};
    std::istringstream ss(text.data());
    ss >> std::get_time(&tm, format.data());
    std::time_t t = std::mktime(&tm);
    return TInstant::Seconds(t);
}

bool TCheckouterOrder::HasCheckpoints(std::initializer_list<ECheckpointStatus> statuses) const
{
    for (const auto& parcel: Scheme()->Delivery()->Parcels()) {
        bool found = false;
        for (const auto& track : parcel->Tracks()) {
            for (const auto& checkpoint: track->Checkpoints()) {
                auto status = FromString<ECheckpointStatus>(checkpoint->Status());
                if (Find(statuses, status) != statuses.end()) {
                    found = true;
                    break;
                }
            }
            if (found) { break; }
        }
        if (!found) { return false; }
    }
    return true;
}

bool TCheckouterOrder::HasCheckpoints(std::initializer_list<EDSCheckpointStatus> statuses) const
{
    for (const auto& parcel: Scheme()->Delivery()->Parcels()) {
        bool found = false;
        for (const auto& track : parcel->Tracks()) {
            for (const auto& checkpoint: track->Checkpoints()) {
                auto status = static_cast<EDSCheckpointStatus>(checkpoint->DeliveryStatus().Get());
                if (Find(statuses, status) != statuses.end()) {
                    found = true;
                    break;
                }
            }
            if (found) { break; }
        }
        if (!found) { return false; }
    }
    return true;
}

TInstant TCheckouterOrder::MaxShipmentDateTime(bool& was_delayed) const
{
    // Get the latest shipment date
    TInstant date;
    was_delayed = false;
    for (const auto& parcel: Scheme()->Delivery()->Parcels()) {
        TInstant shipmentDate = ParseDatetime(parcel->ShipmentDate(), "%Y-%m-%d");
        date = std::max(shipmentDate, date);
        if (parcel->HasDelayedShipmentDate()) {
            was_delayed = true;
            TInstant delayedShipmentDate = ParseDatetime(parcel->DelayedShipmentDate(), "%Y-%m-%d");
            date = std::max(delayedShipmentDate, date);
        }
    }
    return date;
}

TStringBuf TCheckouterOrder::DeliveryServiceContact() const
{
    for (const auto& parcel: Scheme()->Delivery()->Parcels()) {
        for (const auto& track : parcel->Tracks()) {
            if (track->HasDeliveryServiceType() &&
                FromString<EDeliveryServiceType>(track->DeliveryServiceType()) == EDeliveryServiceType::CARRIER &&
                deliveryServicePhones.contains(track->DeliveryServiceId()))
            {
                return deliveryServicePhones[track->DeliveryServiceId()];
            }
        }
    }
    return TString{};
}

bool TCheckouterOrder::IsPaidWithThankyou() const
{
    for (const auto& partition: Scheme()->Payment()->Partitions()) {
        if (partition->PaymentAgent() == ToString(EOrderPaymentAgent::SBER_SPASIBO)) {
            return true;
        }
    }
    return false;
}

TCheckouterClient::TCheckouterClient(TMarketContext& ctx)
    : TBaseClient(ctx.GetSources(), ctx)
{
}

TCgiParameters TCheckouterClient::CreateCRMRobotCGI() const
{
    TCgiParameters cgi;
    cgi.InsertUnescaped(TStringBuf("clientId"), TStringBuf("6"));
    cgi.InsertUnescaped(TStringBuf("clientRole"), TStringBuf("CRM_ROBOT"));
    cgi.InsertUnescaped(TStringBuf("rgb"), TStringBuf("BLUE"));
    return cgi;
}

void TCheckouterClient::AddPagerCGI(TCgiParameters& cgi, int pageSize, int curPage) const
{
    cgi.InsertUnescaped(TStringBuf("pageSize"), ToString(pageSize));
    cgi.InsertUnescaped(TStringBuf("page"), ToString(curPage));
}

THashMap<TString, TString> TCheckouterClient::CreateHeaders() const
{
    THashMap<TString, TString> headers;
    headers["X-Hit-Rate-Group"] = "UNLIMIT";
    return headers;
}

TMaybe<TCheckouterOrder> TCheckouterClient::GetOrderById(TStringBuf orderId)
{
    const auto& source = Sources.MarketCheckouter(TString::Join("orders/", orderId));
    auto orderResponse =
        RunWithTrace<TCheckouterOrder>(
            "market_checkouter",
            source,
            CreateCRMRobotCGI(),
            NSc::TValue() /* body */,
            CreateHeaders()
        ).Wait();

    if (orderResponse.IsError()) {
        return Nothing();
    }
    else return orderResponse.GetResponse();
}

TCheckouterClient::THttpResponse<TCheckouterOrders> TCheckouterClient::GetOrdersByUid(
    TStringBuf uid,
    const TVector<EOrderStatus>& statuses,
    TMaybe<TStringBuf> notesQuery,
    ui32 pageSize)
{
    const auto& source = Sources.MarketCheckouterOrders(uid);

    TCgiParameters cgi = CreateCRMRobotCGI();
    if (notesQuery) {
        cgi.InsertEscaped(TStringBuf("notes"), *notesQuery);
    }
    cgi.InsertUnescaped(
        TStringBuf("fromDate"),
        (Now() - CHECKOUT_ORDERS_HISTORY_DEPTH).FormatLocalTime("%d-%m-%Y"));
    cgi.InsertUnescaped(TStringBuf("pageSize"), ToString(pageSize));
    for (const auto& status: statuses) {
        cgi.InsertUnescaped("status", ToString(status));
    }

    return RunWithTrace<TCheckouterOrders>("market_checkouter", source, cgi, NSc::TValue(), CreateHeaders()).Wait();
}

TVector<TCheckouterOrderChange> TCheckouterClient::GetOrderChanges(
    TStringBuf orderId,
    std::initializer_list<EOrderChangeType> eventTypes,
    bool showReturnStatuses,
    ui32 pageSize)
{
    using TAPIReturnType = TSchemeHolder<NBassApi::Crmbot::TCheckouterOrderEvents<NMarket::TBoolSchemeTraits>>;

    const auto& source = Sources.MarketCheckouter(TString::Join("orders/", orderId, "/events"));
    TVector<TCheckouterOrderChange> result = {};

    TCgiParameters cgi = CreateCRMRobotCGI();
    AddPagerCGI(cgi, pageSize, 1 /* curPage */);
    AddOrderChangesCGI(cgi, eventTypes, showReturnStatuses);

    auto headers = CreateHeaders();

    auto response = RunWithTrace<TAPIReturnType>("market_checkouter", source, cgi, NSc::TValue(), headers).Wait().GetResponse();
    int totalPages = response->Pager()->PagesCount();
    result.reserve(response->Pager()->Total());
    for (auto event: response->Events()) { result.push_back(TCheckouterOrderChange(*(event->GetRawValue()))); }

    TVector<NBASS::NMarket::TResponseHandle<TAPIReturnType>> handles;

    for (int curPage = 2; curPage <= totalPages; ++curPage) {
        TCgiParameters cgi = CreateCRMRobotCGI();
        AddPagerCGI(cgi, pageSize, curPage);
        AddOrderChangesCGI(cgi, eventTypes, showReturnStatuses);
        handles.push_back(RunWithTrace<TAPIReturnType>("market_checkouter", source, cgi, NSc::TValue(), headers));
    }

    for (auto& handle: handles) {
        auto response = handle.Wait().GetResponse();
        for (auto event: response->Events()) { result.push_back(TCheckouterOrderChange(*(event->GetRawValue()))); }
    }

    return result;
}

TCheckouterCancellationRules TCheckouterClient::GetCancellationRules()
{
    const auto& source = Sources.MarketCheckouter("orders/cancellation-substatuses");
    auto orderResponse =
        RunWithTrace<TCheckouterCancellationRules>(
            "market_checkouter",
            source,
            CreateCRMRobotCGI(),
            NSc::TValue() /* body */,
            CreateHeaders()
        ).Wait();
    return orderResponse.GetResponse();
}

TMaybe<TCheckouterOrder> TCheckouterClient::CancelOrder(
    TStringBuf orderId,
    EOrderCancellationReason reason,
    TStringBuf notes)
{
    const auto& source = Sources.MarketCheckouterHeavy(TString::Join("orders/", orderId, "/cancellation-request"));
    NSc::TValue body;
    body["substatus"] = ToString(reason);
    if (!notes.Empty()) {
        body["notes"] = notes;
    }
    auto orderResponse =
        RunWithTrace<TCheckouterOrder>(
            "market_checkouter",
            source,
            CreateCRMRobotCGI(),
            body,
            CreateHeaders()
        ).Wait();

    if (orderResponse.IsError()) {
        return Nothing();
    }
    else return orderResponse.GetResponse();
}

}
