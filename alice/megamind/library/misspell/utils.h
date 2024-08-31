#pragma once

#include <util/generic/maybe.h>
#include <util/generic/string.h>

namespace NAlice::NMisspell {

TMaybe<TString> CleanMisspellMarkup(TString text);

} // namespace NAlice::NMisspell
