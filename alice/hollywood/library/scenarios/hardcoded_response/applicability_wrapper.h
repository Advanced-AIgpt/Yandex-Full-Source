#pragma once

#include <alice/hollywood/library/scenarios/hardcoded_response/proto/hardcoded_response.pb.h>

#include <alice/hollywood/library/request/fwd.h>

#include <alice/library/logger/logger.h>

#include <library/cpp/regex/pcre/regexp.h>

namespace NAlice::NHollywood {

namespace NImpl {
TString EncloseRegexp(const TStringBuf regexp);
TString AddBeginEndToRegexp(const TStringBuf regexp);
} // namespace NImpl

class TRequestApplicabilityWrapper {
public:
    TRequestApplicabilityWrapper(const THardcodedResponseFastDataProto::TApplicabilityInfo& responseInfo);

    bool IsApplicable(const TScenarioRunRequestWrapper& request, TRTLogger& logger) const;

private:
    THardcodedResponseFastDataProto::TApplicabilityInfo Proto;
    TMaybe<TRegExMatch> AppIdRegexp_;
};

} // namespace NAlice::NHollywood
