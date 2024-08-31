#pragma once

#include <util/generic/hash.h>
#include <util/system/types.h>
#include <util/system/yassert.h>

namespace NBASS {
namespace NSmallGeo {

class TVocabulary final {
public:
    ui64 AddGetId(TStringBuf key) {
        const auto it = Index.find(key);
        if (it != Index.end())
            return it->second;

        const ui64 index = Index.size();
        Index[key] = index;
        return index;
    }

    bool Has(TStringBuf key) const {
        return Index.contains(key);
    }

    ui64 GetId(TStringBuf key) const {
        Y_ASSERT(Has(key));
        const auto it = Index.find(key);
        return it->second;
    }

    void Clear() {
        Index.clear();
    }

private:
    THashMap<TString, ui64> Index;
};

} // namespace NSmallGeo
} // namespace NBASS
