#pragma once

#include "context.h"

namespace NBASS {

namespace NMarket {

class TNumberFilterWorker {
public:
    TNumberFilterWorker(TMarketContext& ctx);
    void GetAmountInterval(NSc::TValue& amountFrom, NSc::TValue& amountTo, bool needRange) const;
    void GetAmountInterval(TMaybe<double>& amountFrom, TMaybe<double>& amountTo, bool needRange) const;
    void UpdateFilter(TStringBuf filterId);

private:
    TMarketContext& Ctx;
};

} // namespace NMarket

} // namespace NBASS
