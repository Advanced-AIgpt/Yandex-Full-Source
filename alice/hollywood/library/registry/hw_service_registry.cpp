#include "hw_service_registry.h"

#include <util/generic/set.h>

namespace NAlice::NHollywood {

namespace {

void CheckAllNamesDistinct(const TVector<IHwServiceHandlePtr>& handles) {
    TSet<TString> names{};
    for (const auto& handle : handles) {
        if (names.contains(handle->Name())) {
            ythrow yexception() << "Hw service names conflict: name " << handle->Name() << " occurs twice";
        }
        names.insert(handle->Name());
    }
}

} // namespace

THwServiceRegistry& THwServiceRegistry::Get() {
    return *Singleton<THwServiceRegistry>();
}

void THwServiceRegistry::AddHwServiceProducer(const TServiceProducer &producer) {
    Producers.push_back(producer);
}

const TVector<IHwServiceHandlePtr>& THwServiceRegistry::GetHandles() const {
    Y_ENSURE(Handles.size() == Producers.size());
    return Handles;
}

void THwServiceRegistry::CreateHandles(const THwServicesConfig& config, const TFsPath& resourcesBasePath) {
    Handles.reserve(Producers.size());
    for (const auto& producer : Producers) {
        auto handle = producer();
        handle->InitWithConfig(config, resourcesBasePath);
        Cerr << "Created " << handle->Name() << " hw service handle" << Endl;
        Handles.push_back(std::move(handle));
    }

    CheckAllNamesDistinct(Handles);
}

TServiceRegistrator::TServiceRegistrator(const TServiceProducer &producer) {
    THwServiceRegistry::Get().AddHwServiceProducer(producer);
}

} // namespace NAlice::NHollywood
