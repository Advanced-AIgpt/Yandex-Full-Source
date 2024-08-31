#pragma once

#include <alice/hollywood/library/base_hw_service/base_hw_service_handle.h>
#include <alice/hollywood/library/config/config.pb.h>

#include <library/cpp/protobuf/util/pb_io.h>
#include <util/generic/vector.h>

#include <functional>

namespace NAlice::NHollywood {

using IHwServiceHandlePtr = std::unique_ptr<IHwServiceHandle>;
using TServiceProducer = std::function<IHwServiceHandlePtr()>;

class THwServiceRegistry {
public:
    static THwServiceRegistry& Get();

    void AddHwServiceProducer(const TServiceProducer& producer);
    void CreateHandles(const THwServicesConfig& config, const TFsPath& resourcesBasePath);
    [[nodiscard]] const TVector<IHwServiceHandlePtr>& GetHandles() const;

private:

    TVector<IHwServiceHandlePtr> Handles;
    TVector<TServiceProducer> Producers;
};

struct TServiceRegistrator {
    explicit TServiceRegistrator(const TServiceProducer& producer);
};

} // namespace NAlice::NHollywood

#define REGISTER_HOLLYWOOD_SERVICE(serviceClassName) \
static NAlice::NHollywood::TServiceRegistrator \
Y_GENERATE_UNIQUE_ID(HwServiceRegistrator)([]() -> std::unique_ptr<IHwServiceHandle> { \
return std::make_unique<serviceClassName>(); \
})
