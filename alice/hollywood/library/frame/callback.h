#pragma once
#include "frame.h"
#include <alice/megamind/protos/scenarios/directives.pb.h>
#include <util/generic/maybe.h>
#include <util/generic/strbuf.h>

namespace NAlice {
namespace NHollywood {

constexpr TStringBuf FRAME_CALLBACK = "frame_callback";

TMaybe<TFrame> GetCallbackFrame(const NScenarios::TCallbackDirective* callback);

NScenarios::TCallbackDirective ToCallback(const TSemanticFrame& frame);

} // namespace NHollywood
} // namespace NAlice
