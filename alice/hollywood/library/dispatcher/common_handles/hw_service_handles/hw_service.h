#pragma once

#include <alice/hollywood/library/base_hw_service/base_hw_service_handle.h>
#include <alice/hollywood/library/global_context/global_context.h>

#include <apphost/api/service/cpp/service.h>

namespace NAlice::NHollywood {

void DispatchHwServiceHandle(const IHwServiceHandle& handle,
                             TGlobalContext& globalContext,
                             NAppHost::IServiceContext& ctx);

} // namespace NAlice::NHollywood
