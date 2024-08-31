#pragma once

#include "data_loader.h"

#include <util/folder/path.h>
#include <util/memory/blob.h>
#include <util/stream/fwd.h>

namespace NAlice {

    class TPrefixDataLoader : public IDataLoader {
    public:
        explicit TPrefixDataLoader(TFsPath baseDirectory);

        TBlob LoadBlob(const TString& path) const override;
        THolder<IInputStream> GetInputStream(const TString& path) const override;
        bool Has(const TString& path) const override;

    private:
        TFsPath BaseDirectory;
    };

} // namespace NAlice
