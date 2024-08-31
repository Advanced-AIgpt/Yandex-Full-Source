#pragma once

#include <util/generic/fwd.h>
#include <util/memory/blob.h>
#include <util/stream/fwd.h>

namespace NAlice {

    class IDataLoader {
    public:
        virtual ~IDataLoader() = default;

        virtual TBlob LoadBlob(const TString& path) const = 0;
        virtual THolder<IInputStream> GetInputStream(const TString& path) const = 0;
        virtual bool Has(const TString& path) const = 0;
    };

} // namespace NAlice
