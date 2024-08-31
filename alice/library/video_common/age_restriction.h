#pragma once

#include <alice/library/restriction_level/restriction_level.h>

namespace NAlice::NVideoCommon {

struct TAgeRestrictionCheckerParams {
    bool IsAction = false;
    bool IsFromGallery = false;
    bool IsPlayerContinue = false;
    bool IsPornoGenre = false;
    bool IsPornoQuery = false;
    bool IsVideoByDescriptor = false;
    ui32 MinAge = 0;
    EContentRestrictionLevel RestrictionLevel = EContentRestrictionLevel::Medium;
};

bool IsPornoGenre(const TString& genre);

bool PassesAgeRestriction(const TAgeRestrictionCheckerParams& params);

} // namespace NAlice::NVideoCommon
