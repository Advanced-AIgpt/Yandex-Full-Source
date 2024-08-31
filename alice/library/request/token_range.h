#pragma once

#include <util/digest/multi.h>
#include <util/generic/hash.h>

namespace NAlice {

struct TTokenRange {
    ui32 Start = 0;
    ui32 End = 0;

    bool operator==(const TTokenRange& rhs) const {
        return Start == rhs.Start && End == rhs.End;
    }

    ui32 Size() const {
        return End - Start;
    }
};

} // namespace NAlice

template <>
struct THash<NAlice::TTokenRange> {
    size_t operator()(const NAlice::TTokenRange& r) const {
        return MultiHash(r.Start, r.End);
    }
};
