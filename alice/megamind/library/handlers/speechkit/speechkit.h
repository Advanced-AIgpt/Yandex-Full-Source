#pragma once

#include <alice/megamind/library/handlers/utils/analytics_logs_context_builder.h>

#include <alice/megamind/library/requestctx/requestctx.h>
#include <alice/megamind/library/response/response.h>
#include <alice/megamind/library/walker/scenario.h>

#include <util/generic/ptr.h>

namespace NAlice::NMegamind {

namespace NImpl {

void AddYandexVinsOkHeaderIfNeeded(IHttpResponse& httpResponse, bool raiseErrorOnFailedScenarios);

} // namespace NImpl

class TSpeechKitHandler : public TThrRefBase {
public:
    explicit TSpeechKitHandler(TWalkerPtr walker);

    void HandleHttpRequest(TRequestCtx& requestCtx, IContext& ctx, IHttpResponse& httpResponse,
                           const TWalkerResponse& response, IAnalyticsLogContextBuilder& logContextBuilder) const;

private:
    TWalkerPtr Walker;
};

} // namespace NAlice::NMegamind
