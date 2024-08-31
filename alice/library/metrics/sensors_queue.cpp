#include "sensors_queue.h"

using namespace NAlice::NMetrics;

void TSensorsQueue::Add(TMetric&& metric) {
    TGuard<TMutex> guard(Mutex);
    Metrics.push_back(std::move(metric));
}

TVector<TMetric> TSensorsQueue::StealMetrics() {
    TVector<TMetric> tmpMetrics;
    with_lock (Mutex) {
        DoSwap(tmpMetrics, Metrics);
    }
    return tmpMetrics;
}
