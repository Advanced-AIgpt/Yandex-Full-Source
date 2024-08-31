#pragma once

#include <alice/bass/forms/context/fwd.h>
#include <alice/bass/util/error.h>

namespace NBASS::NTvCommon {

TResultValue StartOnboarding(TContext& ctx, const TStringBuf mode);
TResultValue ContinueOnboarding(TContext& ctx);

} // namespace NBASS::NTvCommon
