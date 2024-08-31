#pragma once

#include <util/generic/flags.h>

namespace NAlice {

enum class ELogMessageType {
    All = 1ULL << 0,

    MegamindPreClasification = 1ULL << 1,
    MegamindPrepareScenarioRunRequests = 1ULL << 2,
    MegamindRetrieveScenarioRunResponses = 1ULL << 3,
};

Y_DECLARE_FLAGS(TLogMessageTypes, ELogMessageType);
Y_DECLARE_OPERATORS_FOR_FLAGS(TLogMessageTypes);

}  // namespace NAlice
