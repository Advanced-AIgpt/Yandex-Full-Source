#pragma once

#include <alice/hollywood/library/config/config.pb.h>
#include <alice/hollywood/library/global_context/global_context.h>
#include <alice/hollywood/library/registry/hw_service_registry.h>
#include <alice/hollywood/library/registry/registry.h>

#include <apphost/api/service/cpp/service.h>

#include <util/generic/set.h>

namespace NAlice::NHollywood {

class TApphostDispatcher {
public:
    TApphostDispatcher(const TConfig_TAppHostConfig& config,
                       const TSet<TString>& scenarios,
                       const TScenarioRegistry& registry,
                       const THwServiceRegistry& serviceRegistry,
                       TGlobalContext& globalContext,
                       bool enableCommonHandles);

    void Dispatch();

private:
    NAppHost::TLoop Loop_;
    const TConfig_TAppHostConfig& Config_;
    TGlobalContext& GlobalContext_;
};

} // namespace NAlice::NHollywood
