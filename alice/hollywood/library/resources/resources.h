#pragma once

#include <util/folder/path.h>

namespace NAlice::NHollywood {

class IResourceContainer {
public:
    virtual ~IResourceContainer() = default;
    virtual void LoadFromPath(const TFsPath& dirPath) = 0;
};

} // namespace NAlice::NHollywood
