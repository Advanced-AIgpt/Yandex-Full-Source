#include "signals.h"

#include "metrics.h"
#include "place.h"

namespace NBASS {
namespace NMetrics {

void TSignals::Register(ICountersPlace& counters, TSignalDescr signal) {
    if (signal.Flags.HasFlags(TSignalDescr::EFlag::Dynamic | TSignalDescr::EFlag::UniStat)) {
        // because we have to register them but it is impossible
        ythrow yexception() << "Unistat signal can not be dynamic";
    }

    if (signal.Flags & TSignalDescr::EFlag::UniStat) {
        counters.BassCounters().RegisterUnistatHistogram(signal.Name);
    }

    Signals.emplace(signal.Name, signal);
}

const TSignalDescr* TSignals::Get(TStringBuf name) const {
    return Signals.FindPtr(name);
}

} // namespace NMetrics
} // namespace NBASS
