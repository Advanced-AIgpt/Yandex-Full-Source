#pragma once

#include <alice/hollywood/library/config/config.pb.h>
#include <alice/hollywood/library/hw_service_context/context.h>

namespace NAlice::NHollywood {

using THwServicesConfig = TConfig_THwServicesConfig;

class IHwServiceHandle {
public:
    IHwServiceHandle() = default;
    virtual ~IHwServiceHandle() = default;

    virtual void Do(THwServiceContext& ctx) const = 0;
    virtual const TString& Name() const = 0;

    virtual void InitWithConfig(const THwServicesConfig& config, const TFsPath& resourcesBasePath) {
        Y_UNUSED(config, resourcesBasePath);
    }
};

} // namespace NAlice::NHollywood
