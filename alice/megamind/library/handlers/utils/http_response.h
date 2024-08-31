#pragma once

#include <alice/megamind/library/handlers/utils/analytics_logs_context_builder.h>

#include <alice/megamind/library/analytics/megamind_analytics_info.h>
#include <alice/megamind/library/context/context.h>
#include <alice/megamind/library/globalctx/fwd.h>
#include <alice/megamind/library/response/response.h>
#include <alice/megamind/library/util/http_response.h>
#include <alice/megamind/library/util/status.h>
#include <alice/megamind/library/walker/talkien.h>
#include <alice/megamind/library/walker/walker.h>

namespace NJson {
class TJsonValue;
} // namespace NJson

namespace NAlice::NMegamind {

class THttpResponseVisitor {
public:
    THttpResponseVisitor(TRequestCtx& requestCtx, IContext& ctx, IHttpResponse& response,
                         const TMegamindAnalyticsInfoBuilder& analyticsInfoBuilder,
                         const TQualityStorage& qualityStorage,
                         const TProactivityLogStorage& proactivityLogStorage,
                         IAnalyticsLogContextBuilder& analyticsLogContextBuilder);

    void operator()(const TDirectiveListResponse& response);
    void operator()(const TScenarioResponse& response);
    void operator()(const TError& error);
    void operator()(const TScenariosErrors& errors);

private:
    void ProcessResponse(const TSpeechKitResponseProto& response, HttpCodes httpCode, bool hideSensitiveData = false);

private:
    TRequestCtx& RequestCtx;
    IContext& Ctx;
    IHttpResponse& Response;
    const TMegamindAnalyticsInfoBuilder& AnalyticsInfoBuilder;
    const TQualityStorage& QualityStorage;
    const TProactivityLogStorage& ProactivityLogStorage;
    IAnalyticsLogContextBuilder& AnalyticsLogContextBuilder;
};

} // namespace NAlice::NMegamind
