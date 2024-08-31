#pragma once

#include <util/generic/hash.h>
#include <util/generic/vector.h>
#include <util/generic/string.h>

namespace NGranet {

// ~~~~ TStringId ~~~~

// String index in pool (in TStringPool or TGrammarData::StringPool).
using TStringId = ui32;

// ~~~~ TStringPool ~~~~

class TStringPool {
public:
    TStringId Insert(TStringBuf str) {
        const auto [it, isNew] = StringToId.try_emplace(str, static_cast<TStringId>(Pool.size()));
        if (isNew) {
            Pool.push_back(TString{str});
        }
        return it->second;
    }

    const TString& GetString(TStringId id) const {
        return Pool[id];
    }

    TVector<TString> ReleasePool() {
        StringToId.clear();
        return std::move(Pool);
    }

private:
    TVector<TString> Pool;
    THashMap<TString, TStringId> StringToId;
};

} // namespace NGranet
