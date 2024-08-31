#pragma once

#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NBASS {

bool IsoTranslit(TStringBuf value, TString& translit);

namespace NCall {

TVector<TString> FioTranslit(TStringBuf value);

} // namespace NCall
} // namespace NBASS
