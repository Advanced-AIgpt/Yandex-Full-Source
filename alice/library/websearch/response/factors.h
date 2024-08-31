#pragma once

#include <util/generic/hash.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>

namespace NAlice::NSearch {

using TFactorsMap = THashMap<TString, TString>;
TFactorsMap ParseFactors(TStringBuf relev);

} // namespace NAlice::NSearch
