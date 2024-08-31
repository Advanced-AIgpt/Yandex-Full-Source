#pragma once

#include "source.h"

#include <alice/bass/libs/fetcher/neh.h>

#include <library/cpp/scheme/scheme.h>

#include <util/generic/maybe.h>

namespace NBASS {

class TAppHostInitContext {
public:
    void AddSourceInit(const IAppHostSource& appHostSource);
    NSc::TValue GetInitData() const;

private:
    NSc::TValue AppHostInitData;
};

class TAppHostResultContext {
public:
    explicit TAppHostResultContext(NSc::TValue resultsFromSource);
    TMaybe<NSc::TValue> GetItemRef(TStringBuf name) const;

private:
    NSc::TValue RawResponse;
};

} // NBASS
