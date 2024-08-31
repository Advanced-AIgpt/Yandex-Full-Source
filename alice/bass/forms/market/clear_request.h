#pragma once

#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NBASS {

namespace NMarket {

TString ClearRequest(
    TStringBuf original,
    const TVector<TString>& removePhrases);

TString ClearRequests(
    TStringBuf original,
    const TVector<TString>& removePhrases,
    const TVector<std::pair<TString, TString>>& replacePhrases);

} // namespace NMarket

} // namespace NBASS
