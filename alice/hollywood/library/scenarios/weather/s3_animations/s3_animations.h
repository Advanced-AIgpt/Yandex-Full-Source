#pragma once

#include <util/generic/maybe.h>
#include <util/generic/strbuf.h>

namespace NAlice::NScenarios {
class TDirective;
} // namespace NAlice::NScenarios

namespace NAlice::NHollywood::NWeather::NS3Animations {

TMaybe<TStringBuf> TryGetS3AnimationPathFromCondition(TStringBuf condition);

} // namespace NAlice::NHollywood::NWeather::NS3Animations
