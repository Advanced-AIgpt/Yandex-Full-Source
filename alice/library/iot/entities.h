#pragma once

#include "structs.h"

#include <util/generic/vector.h>
#include <util/string/builder.h>


namespace NAlice::NIot {

struct TIoTEntitiesInfo {
    TVector<TRawEntity> Entities;
    TString NormalizedUtterance;
    int NTokens;
};

TVector<TRawEntity> ComputeFstEntities(const TVector<TString>& tokens, ELanguage language);

TVector<TRawEntity> ComputeNonsenseEntities(const TNluInput& nluInput, const TVector<TString>& tokens, ELanguage language);

}  // namespace NAlice::NIot
