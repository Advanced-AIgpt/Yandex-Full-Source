#include "prefix_data_loader.h"

#include <util/stream/file.h>

namespace NAlice {

    TPrefixDataLoader::TPrefixDataLoader(TFsPath baseDirectory)
        : BaseDirectory(std::move(baseDirectory))
    {
    }

    TBlob TPrefixDataLoader::LoadBlob(const TString& path) const {
        return TBlob::FromFile(JoinFsPaths(BaseDirectory, path));
    }

    THolder<IInputStream> TPrefixDataLoader::GetInputStream(const TString& path) const {
        return MakeHolder<TMappedFileInput>(JoinFsPaths(BaseDirectory, path));
    }

    bool TPrefixDataLoader::Has(const TString& path) const {
        auto fullPath = BaseDirectory;
        fullPath /= path;
        return fullPath.Exists();
    }

} // namespace NAlice
