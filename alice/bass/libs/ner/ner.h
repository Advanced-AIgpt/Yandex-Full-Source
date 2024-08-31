#pragma once

#include <alice/bass/libs/fetcher/request.h>

#include <library/cpp/scheme/scheme.h>

#include <util/generic/maybe.h>
#include <util/generic/strbuf.h>

namespace NBASS {

class TNerRequester {
public:
    TNerRequester(NHttpFetcher::TRequestPtr request, TStringBuf utterance, TStringBuf skillId);

    const NSc::TValue* Response();

private:
    NHttpFetcher::THandle::TRef Handle_;
    TMaybe<NSc::TValue> Response_;
};

} // namespace NBASS
