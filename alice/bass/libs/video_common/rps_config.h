#pragma once

#include <util/generic/hash.h>
#include <util/generic/strbuf.h>

namespace NVideoCommon {
class TRPSConfig {
public:
    static constexpr size_t DefaultMaxRPSValue = 400;

    explicit TRPSConfig(size_t defaultMaxRPS = DefaultMaxRPSValue)
        : DefaultMaxRPS(defaultMaxRPS) {
    }

    void AddRPSLimit(TStringBuf providerName, size_t maxRPS) {
        RPSOverrides[providerName] = maxRPS;
    }

    size_t GetRPSLimit(TStringBuf providerName) const {
        const size_t* maxRPS = RPSOverrides.FindPtr(providerName);
        return maxRPS ? *maxRPS : DefaultMaxRPS;
    }

private:
    size_t DefaultMaxRPS;
    THashMap<TStringBuf, size_t> RPSOverrides;
};
} // namespace NVideoCommon
