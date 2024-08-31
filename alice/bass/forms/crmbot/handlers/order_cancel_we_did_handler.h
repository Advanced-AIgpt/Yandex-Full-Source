#pragma once

#include "authorizing_handler.h"

#include <alice/bass/forms/crmbot/forms.h>

namespace NBASS::NCrmbot {

class TOrderCancelWeDidHandlerMixin {
protected:
    using EForm = EOrderCancelWeDidForm;
    using EScenarioStatus = EOrderCancelWeDidScenarioStatus;
};

class TOrderCancelWeDidHandler : public TListingAuthorizingOrderFormHandler, protected TOrderCancelWeDidHandlerMixin {
public:
    static void Register(THandlersMap* handlers);

protected:
    void ChangeFormToScenarioEntry(TCrmbotContext& ctx) const override;

    void MakeSingleOrderResponse(TCrmbotContext& ctx, const TCheckouterOrder& order) const override;
};

class TOrderCancelWeDidContinuationHandler : public TCrmbotFormHandler, protected TOrderCancelWeDidHandlerMixin {
public:
    static void Register(THandlersMap* handlers);

    TResultValue Do(TRequestHandler& r) override;
};

}
