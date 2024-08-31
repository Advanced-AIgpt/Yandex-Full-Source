#pragma once

#include "updater.h"


class TFileHandlerMetricsUpdater : public IMetricsUpdater {
public:
    TFileHandlerMetricsUpdater() = default;

    virtual void UpdateMetrics(TMetrics& metrics) const override;
};

