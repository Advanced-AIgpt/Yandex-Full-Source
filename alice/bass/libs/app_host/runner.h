#pragma once

#include "context.h"

#include <alice/bass/libs/source_request/source_request.h>

#include <util/generic/noncopyable.h>

namespace NBASS {

class TAppHostRunner {
public:
    explicit TAppHostRunner(const TSourceRequestFactory& appHostGraph);
    TAppHostResultContext Fetch(const TAppHostInitContext& appHostContext);

private:
    const TSourceRequestFactory GraphSourceFactory;
};

} // NBASS
