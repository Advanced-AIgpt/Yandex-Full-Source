#pragma once

#include "vins.h"

namespace NBASS {

TResultValue FillContext(TContext::TPtr& context, NSc::TValue value, TGlobalContextPtr globalContext, NSc::TValue meta,
                         const TString& authHeader, const TString& appInfoHeader, const TString& fakeTimeHeader,
                         const TMaybe<TString>& userTicketHeader, const NSc::TValue& configPatch);

} // namespace NBASS
