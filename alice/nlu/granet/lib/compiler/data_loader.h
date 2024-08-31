#pragma once

#include <util/folder/path.h>
#include <util/generic/string.h>
#include <util/stream/output.h>

namespace NGranet::NCompiler {

// ~~~~ IDataLoader ~~~~

// Special loader for hostile environments there TCompiler can't just load files mentioned in config.
class IDataLoader {
public:
    virtual ~IDataLoader() = default;

    virtual bool IsFile(const TFsPath& path) = 0;
    virtual TString ReadTextFile(const TFsPath& path) = 0;
};

// ~~~~ TFsDataLoader ~~~~

class TFsDataLoader : public IDataLoader {
public:
    bool IsFile(const TFsPath& path) override;
    TString ReadTextFile(const TFsPath& path) override;
};

// ~~~~ TResourceDataLoader ~~~~

class TResourceDataLoader : public NCompiler::IDataLoader {
public:
    bool IsFile(const TFsPath& path) override;
    TString ReadTextFile(const TFsPath& path) override;
};

void DumpResourceFs(IOutputStream* log);

} // namespace NGranet::NCompiler
