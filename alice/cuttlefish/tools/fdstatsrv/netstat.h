#pragma once

#include "updater.h"


class TNetstatMetricsUpdater : public IMetricsUpdater {
public:
    TNetstatMetricsUpdater() = default;

    virtual void UpdateMetrics(TMetrics& metrics) const override;
};
