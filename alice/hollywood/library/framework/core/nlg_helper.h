#pragma once

#include "request.h"

#include <alice/hollywood/library/nlg/nlg.h>
#include <alice/hollywood/library/nlg/nlg_render_history.h>

#include <alice/library/json/json.h>

namespace NAlice::NHollywoodFw::NPrivate {

NHollywood::TNlgData ConstructNlgData(const TRequest& request, const NJson::TJsonValue& jsonContext,
                                      const NJson::TJsonValue& complexRoot);

} // namespace NAlice::NHollywoodFw::NPrivate
