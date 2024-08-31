#pragma once

#include <alice/library/logger/logger.h>

#include <google/protobuf/message.h>

#include <util/folder/path.h>
#include <util/generic/hash.h>
#include <util/generic/set.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/system/rwlock.h>

#include <functional>
#include <memory>
#include <typeinfo>

namespace NAlice::NHollywood {

class IFastData {
public:
    virtual ~IFastData() = default;
};

using TScenarioFastDataPtr = std::shared_ptr<IFastData>;
using TScenarioFastDataProtoPtr = std::shared_ptr<google::protobuf::Message>;

using TFastDataProducer = std::function<TScenarioFastDataPtr(const TScenarioFastDataProtoPtr)>;
using TFastDataProtoProducer = std::function<TScenarioFastDataProtoPtr()>;

using TFastDataInfo = std::pair<TString, std::pair<TFastDataProtoProducer, TFastDataProducer>>;

class TFastData {
public:
    explicit TFastData(const TString& dirPath);

    void Register(const TVector<TFastDataInfo>& fastDataProtoInfo);

    void Reload();

    int GetVersion() const;

    template <typename TParsedFastData>
    const std::shared_ptr<const TParsedFastData> GetFastData() {
        TScenarioFastDataPtr data;
        {
            TReadGuard lock(Mutex_);
            auto* ptr = FindIfPtr(FastData_, [](const TScenarioFastDataPtr& fastData) {
                const auto& tmp = *fastData;
                return typeid(tmp) == typeid(TParsedFastData);
            });
            if (ptr == nullptr) {
                ythrow yexception() << "Requesting missing fast_data" << Endl;
            }
            data = *ptr;
        }
        return std::dynamic_pointer_cast<TParsedFastData>(std::move(data));
    }

protected:
    // Allow access to fastdata for mock objects in unittests
    TVector<TScenarioFastDataPtr> FastData_;

private:
    const TFsPath DirPath_;
    THashMap<TString, std::pair<const TFastDataProtoProducer, const TFastDataProducer>> FastDataProducers_;

    int Version_ = 0;
    TRWMutex Mutex_;
};

}
