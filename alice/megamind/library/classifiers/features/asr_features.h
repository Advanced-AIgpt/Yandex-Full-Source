#pragma once

#include <alice/megamind/library/context/context.h>
#include <alice/megamind/library/request/request.h>

#include <alice/megamind/protos/common/events.pb.h>

class TFactorStorage;

namespace NAlice {

void FillAsrFactors(const TEvent& event, const TRequest& request, const IContext::TExpFlags& expFlags, TFactorStorage& storage);

} // namespace NAlice
