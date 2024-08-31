#pragma once

#include <alice/bass/libs/fetcher/fwd.h>

#include <util/generic/ptr.h>

class IScheduler;
class TConfig;

class IFormsDbDelegate {
public:
    virtual const TConfig& Config() const = 0;
    virtual IScheduler& Scheduler() = 0;

    virtual NHttpFetcher::TRequestPtr ExternalSkillDbRequest() const = 0;
};
