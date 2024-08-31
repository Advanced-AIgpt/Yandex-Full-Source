#pragma once

#include "data_loader.h"

#include <library/cpp/resource/resource.h>
#include <util/folder/path.h>

namespace NAlice {

class TResourceDataLoader : public NAlice::IDataLoader {
public:
    explicit TResourceDataLoader(TFsPath baseDir)
        : BaseDir(std::move(baseDir))
    {
    }

    TBlob LoadBlob(const TString& path) const override {
        return TBlob::FromString(NResource::Find(FullPath(path)));
    }

    THolder<IInputStream> GetInputStream(const TString& path) const override {
        return MakeHolder<TStringStream>(NResource::Find(FullPath(path)));
    }

    bool Has(const TString& path) const override {
        // We have to read (decompress) resource to check its existence.
        // TODO: remove IDataLoader::Has or add correspondent function to NResource.
        TString dummy;
        return NResource::FindExact(FullPath(path), &dummy);
    }

private:
    TString FullPath(const TString& path) const {
        return BaseDir / path;
    }

private:
    TFsPath BaseDir;
};

} // namespace NNlu
