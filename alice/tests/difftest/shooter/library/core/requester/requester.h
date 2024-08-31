#pragma once

#include <alice/tests/difftest/shooter/library/core/fwd.h>

#include <util/generic/string.h>
#include <util/folder/path.h>

namespace NAlice::NShooter {

struct TRequestResponse {
    TString Data;
    TDuration Duration;
    TFsPath OutputPath;
};

class IRequester {
public:
    virtual ~IRequester() = default;
    virtual TMaybe<TRequestResponse> Request(const TFsPath& path) const = 0;
};

} // namespace NAlice::NShooter
