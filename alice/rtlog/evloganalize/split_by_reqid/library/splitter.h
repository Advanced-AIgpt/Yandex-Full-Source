#pragma once

#include <util/folder/path.h>

namespace NAlice::NSplitter {

class TSplitter {
public:
    TSplitter(TFsPath savePath, TVector<TFsPath> logPaths);
    void Split();

private:
    const TFsPath SavePath_;
    const TVector<TFsPath> LogPaths_;
};

} // namespace NAlice::NSplitter
