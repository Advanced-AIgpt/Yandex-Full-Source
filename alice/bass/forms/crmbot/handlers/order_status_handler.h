#pragma once

#include "authorizing_handler.h"

#include <alice/bass/forms/crmbot/forms.h>

namespace NBASS::NCrmbot {

class TCheckouterOrder;

class TCrmbotContext;

class TOrderStatusHandlerMixin {
protected:
    using EForm = EOrderStatusForm;
    using EScenarioStatus = EOrderStatusScenarioStatus;
};

class TOrderStatusHandler : public TListingAuthorizingOrderFormHandler, protected TOrderStatusHandlerMixin {
public:
    static void Register(THandlersMap* handlers);

protected:
    void ChangeFormToScenarioEntry(TCrmbotContext& ctx) const override;

    void MakeSingleOrderResponse(TCrmbotContext& ctx, const TCheckouterOrder& order) const override;

private:
    void HandleComplexStatuses(const TCheckouterOrder& order, TCrmbotContext& ctx) const;
    void HandleDelivered(const NSc::TValue& contextData, TCrmbotContext& ctx) const;

    using TCheckouterDeliveryDatesRef = const NBassApi::Crmbot::TCheckouterDeliveryDatesConst<NMarket::TBoolSchemeTraits>&;
    bool AreDatesChanged(TCheckouterDeliveryDatesRef before, TCheckouterDeliveryDatesRef after) const;

    // The following function returns the delivery deadline chosen by user
    // either when placing the order or by changing it
    struct TDeliveryDeadline {
        const NSc::TValue* Deadline = nullptr;
        bool WasChanged = false;
        bool WasChangedByUser = true;  // was changed by user by placing the order and selecting it there
    };
    TDeliveryDeadline GetUserDeliveryDeadline(const TVector<TCheckouterOrderChange>& changes) const;
};

class TOrderStatusContinuationHandler : public TCrmbotFormHandler, protected TOrderStatusHandlerMixin {
public:
    TResultValue Do(TRequestHandler& r) override;

    static void Register(THandlersMap* handlers);
};

class TOrderStatusPickupWhereHandler : public TListingAuthorizingOrderFormHandler, protected TOrderStatusHandlerMixin {
public:
    static void Register(THandlersMap* handlers);

protected:
    void MakeSingleOrderResponse(TCrmbotContext& ctx, const TCheckouterOrder& order) const override;
    void ChangeFormToScenarioEntry(TCrmbotContext& ctx) const override;
};

}
