#pragma once

#include <alice/hollywood/library/scenarios/goodwin/handlers/action.pb.h>
#include <alice/megamind/protos/scenarios/response.pb.h>

#include <util/generic/string.h>

namespace NAlice {
namespace NFrameFiller {
namespace NGoodwin {

NScenarios::TFrameAction ToFrameAction(const TAction& action, const TString& actionName);

} // namespace NGoodwin
} // namespace NFrameFiller
} // namespace NAlice
