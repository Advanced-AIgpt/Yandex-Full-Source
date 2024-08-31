#include "file_system_loader.h"

namespace NAlice {

    TFileSystemDataLoader::TFileSystemDataLoader(THolder<NBg::TFileSystem> fileSystem)
        : FileSystem{std::move(fileSystem)}
    {
        Y_ENSURE(FileSystem, "fileSystem cannot be nullptr");
    }

    TBlob TFileSystemDataLoader::LoadBlob(const TString& path) const {
        return FileSystem->LoadBlob(path);
    }

    THolder<IInputStream> TFileSystemDataLoader::GetInputStream(const TString& path) const {
        return FileSystem->LoadInputStream(path);
    }

    bool TFileSystemDataLoader::Has(const TString& path) const {
        return FileSystem->Exists(path);
    }

} // namespace NAlice
