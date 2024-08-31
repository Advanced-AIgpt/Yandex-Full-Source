#pragma once

#include "client.h"
#include "context.h"
#include "number_filter_worker.h"

namespace NBASS {

namespace NMarket {

class TFilterWorker {
public:
    explicit TFilterWorker(TMarketContext&);
    void AddFormalizedFilters(const TFormalizedGlFilters& filters);
    void SetFilterExamples(const NSc::TArray& filters);
    bool TrySetFilterExample(const NSc::TValue& filter, TVector<NSc::TValue>& filterExamples);
    THashMap<TString, TString> ResolveFiltersNameAndValue(const NSc::TArray& glFilters);
    void UpdateFiltersDescription(const NSc::TArray& filters);

private:
    TMarketContext& Ctx;
    TNumberFilterWorker NumberFilterWorker;

    NSc::TValue GetBooleanBlockFilterExample(const TVector<const NSc::TValue*>& filters) const;
};

} // namespace NMarket

} // namespace NBASS
