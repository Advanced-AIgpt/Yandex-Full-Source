#pragma once

#include <util/datetime/base.h>
#include <util/generic/map.h>
#include <util/generic/string.h>
#include <util/str_stl.h>

#include <type_traits>


namespace NCachalot {

template<typename T, size_t Salt>
struct THashWithSalt {
    size_t operator()(const T& value) const {
        return Salt ^ THash<T>()(value);
    }
};


class TUniqueClock {
public:
    TUniqueClock();
    TInstant GetActualTimestamp();

private:
    TInstant LastTimestamp;
};


template <typename T>
uint64_t GetMemoryUsage(const T& obj) {
    if constexpr (std::is_same_v<T, TString>) {
        return sizeof(TString) + obj.capacity();
    } else {
        return obj.GetMemoryUsage();
    }
}

}  // namespace NCachalot
