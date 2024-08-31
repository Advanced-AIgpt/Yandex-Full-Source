#include "beggins.h"

#include <alice/hollywood/library/util/service_context.h>

namespace NAlice::NHollywoodFw {

const TString BEGGINS_ITEM = "datasource_BEGEMOT_BEGGINS_RESULT";

TMaybe<NBg::NProto::TAliceResponseResult> GetBegginsResponse(const TRequest& request) {
    return NHollywood::GetMaybeOnlyProto<NBg::NProto::TAliceResponseResult>(request.GetServiceCtx(), BEGGINS_ITEM);
}

} // namespace NAlice::NHollywoodFw
