#pragma once

#include <alice/nlu/libs/fst/data_loader.h>

#include <search/begemot/core/filesystem.h>

namespace NAlice {

    class TFileSystemDataLoader : public IDataLoader {
    public:
        explicit TFileSystemDataLoader(THolder<NBg::TFileSystem> fileSystem);

        TBlob LoadBlob(const TString& path) const override;
        THolder<IInputStream> GetInputStream(const TString& path) const override;
        bool Has(const TString& path) const override;

    private:
        THolder<NBg::TFileSystem> FileSystem;
    };

} // namespace NAlice
