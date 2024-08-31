#pragma once

#include <util/folder/path.h>

namespace NAlice::NShooter {

class TPerfDiff : public NNonCopyable::TNonCopyable {
public:
    TPerfDiff(const TFsPath& oldResponsesPath, const TFsPath& newResponsesPath, const TFsPath& outputPath);
    void ConstructDiff();
private:
    const TFsPath& OldResponsesPath_;
    const TFsPath& NewResponsesPath_;
    const TFsPath& OutputPath_;
};

} // namespace NAlice::NShooter
