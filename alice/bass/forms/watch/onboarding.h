#pragma once

#include <alice/bass/forms/context/fwd.h>
#include <alice/bass/util/error.h>

namespace NBASS {
namespace NWatch {

TResultValue StartOnboarding(TContext& ctx, TStringBuf mode);
TResultValue ContinueOnboarding(TContext& ctx);

} // namespace NWatch
} // namespace NBASS
