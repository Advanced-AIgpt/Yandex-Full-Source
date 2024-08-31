#pragma once

#include <alice/bass/libs/metrics/signals.h>

#include <library/cpp/monlib/metrics/labels.h>
#include <library/cpp/monlib/service/pages/mon_page.h>
#include <library/cpp/monlib/metrics/metric_registry.h>
#include <library/cpp/monlib/metrics/timer.h>
#include <library/cpp/unistat/unistat.h>

#include <util/generic/ptr.h>

#include <chrono>

namespace NMonitoring {

using TMetricRegistryPtr = TAtomicSharedPtr<TMetricRegistry>;

TMetricRegistry& GetSensors();
NMonitoring::THistogram& GetHistogram(const TString& name);
void SetSensorsSingleton(TMetricRegistryPtr sensors);

TString NormalizeMetricNameForGolovan(TStringBuf name);

class TBassGolovanCountersPage : public NMonitoring::IMonPage {
public:
    TBassGolovanCountersPage(const TString& path, TMetricRegistryPtr counters);

    void Output(NMonitoring::IMonHttpRequest& request) override;

private:
    TMetricRegistryPtr Sensors;
};

class TCountersChanger {
public:
    TCountersChanger(TMetricRegistry& sensors)
        : Sensors(sensors)
    {
    }

    static TCountersChanger DummyForTests() {
        static TMetricRegistry dummyRegistry;
        return TCountersChanger(dummyRegistry);
    }

    void Add(NMonitoring::TLabels labels, const TString& name, int delta);
    void Inc(NMonitoring::TLabels labels, const TString& name);
    //void Dec(NMonitoring::TLabels labels, const TString& name);

private:
    TMetricRegistry& Sensors;
};

struct TBassCounters : public TThrRefBase {
public:
    THashMap<TString, NUnistat::IHolePtr> UnistatHistograms;
    TAtomic Initialized;

public:
    TBassCounters() {
        AtomicSet(Initialized, 0);
    }

    void RegisterUnistatHistogram(TStringBuf name);
    void InitUnistat();

    /** This two functions are for hack to get rid of TApplication everywhere.
     * It must be removed when metrics.h is refactored.
     */
    static TBassCounters* Counters();
    static void SetCountersSingleton(TIntrusivePtr<TBassCounters> counters);

private:
    static TIntrusivePtr<TBassCounters> BassCounters_;
};

using TMsTimerScope = TMetricTimerScope<THistogram, std::chrono::milliseconds>;

} // namespace NMonitoring


// This one is actual counter, it always increases
#define Y_STATS_INC_INTEGER_COUNTER(name) \
    do { \
        NMonitoring::GetSensors().Counter({{"sensor", NMonitoring::NormalizeMetricNameForGolovan(name)}})->Inc(); \
    } while (false)

// This one is for determining rate, its real value depends on Solomon's and Golovan's update periods
#define Y_STATS_ADD_COUNTER(name, delta) \
    do { \
        NMonitoring::GetSensors().Rate({{"sensor", NMonitoring::NormalizeMetricNameForGolovan(name)}})->Add(delta); \
    } while (false)
#define Y_STATS_INC_COUNTER(name) \
    Y_STATS_ADD_COUNTER(name, 1)

#define Y_STATS_INC_COUNTER_IF(cond, name) \
    do { \
        if ((cond)) { \
            Y_STATS_INC_COUNTER(name); \
        } \
    } while(0)

#define Y_STATS_SCOPE_HISTOGRAM(name) \
    NMonitoring::TMsTimerScope Y_CAT(counter, __LINE__)((&NMonitoring::GetHistogram(name)))
