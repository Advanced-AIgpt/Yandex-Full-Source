#pragma once

#include <util/generic/ptr.h>
#include <util/system/mutex.h>
#include <util/system/rwlock.h>

#include "aggregation.h"


namespace NVoice {
namespace NMetrics {


enum class ESensorType {
    Gauge,
    Rate,
    Hist,
};


class ISensor {
public:
    virtual ~ISensor() { }

    virtual void Push(int64_t value) = 0;
};

using ISensorPtr = ISensor*;

}   // namesapce NMetrics
}   // namesapce NVoice


template <>
class THash<THolder<NVoice::NMetrics::ISensor>> {
public:
    inline size_t operator()(const THolder<NVoice::NMetrics::ISensor>& key) {
        return reinterpret_cast<size_t>(key.Get());
    }
};


namespace NVoice {
namespace NMetrics {


enum EOutputFormat {
    TEXT        /* "TEXT" */,
    JSON        /* "JSON" */,
    SPACK       /* "SPACK" */,
    SPACK_LZ4   /* "SPACK_LZ4" */,

    Max
};


class IBackend {
public:
    IBackend(TAggregationRules rules) : Rules_(std::move(rules)) { }

    virtual ~IBackend() { }

    virtual ISensorPtr CreateAbsoluteSensor(const TString& sensorName, TStringBuf code, TLabels labels) = 0;

    virtual ISensorPtr CreateRateSensor(const TString& sensorName, TStringBuf code, TLabels labels) = 0;

    virtual ISensorPtr CreateHistogramSensor(const TString& sensorName, TStringBuf code, TLabels labels) = 0;

    virtual TString    BuildSensorName(TStringBuf scope, TStringBuf sensor, TStringBuf code, TStringBuf backend, TLabels labels) = 0;

    virtual bool       SerializeMetrics(IOutputStream& stream, EOutputFormat format) = 0;

    virtual void       Reset() = 0;


    inline TLabels ApplyAggregationRules(const TClientInfo& info) {
        return Rules_.Process(info);
    }

    inline ISensorPtr RegisterSensor(ISensorPtr ptr) {
        with_lock(Guard_) {
            Sensors_.emplace(ptr);
            return ptr;
        }
    }

private:
    TAggregationRules           Rules_;
    THashSet<THolder<ISensor>>  Sensors_;
    TMutex                      Guard_;
};

using IBackendPtr = THolder<IBackend>;


}   // namespace NMetrics
}   // namespace NVoice
