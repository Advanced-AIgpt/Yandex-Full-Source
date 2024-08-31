#pragma once

#include "consts.h"

#include <alice/bass/forms/context/context.h>
#include <alice/bass/forms/urls_builder.h>

#include <alice/library/datetime/datetime.h>

namespace NBASS::NWeather {

enum class ESuggestType {
    Verbose,
    Today,
    Tomorrow,
    AfterTomorrow,
    Weekend,
    SearchFallback,
    Onboarding,
    NowcastWhenEnds,
    NowcastWhenStarts,
    Feedback,
};

void WriteSuggests(TContext& ctx, TVector<ESuggestType> suggests, const NAlice::TDateTime::TSplitTime* time = nullptr,
                   const NSc::TValue* weatherJson = nullptr);

} // namespace NBASS::NWeather
