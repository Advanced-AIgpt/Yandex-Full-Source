#include "data_loader.h"
#include <alice/nlu/granet/lib/utils/string_utils.h>
#include <library/cpp/resource/resource.h>
#include <util/stream/file.h>

namespace NGranet::NCompiler {

// ~~~~ TFsDataLoader ~~~~

bool TFsDataLoader::IsFile(const TFsPath& path) {
    return path.IsFile();
}

TString TFsDataLoader::ReadTextFile(const TFsPath& path) {
    return TFileInput(path).ReadAll();
}

// ~~~~ TResourceDataLoader ~~~~

bool TResourceDataLoader::IsFile(const TFsPath& path) {
    TString dummy;
    return NResource::FindExact(path.GetPath(), &dummy);
}

TString TResourceDataLoader::ReadTextFile(const TFsPath& path) {
    TString text;
    Y_ENSURE(NResource::FindExact(path.GetPath(), &text), "Resource " + Cite(path) + " not found");
    return text;
}

void DumpResourceFs(IOutputStream* log) {
    Y_ENSURE(log);
    *log << "NResource::ListAllKeys:" << Endl;
    for (const TStringBuf& key : NResource::ListAllKeys()) {
        *log << "  " << key << Endl;
    }
}

} // namespace NGranet::NCompiler
