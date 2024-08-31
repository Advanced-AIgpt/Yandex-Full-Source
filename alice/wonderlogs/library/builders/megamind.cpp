#include "megamind.h"

#include <alice/wonderlogs/protos/megamind_prepared.pb.h>

#include <alice/megamind/protos/common/events.pb.h>
#include <alice/megamind/protos/speechkit/directives.pb.h>
#include <alice/megamind/protos/speechkit/request.pb.h>
#include <alice/megamind/protos/speechkit/response.pb.h>

namespace {

const TString DEFER_APPLY_DIRECTIVE = "defer_apply";

} // namespace

namespace NAlice::NWonderlogs {

bool PreferableRequestResponse(
    const TMaybe<TUniproxyPrepared>& successfulUniproxyPrepared,
    const TMaybe<TMegamindPrepared::TMegamindRequestResponse>& successfulMegamindRequestResponse,
    const TMegamindPrepared::TMegamindRequestResponse& megamindRequestResponse) {
    {
        const bool matchedResponseId =
            successfulUniproxyPrepared &&
            megamindRequestResponse.GetResponseId() == successfulUniproxyPrepared->GetMegamindResponseId();
        if (matchedResponseId) {
            return true;
        }
    }

    const auto hasDeferApply = [](const auto& megamindRequestResponse) {
        return AnyOf(megamindRequestResponse.GetSpeechKitResponse().GetResponse().GetDirectives(),
                     [](const auto& directive) { return DEFER_APPLY_DIRECTIVE == directive.GetName(); });
    };

    if (successfulMegamindRequestResponse) {
        const bool matchedResponseId =
            successfulUniproxyPrepared &&
            successfulMegamindRequestResponse->GetResponseId() == successfulUniproxyPrepared->GetMegamindResponseId();
        if (matchedResponseId) {
            return false;
        }

        if (hasDeferApply(*successfulMegamindRequestResponse) && !hasDeferApply(megamindRequestResponse)) {
            return true;
        }

        {
            const auto succTime = successfulMegamindRequestResponse->GetSpeechKitRequest()
                                      .GetRequest()
                                      .GetAdditionalOptions()
                                      .GetServerTimeMs();
            const auto curTime =
                megamindRequestResponse.GetSpeechKitRequest().GetRequest().GetAdditionalOptions().GetServerTimeMs();
            if (succTime != curTime) {
                return succTime < curTime;
            }
        }
        return successfulMegamindRequestResponse->GetSpeechKitRequest().GetRequest().GetEvent().GetHypothesisNumber() <
               megamindRequestResponse.GetSpeechKitRequest().GetRequest().GetEvent().GetHypothesisNumber();
    }
    return true;
}

} // namespace NAlice::NWonderlogs
