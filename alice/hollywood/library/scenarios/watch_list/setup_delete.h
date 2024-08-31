#pragma once

#include "setup.h"

namespace NAlice::NHollywood::NWatchList {

    class TTvWatchListDeleteSetupHandle: public TTvWatchListSetupHandle {
    public:
        TString Name() const override;

    protected:
        NAppHostHttp::THttpRequest_EMethod HttpMethod() const override;
        TString ItemUuid(NAppHost::IServiceContext& serviceCtx) const override;
    };

} // namespace NAlice::NHollywood::NWatchList
