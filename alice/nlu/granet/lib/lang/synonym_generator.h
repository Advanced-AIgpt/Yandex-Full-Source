#pragma once

#include "string_with_weight.h"
#include <util/generic/vector.h>
#include <util/generic/string.h>

namespace NGranet {

// TODO(samoylovboris) remove
TVector<TStringWithWeight> GenerateBuiltinSynonyms(TStringBuf original);

} // namespace NGranet
