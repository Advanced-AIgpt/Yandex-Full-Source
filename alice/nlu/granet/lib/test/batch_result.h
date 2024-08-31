#pragma once

#include <util/folder/path.h>

namespace NGranet {

void PrepareBatchResultDir(const TFsPath& batchResultsDir, TFsPath* prevResultDir,
    TFsPath* currResultDir, IOutputStream* log = nullptr);
void WriteBatchResultDiff(const TFsPath& prevResultDir, const TFsPath& currResultDir);

} // namespace NGranet
