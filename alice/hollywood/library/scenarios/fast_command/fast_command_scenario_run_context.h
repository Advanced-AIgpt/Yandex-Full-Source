#pragma once

#include <alice/library/logger/logger.h>
#include <alice/hollywood/library/request/request.h>
#include <alice/hollywood/library/response/response_builder.h>

namespace NAlice::NHollywood {

struct TFastCommandScenarioRunContext {
    TRTLogger& Logger;
    TRunResponseBuilder& RunResponseBuilder;
    const TScenarioRunRequestWrapper& Request;
};

}
