#pragma once

#include "backend.h"
#include "settings.h"

#include <util/system/tls.h>
#include <util/generic/hash.h>
#include <util/digest/multi.h>


namespace NVoice {
namespace NMetrics {

struct TSensorKeyBad {
    TString Name;
    TString Code;
    TString Device;
    TString Surface;
    TString SubgroupName;
    TString AppId;
    TString ClientType;

    TSensorKeyBad(const TString& name, TStringBuf code, TLabels labels)
        : Name(name)
        , Code(code)
        , Device(labels.DeviceName)
        , Surface(labels.GroupName)
        , SubgroupName(labels.SubgroupName)
        , AppId(labels.AppId)
        , ClientType(labels.ClientType == EClientType::User ? "user" : (labels.ClientType == EClientType::Robot ? "robot" : ""))
    { }

    inline bool operator==(const TSensorKeyBad& other) const {
        return Name == other.Name
            && Code == other.Code
            && Device == other.Device
            && Surface == other.Surface
            && SubgroupName == other.SubgroupName
            && AppId == other.AppId
            && ClientType == other.ClientType
        ;
    }
};


struct TSensorKeyGood {
    size_t Hash_ { 0 };

    TSensorKeyGood(const TString& name, TStringBuf code, TLabels labels) {
        THash<TString> hasher;

        Hash_ = MultiHash(
            hasher(name),
            hasher(code),
            hasher(labels.DeviceName),
            hasher(labels.GroupName),
            hasher(labels.SubgroupName),
            hasher(labels.AppId),
            THash<size_t>()(static_cast<size_t>(labels.ClientType))
        );
    }

    inline bool operator==(const TSensorKeyGood& other) const {
        return Hash_ == other.Hash_;
    }

    inline bool operator<(const TSensorKeyGood& other) const {
        return Hash_ < other.Hash_;
    }

    inline bool operator>(const TSensorKeyGood& other) const {
        return Hash_ > other.Hash_;
    }

    inline bool operator<=(const TSensorKeyGood& other) const {
        return Hash_ <= other.Hash_;
    }

    inline bool operator>=(const TSensorKeyGood& other) const {
        return Hash_ >= other.Hash_;
    }
};

}   // namespace NMetrics
}   // namespace NVoice


template <>
class THash<NVoice::NMetrics::TSensorKeyBad> {
public:
    inline size_t operator()(const NVoice::NMetrics::TSensorKeyBad& key) {
        THash<TString> hasher;

        return MultiHash(
            hasher(key.Name),
            hasher(key.Code),
            hasher(key.Device),
            hasher(key.Surface),
            hasher(key.SubgroupName),
            hasher(key.AppId),
            hasher(key.ClientType)
        );
    }
};


template <>
class THash<NVoice::NMetrics::TSensorKeyGood> {
public:
    inline size_t operator()(const NVoice::NMetrics::TSensorKeyGood& key) {
        return key.Hash_;
    }
};


namespace NVoice {
namespace NMetrics {


using TSensorKey = TSensorKeyGood;


class TSensorStorage {
public:
    TSensorStorage() { }

    ~TSensorStorage();

    /**
     *  @brief get sensor
     *  TSensorStorage owns ISensor*, do not tryue
     */
    ISensor* GetSensor(const TString& name, TStringBuf code, TLabels labels) {
        TSensorKey key(name, code, labels);
        auto sensor = Sensors_.find(key);
        if (sensor != Sensors_.end()) {
            //return sensor->second.Get();
            return sensor->second;
        }
        return nullptr;
    }

    ISensor* AddSensor(const TString& name, TStringBuf code, TLabels labels, ISensorPtr ptr) {
        TSensorKey key(name, code, labels);
        Sensors_[key] = ptr;
        //return ptr.Get();
        return ptr;
    }

    void Reset() {
        Sensors_.clear();
    }

private:
    TMap<TSensorKey, ISensorPtr> Sensors_;
};  // class TSensorStorage


}   // namespace NMetrics
}   // namespace NVoice
