#pragma once

#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/generic/hash.h>
#include <util/system/types.h>

namespace NVins {
    void SaveWordNnClassToIndicesMapping(const TString& fileName, const THashMap<TString, TVector<ui32>>& mapping);
} // namespace NVins
