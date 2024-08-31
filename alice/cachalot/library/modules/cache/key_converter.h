#pragma once

#include <alice/cachalot/library/status.h>
#include <util/generic/strbuf.h>

namespace NCachalot {

    TString MakeResponseKeyFromRequest(TStringBuf requestKey);
    TString MakeFlagFromRequestKey(TStringBuf requestKey, EResponseStatus responseStatus);

} // namespace NCachalot
