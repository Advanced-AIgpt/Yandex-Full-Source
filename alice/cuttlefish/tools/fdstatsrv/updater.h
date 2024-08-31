#pragma once

#include "metrics.h"


class IMetricsUpdater {
public:
    virtual ~IMetricsUpdater() = default;

    virtual void UpdateMetrics(TMetrics& metrics) const = 0;
};
