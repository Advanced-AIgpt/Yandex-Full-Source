#pragma once

#include <alice/library/logger/logger.h>
#include <alice/megamind/protos/scenarios/request.pb.h>

namespace NAlice {

bool CheckFeatureSupport(const NScenarios::TInterfaces& interfaces, const TStringBuf feature, TRTLogger& logger);

} // namespace NAlice
