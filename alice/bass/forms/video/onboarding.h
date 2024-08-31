#pragma once

#include <alice/bass/forms/context/fwd.h>

#include <alice/bass/util/error.h>

namespace NBASS {
namespace NVideo {

TResultValue StartOnboarding(TContext& ctx);
TResultValue ContinueOnboarding(TContext& ctx);

} // namespace NVideo
} // namespace NBASS
