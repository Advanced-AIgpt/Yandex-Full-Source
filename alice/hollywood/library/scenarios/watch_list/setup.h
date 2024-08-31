#pragma once

#include <alice/hollywood/library/base_scenario/scenario.h>
#include <alice/hollywood/library/http_proxy/http_proxy.h>
#include <alice/hollywood/library/util/service_context.h>

namespace NAlice::NHollywood::NWatchList {

    class TTvWatchListSetupHandle: public TScenario::THandleBase {
    public:
        void Do(TScenarioHandleContext& ctx) const override;

    protected:
        virtual NAppHostHttp::THttpRequest_EMethod HttpMethod() const = 0;
        virtual TString ItemUuid(NAppHost::IServiceContext& serviceCtx) const = 0;

    private:
        NAlice::NHollywood::THttpProxyRequest SetupRequest(TScenarioHandleContext& ctx, const TString& uuid) const;
    };

} // namespace NAlice::NHollywood::NWatchList
