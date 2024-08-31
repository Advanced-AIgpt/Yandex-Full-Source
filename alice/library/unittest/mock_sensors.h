#pragma once

#include <alice/library/metrics/sensors.h>

#include <library/cpp/testing/gmock_in_unittest/gmock.h>

#include <util/datetime/base.h>
#include <util/generic/maybe.h>

namespace NAlice {

class TMockSensors : public NMetrics::ISensors {
public:
    MOCK_METHOD(void, AddRate, (NMonitoring::TLabels&&, i32), (override));
    MOCK_METHOD(void, IncRate, (NMonitoring::TLabels&&), (override));
    MOCK_METHOD(void, AddHistogram, (NMonitoring::TLabels&&, ui64, const TVector<double>& bins), (override));
    MOCK_METHOD(void, AddIntGauge, (NMonitoring::TLabels&&, i64), (override));
    MOCK_METHOD(void, SetIntGauge, (NMonitoring::TLabels&&, i64), (override));
};

struct TNoopSensors : public NMetrics::ISensors {
    void AddRate(NMonitoring::TLabels&& /* labels */, i32 /* delta */) override {
    }
    void IncRate(NMonitoring::TLabels&& /* labels */) override {
    }
    void AddHistogram(NMonitoring::TLabels&& /* labels */, ui64 /* value */, const TVector<double>& /* bins */) override {
    }
    void AddIntGauge(NMonitoring::TLabels&& /* labels */, i64 /* value */) override {
    }
    void SetIntGauge(NMonitoring::TLabels&& /* labels */, i64 /* value */) override {
    }
};

class TFakeSensors : public TNoopSensors {
public:
    struct TRateSensor {
        NMonitoring::TLabels Labels;
        i64 Value = 0;
    };

    const TRateSensor* FindFirstRateSensor(TStringBuf name, TStringBuf value) const;

    TString Print() const {
        TStringBuilder str;
        for (const auto& kv : RateCounters_) {
            str << "id: " << kv.first << ", ";
            str << kv.second.Labels << Endl;
        }

        return str;
    }

    // Overrides NMetrics::ISensors.
    void AddRate(NMonitoring::TLabels&& labels, i32 delta) override {
        GetRateSensor(labels).Value += delta;
    }

    void IncRate(NMonitoring::TLabels&& labels) override {
        ++GetRateSensor(labels).Value;
    }

    TMaybe<i64> RateCounter(NMonitoring::TLabels&& labels) const {
        const auto* sensor = RateCounters_.FindPtr(labels.Hash());
        return sensor ? TMaybe<i64>(sensor->Value) : Nothing();
    }

    // TODO Implment the rest overrides when needed!

private:
    TRateSensor& GetRateSensor(const NMonitoring::TLabels& labels);

private:
    // size_t here because NMonitoring::TLables::Hash() returns it!
    THashMap<size_t, TRateSensor> RateCounters_;
};

} // namespace NAlice
