#pragma once

#include <alice/hollywood/library/framework/core/request.h>

namespace NAlice::NHollywoodFw {

struct TRenderBassBlockContext {
    const NHollywoodFw::TRequest& Request;
    TStringBuf NlgTemplate;
    const NJson::TJsonValue& NlgContext;
    NScenarios::TScenarioResponseBody& ResponseBody;
};

} // namespace NAlice::NHollywood
