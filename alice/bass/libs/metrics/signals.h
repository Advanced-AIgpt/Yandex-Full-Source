#pragma once

#include "fwd.h"

#include <util/generic/flags.h>
#include <util/generic/hash.h>
#include <util/generic/string.h>

#include <chrono>
#include <type_traits>

namespace NBASS {
namespace NMetrics {

using TMs = std::chrono::milliseconds;
using TUs = std::chrono::microseconds;

struct TSignalDescr {
    enum class EType {
        Unknown,
        Histogram,
    };
    enum class EFlag {
        UniStat = 1,
        Solomon = 2,
        /// Dynamic means that the Name is a prefix and it is a description of a group of signals.
        Dynamic = 4,
    };
    Y_DECLARE_FLAGS(EFlags, EFlag)

    const TString Name;
    const EType Type = EType::Unknown;
    EFlags Flags;
};

class TNoSignalException : public yexception {
};

class TSignals {
public:
    void Register(ICountersPlace& counters, TSignalDescr signal);

    const TSignalDescr* Get(TStringBuf name) const;

private:
    THashMap<TString, TSignalDescr> Signals;
};

Y_DECLARE_OPERATORS_FOR_FLAGS(TSignalDescr::EFlags)

} // namespace NMetrics
} // namespace NBASS
