#pragma once

#include "updater.h"


class TUlimitMetricsUpdater : public IMetricsUpdater {
public:
    TUlimitMetricsUpdater() = default;

    virtual void UpdateMetrics(TMetrics& metrics) const override;
};

