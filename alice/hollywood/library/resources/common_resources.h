#pragma once

#include "resources.h"

#include <alice/hollywood/library/config/config.pb.h>
#include <alice/library/logger/fwd.h>

#include <util/generic/hash.h>
#include <util/system/type_name.h>

#include <typeindex>
#include <typeinfo>
#include <type_traits>

namespace NAlice::NHollywood {

// Thrown when a requested resource wasn't found
class TCommonResourceNotFoundError final : public yexception {
};

// A registry for resources that should be available to all of the shard's scenarios.
class TCommonResources {
public:
    void AddResource(std::unique_ptr<IResourceContainer> resource) {
        auto* ptr = resource.get();
        Y_ENSURE(ptr);
        Resources_[std::type_index(typeid(*ptr))] = std::move(resource);
    }

    template <typename TResource>
    const TResource& Resource() const {
        if (const auto* ptr = Resources_.FindPtr(std::type_index(typeid(TResource)))) {
            Y_ENSURE(*ptr);
            const IResourceContainer* resource = ptr->get();
            const TResource* typedResource = dynamic_cast<const TResource*>(resource);
            Y_ENSURE(typedResource);
            return *typedResource;
        }

        ythrow TCommonResourceNotFoundError() << "Resource not found: " << TypeName<TResource>();
    }

private:
    THashMap<std::type_index, std::unique_ptr<IResourceContainer>> Resources_;
};

// Knows how to initialize resources requested in the config
TCommonResources LoadCommonResources(TRTLogger& logger, const TConfig& config);

} // namespace NAlice::NHollywood
