#pragma once

#include "resources.h"

#include <library/cpp/geobase/lookup.hpp>

#include <util/generic/maybe.h>

namespace NAlice::NHollywood {

class TGeobaseResource final : public IResourceContainer {
public:
    explicit TGeobaseResource(const bool lockMemoryMappedFile)
        : LockMemoryMappedFile_(lockMemoryMappedFile)
    {}

    const NGeobase::TLookup& GeobaseLookup() const {
        return *GeobaseLookup_;
    }

    void LoadFromPath(const TFsPath& dirPath) override;

private:
    bool LockMemoryMappedFile_;
    TMaybe<NGeobase::TLookup> GeobaseLookup_;
};

} // namespace NAlice::NHollywood
