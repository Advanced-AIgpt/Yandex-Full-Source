#pragma once

#include <util/string/join.h>
#include <util/digest/city.h>

namespace NNlgTextUtils {
    ui64 CalculateContextHash(const TVector<TString>& context, const TString& salt) {
        return CityHash64(JoinSeq("\t", context) + salt);
    }
}
