#pragma once

#include "authorizing_handler.h"

#include <alice/bass/forms/crmbot/forms.h>

namespace NBASS::NCrmbot {

class TOrderCancelHandlerMixin {
protected:
    using EForm = EOrderCancelForm;
    using EScenarioStatus = EOrderCancelScenarioStatus;
};

class TOrderCancelHandler : public TListingAuthorizingOrderFormHandler, protected TOrderCancelHandlerMixin {
public:
    static void Register(THandlersMap* handlers);

protected:
    void ChangeFormToScenarioEntry(TCrmbotContext& ctx) const override;

    void MakeSingleOrderResponse(TCrmbotContext& ctx, const TCheckouterOrder& order) const override;

private:
    void HandleDelivered(TCrmbotContext& ctx, const TCheckouterOrder& order) const;
    void HandleCancelled(TCrmbotContext& ctx, const TCheckouterOrder& order) const;
};

class TOrderCancelReasonHandler : public TCrmbotFormHandler, protected TOrderCancelHandlerMixin {
public:
    static void Register(THandlersMap* handlers);

    TResultValue Do(TRequestHandler& r) override;
private:
    void SetReasonSelectedAndRespond(TCrmbotContext& ctx) const;
};

class TOrderCancelFinishHandler : public TCrmbotFormHandler, protected TOrderCancelHandlerMixin {
public:
    static void Register(THandlersMap* handlers);

    TResultValue Do(TRequestHandler& r) override;
};

}