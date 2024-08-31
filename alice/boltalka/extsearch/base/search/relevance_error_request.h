#pragma once
#include "async_logger.h"
#include "unistat_registry.h"

#include <kernel/externalrelev/relev.h>

namespace NNlg {

class TNlgErrorHandlerRelevance : public TThrRefBase, public IRelevance {
public:
    explicit TNlgErrorHandlerRelevance(const TString& message);

    bool CalcFilteringResults(TCalcFilteringResultsContext& ctx) override;
    const IFactorsInfo* GetFactorsInfo() const override;
    void CalcFactors(TCalcFactorsContext& ctx) override;
    void CalcRelevance(TCalcRelevanceContext& ctx) override;

private:
    TString Message;
};

TNlgErrorHandlerRelevance* CreateNlgErrorHandlerRelevance(const TString& message, TAsyncLogger* logger, TUnistatRegistry* unistatRegistry);

} // namespace NNlg
