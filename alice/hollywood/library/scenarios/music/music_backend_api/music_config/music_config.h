#pragma once

#include <alice/hollywood/library/framework/core/request.h>
#include <alice/hollywood/library/request/experiments.h>

#include <util/system/types.h>

namespace NAlice::NHollywood::NMusic {

struct TMusicConfig {
    i32 PageSize = 20;
    i32 HistorySize = 10;
    double ExplicitFilteredOutWarningRate = 0.25;
    i32 FindTrackIdxPageSize = 10000;
};

TMusicConfig CreateMusicConfig(const TExpFlags& flags);
TMusicConfig CreateMusicConfig(const NHollywoodFw::TRequest::TFlags& flags);

} // namespace NAlice::NHollywood::NMusic
