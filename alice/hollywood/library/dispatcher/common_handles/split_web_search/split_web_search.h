#pragma once

#include <alice/hollywood/library/global_context/global_context.h>

#include <apphost/api/service/cpp/service.h>

namespace NAlice::NHollywood {

void SplitWebSearch(NAppHost::IServiceContext& ctx, TGlobalContext& globalContext);

} // namespace NAlice::NHollywood
