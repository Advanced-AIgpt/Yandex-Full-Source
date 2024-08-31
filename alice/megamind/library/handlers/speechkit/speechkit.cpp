#include "speechkit.h"

#include <alice/megamind/library/handlers/utils/analytics_logs_context_builder.h>
#include <alice/megamind/library/handlers/utils/http_response.h>
#include <alice/megamind/library/response/utils.h>
#include <alice/megamind/library/response_meta/error.h>

#include <library/cpp/json/json_writer.h>

namespace NAlice::NMegamind {

namespace NImpl {

void AddYandexVinsOkHeaderIfNeeded(IHttpResponse& httpResponse, bool raiseErrorOnFailedScenarios) {
    if (httpResponse.HttpCode() == HTTP_UNASSIGNED_512 && !raiseErrorOnFailedScenarios) {
        httpResponse.AddHeader(THttpInputHeader{"x-yandex-vins-ok", "true"});
    }
}

} // namespace NImpl

TSpeechKitHandler::TSpeechKitHandler(TWalkerPtr walker)
    : Walker{walker}
{
}

void TSpeechKitHandler::HandleHttpRequest(TRequestCtx& requestCtx, IContext& ctx, IHttpResponse& httpResponse,
                                          const TWalkerResponse& response, IAnalyticsLogContextBuilder& logContextBuilder) const
{
    response.Visit(THttpResponseVisitor{requestCtx, ctx, httpResponse, response.AnalyticsInfoBuilder,
                                        response.QualityStorage, response.ProactivityLogStorage, logContextBuilder});

    NImpl::AddYandexVinsOkHeaderIfNeeded(httpResponse, ctx.HasExpFlag(EXP_RAISE_ERROR_ON_FAILED_SCENARIOS));

    httpResponse.Flush();
}

} // namespace NAlice::NMegamind
