#pragma once

#include "structs.h"

#include <alice/megamind/protos/scenarios/iot.pb.h>

#include <library/cpp/scheme/scheme.h>

#include <util/generic/maybe.h>


namespace NAlice {

using TIotParseErrorMessage = TMaybe<TString>;

TIotParseErrorMessage AssembleTypedHypothesis(const NSc::TValue& input, NScenarios::TBegemotIotNluResult::THypothesis& result);

}  // namespace NAlice
