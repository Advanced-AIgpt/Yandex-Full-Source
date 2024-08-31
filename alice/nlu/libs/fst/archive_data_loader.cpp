#include "archive_data_loader.h"

#include <util/generic/strbuf.h>
#include <util/generic/yexception.h>
#include <util/stream/input.h>

namespace NAlice {

    TArchiveDataLoader::TArchiveDataLoader(THolder<TArchiveReader> archiveReader)
        : ArchiveReader(std::move(archiveReader))
    {
        Y_ENSURE(ArchiveReader);
    }

    TBlob TArchiveDataLoader::LoadBlob(const TString& path) const {
        return ArchiveReader->BlobByKey(TStringBuf("/") + path);
    }

    THolder<IInputStream> TArchiveDataLoader::GetInputStream(const TString& path) const {
        return ArchiveReader->ObjectByKey(TStringBuf("/") + path);
    }

    bool TArchiveDataLoader::Has(const TString& path) const {
        return ArchiveReader->Has(TStringBuf("/") + path);
    }

} // namespace NAlice
