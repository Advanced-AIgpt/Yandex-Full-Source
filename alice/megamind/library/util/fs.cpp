#include "fs.h"

namespace NAlice::NMegamind {

TDirIterator ListFolder(const TString& folderPath) {
    return TDirIterator(folderPath, TDirIterator::TOptions(FTS_LOGICAL));
}

TVector<TFsPath> ListFilesInDirectory(const TString& folderPath) {
    TVector<TFsPath> filePaths{};
    for (const auto& filePath : ListFolder(folderPath)) {
        const TFsPath path = filePath.fts_path;
        if (!path.IsFile()) {
            continue;
        }
        filePaths.push_back(path);
    }
    return filePaths;
}

} // namespace NAlice::NMegamind
