#include "relevance_error_request.h"

#include <alice/boltalka/extsearch/base/factors/factor_names.h>

#include <kernel/reqerror/reqerror.h>
#include <search/rank/srchdoc.h>

namespace NNlg {

TNlgErrorHandlerRelevance::TNlgErrorHandlerRelevance(const TString& message)
    : Message(message)
{
}

const IFactorsInfo* TNlgErrorHandlerRelevance::GetFactorsInfo() const {
    return NNlg::GetNlgFactorsInfo();
}

void TNlgErrorHandlerRelevance::CalcFactors(TCalcFactorsContext& /*ctx*/) {
}

void TNlgErrorHandlerRelevance::CalcRelevance(TCalcRelevanceContext& /*ctx*/) {
}

bool TNlgErrorHandlerRelevance::CalcFilteringResults(TCalcFilteringResultsContext& ctx) {
    ctx.Results->AnswerIsComplete = true;
    ctx.Results->YxErrCode = yxINCOMPATIBLE_REQ_PARAMS;
    ctx.Results->Warnings.push_back(Message);
    ctx.Results->NumDocs[0] = 0;
    return true;
}

TNlgErrorHandlerRelevance* CreateNlgErrorHandlerRelevance(const TString& message, TAsyncLogger* logger, TUnistatRegistry* unistatRegistry) {
    unistatRegistry->RequestCount->PushSignal(1);
    if (logger) {
        logger->Write("Error: " + message);
    }
    return new TNlgErrorHandlerRelevance(message);
}

} // namespace NNlg
