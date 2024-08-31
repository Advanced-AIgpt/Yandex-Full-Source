#pragma once

#include "vins.h"

namespace NBASS {

class TTaxiOrderHandler : public IHandler {
public:
    TResultValue Do(TRequestHandler& r) override;
    static void Register(THandlersMap* handlers);

    static void AddTaxiSuggest(TContext& context, TStringBuf fromSlotName, TStringBuf toSlotName);

private:
    TResultValue DoImpl(TRequestHandler& r);
};

}
