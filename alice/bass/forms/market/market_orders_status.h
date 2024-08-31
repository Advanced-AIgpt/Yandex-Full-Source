#pragma once

#include "context.h"
#include "forms.h"

namespace NBASS {

namespace NMarket {

class TMarketOrdersStatusImpl {
public:
    TMarketOrdersStatusImpl(TMarketContext& ctx);
    TResultValue Do();

private:
    TString GetUid() const;

    TMarketContext& Ctx;
    EMarketOrdersStatusForm Form;
};

} // namespace NMarket

} // namespace NBASS
