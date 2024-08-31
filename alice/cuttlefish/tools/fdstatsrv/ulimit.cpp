#include "ulimit.h"

#include <sys/resource.h>


void TUlimitMetricsUpdater::UpdateMetrics(TMetrics& metrics) const {
    Y_UNUSED(metrics);

    struct rlimit data;
    if (!::getrlimit(RLIMIT_NOFILE, &data)) {
        metrics.SetFileDescriptorsMax(data.rlim_cur);
    }
}
