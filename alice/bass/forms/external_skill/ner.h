#pragma once

#include <alice/bass/forms/context/fwd.h>

#include <alice/bass/libs/fetcher/request.h>

#include <library/cpp/scheme/scheme.h>

#include <util/generic/maybe.h>

namespace NBASS {
namespace NExternalSkill {

class TNerInfo {
public:
    void Request(TContext& ctx);
    TMaybe<NSc::TValue> Response() const;

private:
    NHttpFetcher::THandle::TRef Req;
};

} // namespace NBASS::NExternalSkill
} // namespace NBASS
