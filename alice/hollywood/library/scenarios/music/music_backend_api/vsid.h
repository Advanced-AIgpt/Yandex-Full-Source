#pragma once

#include <alice/library/util/rng.h>

#include <util/generic/string.h>
#include <util/datetime/base.h>

namespace NAlice::NHollywood::NMusic {

TString MakeHollywoodVsid(IRng& rng, TInstant ts);

} // namespace NAlice::NHollywood::NMusic
