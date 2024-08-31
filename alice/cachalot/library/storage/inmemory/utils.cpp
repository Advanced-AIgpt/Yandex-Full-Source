#include <alice/cachalot/library/storage/inmemory/utils.h>


namespace NCachalot {

TUniqueClock::TUniqueClock()
    : LastTimestamp(TInstant::Now())
{
}

TInstant TUniqueClock::GetActualTimestamp() {
    LastTimestamp = Max(TInstant::Now(), LastTimestamp + TDuration::MicroSeconds(1));
    return LastTimestamp;
}

}  // namespace NCachalot
