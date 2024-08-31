#pragma once

#include "fwd.h"

namespace NMonitoring {
struct TBassCounters;
struct TDynamicCounters;
class TMonService2;
class TMetricRegistry;
} // namespace NMonitoring

namespace NBASS {
namespace NMetrics {

class ICountersPlace {
public:
    using TMonService = NMonitoring::TMonService2;

public:
    virtual ~ICountersPlace() = default;

    virtual NMonitoring::TMetricRegistry& Sensors() = 0;
    virtual NMonitoring::TMetricRegistry& SkillSensors() = 0;
    virtual NMonitoring::TBassCounters& BassCounters() = 0;

    virtual const TSignals& Signals() const = 0;

    virtual void Register(TMonService& service) = 0;
};

} // namespace NMetrics
} // namespace NBASS
