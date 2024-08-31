#pragma once

#include "request_build.h"

#include <alice/megamind/library/requestctx/requestctx.h>
#include <alice/megamind/library/util/status.h>

namespace NAlice::NMegamind {

TStatus ParseSkRequest(TRequestCtx& requestCtx, TSpeechKitInitContext& initCtx);

} // namespace NAlice::NMegamind
