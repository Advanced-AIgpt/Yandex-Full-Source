#pragma once

#include "data_loader.h"

#include <library/cpp/archive/yarchive.h>

namespace NAlice {

    class TArchiveDataLoader : public IDataLoader {
    public:
        explicit TArchiveDataLoader(THolder<TArchiveReader> archiveReader);

        TBlob LoadBlob(const TString& path) const override;
        THolder<IInputStream> GetInputStream(const TString& path) const override;
        bool Has(const TString& path) const override;

    private:
        THolder<TArchiveReader> ArchiveReader;
    };

} // namespace NAlice
