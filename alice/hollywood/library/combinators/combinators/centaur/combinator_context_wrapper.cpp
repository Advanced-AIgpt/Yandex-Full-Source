#include "combinator_context_wrapper.h"


namespace NAlice::NHollywood::NCombinators {

using namespace NAlice::NScenarios;

TCombinatorContextWrapper::TCombinatorContextWrapper(
    THwServiceContext& Ctx,
    const TCombinatorRequestWrapper& Request,
    NScenarios::TScenarioRunResponse& ResponseForRenderer)
    : Ctx_(Ctx),
      Request_(Request),
      ResponseForRenderer_(ResponseForRenderer),
      Logger_(Ctx_.Logger())
{
}

THwServiceContext& TCombinatorContextWrapper::Ctx() {
    return Ctx_;
}
const TCombinatorRequestWrapper& TCombinatorContextWrapper::Request() {
    return Request_;
}
NScenarios::TScenarioRunResponse& TCombinatorContextWrapper::ResponseForRenderer() {
    return ResponseForRenderer_;
}
TRTLogger& TCombinatorContextWrapper::Logger() {
    return Logger_; 
}

}
