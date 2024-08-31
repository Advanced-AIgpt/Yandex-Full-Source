#include "age_restriction.h"

namespace NAlice::NVideoCommon {

namespace {

constexpr ui32 MAX_FAMILY_SEARCH_AGE = 15;
constexpr ui32 MAX_SAFE_SEARCH_AGE = 6;

} // namespace

bool IsPornoGenre(const TString& genre) {
    // FIXME (@a-sidorin, @vi002): genre checking may fail because we don't have provider genre handling.
    return genre.Contains("porn") || genre.Contains("порн")
        || genre.Contains("erot") || genre.Contains("эрот");
}

bool PassesAgeRestriction(const TAgeRestrictionCheckerParams& params){
    if (params.IsFromGallery || params.IsVideoByDescriptor) {
        return true;
    }

    switch (params.RestrictionLevel) {
        case EContentRestrictionLevel::Safe:
            return params.MinAge <= MAX_SAFE_SEARCH_AGE && !params.IsPornoGenre;
        case EContentRestrictionLevel::Children:
            return params.MinAge <= MAX_FAMILY_SEARCH_AGE && !params.IsPornoGenre;
        case EContentRestrictionLevel::Medium:
            return params.IsPlayerContinue || !params.IsPornoGenre || params.IsPornoQuery || params.IsAction;
        case EContentRestrictionLevel::Without:
            return true;
    }
}

} // namespace NAlice::NVideoCommon
