#pragma once

#include "base_client.h"

#include <alice/bass/forms/crmbot/order_status.h>

#include <alice/bass/forms/crmbot/clients/checkouter.sc.h>
#include <alice/bass/forms/market/client/bool_scheme_traits.h>

#include <alice/library/datetime/datetime.h>

#include <util/generic/vector.h>
#include <util/generic/serialized_enum.h>

namespace NBASS::NCrmbot {

class TCheckouterOrder : public TSchemeHolder<NBassApi::Crmbot::TCheckouterOrderInfoConst<NMarket::TBoolSchemeTraits>>
{
private:
    using Base = TSchemeHolder<NBassApi::Crmbot::TCheckouterOrderInfoConst<NMarket::TBoolSchemeTraits>>;
public:
    explicit TCheckouterOrder(const NSc::TValue& val) : Base(val) {}

    EOrderStatus Status() const {
        return FromString(TString{*Scheme()->Status()});
    }
    EOrderSubstatus Substatus() const {
        return FromString(TString{*Scheme()->Substatus()});
    }
    EDeliveryType DeliveryType() const {
        return FromString(TString{*Scheme()->Delivery()->Type()});
    }
    EOrderPaymentMethod PaymentMethod() const {
        return FromString(TString(*Scheme()->PaymentMethod()));
    }
    bool IsPaidWithThankyou() const;
    TInstant DeliveryDateTo() const;
    TInstant DeliveryDateFrom() const;
    bool HasCheckpoints(std::initializer_list<ECheckpointStatus> statuses) const;
    bool HasCheckpoints(std::initializer_list<EDSCheckpointStatus> statuses) const;
    TInstant MaxShipmentDateTime(bool& was_delayed) const;
    TStringBuf DeliveryServiceContact() const;

    static TInstant ParseDatetime(TStringBuf text, TStringBuf format="%d-%m-%Y %H:%M:%S");
private:
    TInstant SscanfDate(TStringBuf date) const {
        return ParseDatetime(date, "%d-%m-%Y");
    }
    TDuration SscanfTime(TStringBuf time) const {
        ui32 h = 0, m = 0;
        sscanf(time.data(), "%d:%d", &h, &m);
        return TDuration::Hours(h) + TDuration::Minutes(m);
    }
};

using TCheckouterOrders = TSchemeHolder<NBassApi::Crmbot::TCheckouterOrderInfos<NMarket::TBoolSchemeTraits>>;

using TCheckouterOrderChange = TSchemeHolder<NBassApi::Crmbot::TCheckouterOrderEvent<NMarket::TBoolSchemeTraits>>;

using TCheckouterCancellationRules = TSchemeHolder<NBassApi::Crmbot::TCheckouterCancellationRules<NMarket::TBoolSchemeTraits>>;

class TCheckouterClient: public TBaseClient {
private:
    using TMarketContext = NMarket::TMarketContext;
public:
    template <class TTypedResponse>
    using THttpResponse = NMarket::THttpResponse<TTypedResponse>;

    explicit TCheckouterClient(TMarketContext& context);

    THttpResponse<TCheckouterOrders> GetOrdersByUid(
        TStringBuf uid,
        const TVector<EOrderStatus>& statuses = {},
        TMaybe<TStringBuf> notesQuery = Nothing(),
        ui32 pageSize = 50);

    TMaybe<TCheckouterOrder> GetOrderById(TStringBuf orderId);

    TVector<TCheckouterOrderChange> GetOrderChanges(
        TStringBuf orderId,
        std::initializer_list<EOrderChangeType> eventTypes,
        bool showReturnStatuses = false,
        ui32 pageSize = 50);

    TCheckouterCancellationRules GetCancellationRules();

    TMaybe<TCheckouterOrder> CancelOrder(TStringBuf orderId, EOrderCancellationReason reason, TStringBuf notes = "");
private:
    TCgiParameters CreateCRMRobotCGI() const;
    void AddPagerCGI(TCgiParameters& cgi, int pageSize, int curPage) const;
    THashMap<TString, TString> CreateHeaders() const;
};

}

