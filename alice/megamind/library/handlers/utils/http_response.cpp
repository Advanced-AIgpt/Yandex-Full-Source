#include "http_response.h"

#include <alice/megamind/library/apphost_request/protos/analytics_logs_context.pb.h>
#include <alice/megamind/library/experiments/flags.h>
#include <alice/megamind/library/globalctx/globalctx.h>
#include <alice/megamind/library/response/utils.h>
#include <alice/megamind/library/response_meta/error.h>
#include <alice/megamind/library/sensors/utils.h>
#include <alice/megamind/library/speechkit/request.h>

#include <dj/lib/proto/action.pb.h>

#include <alice/library/analytics/common/names.h>
#include <alice/library/experiments/flags.h>
#include <alice/library/metrics/names.h>
#include <alice/library/metrics/sensors.h>
#include <alice/library/version/version.h>

#include <alice/wonderlogs/sdk/utils/speechkit_utils.h>

#include <library/cpp/http/server/response.h>
#include <library/cpp/json/json_value.h>
#include <library/cpp/protobuf/json/proto2json.h>
#include <library/cpp/svnversion/svnversion.h>

#include <util/generic/ptr.h>
#include <util/generic/utility.h>

namespace NAlice::NMegamind {
namespace {

void CheckIncorrectStateOnExternalButton(const NJson::TJsonValue& response, NMetrics::ISensors& sensors) {
    for (auto& object : response["response"]["meta"].GetArray()) {
        if (object["type"].GetString() == "error" &&
            object["error_type"].GetString() == "incorrect_state_on_external_button") {
            sensors.IncRate(NSignal::INCORRECT_STATE_ON_EXTERNAL_BUTTON_ERRORS);
            break;
        }
    }
}

void CheckEmptySessionInExternalSkill(const TSpeechKitRequest& skr, NMetrics::ISensors& sensors) {
    if (!skr->GetHeader().GetDialogId().Empty() && skr->GetSession().Empty()) {
        sensors.IncRate(NSignal::EMPTY_SESSIONS);
    }
}

} // namespace

namespace NImpl {

TScenarioResponseVisitor MakeResponseVisitor(const TSpeechKitRequest& speechKitRequest,
                                             const TMegamindAnalyticsInfoBuilder& analyticsInfoBuilder,
                                             const TQualityStorage& qualityStorage,
                                             const TProactivityLogStorage& proactivityLogStorage) {
    const auto hasAnalyticsInfo = speechKitRequest.HasExpFlag(NExperiments::ANALYTICS_INFO);
    const auto hasProactivityLogStorage = speechKitRequest.HasExpFlag(NExperiments::PROACTIVITY_LOG_STORAGE);
    return TScenarioResponseVisitor{hasAnalyticsInfo ? &analyticsInfoBuilder : nullptr,
                                    &qualityStorage,
                                    hasProactivityLogStorage ? &proactivityLogStorage : nullptr};
}

} // namespace NImpl

THttpResponseVisitor::THttpResponseVisitor(TRequestCtx& requestCtx, IContext& ctx, IHttpResponse& response,
                                           const TMegamindAnalyticsInfoBuilder& analyticsInfoBuilder,
                                           const TQualityStorage& qualityStorage,
                                           const TProactivityLogStorage& proactivityLogStorage,
                                           IAnalyticsLogContextBuilder& analyticsLogContextBuilder)
    : RequestCtx{requestCtx}
    , Ctx{ctx}
    , Response{response}
    , AnalyticsInfoBuilder{analyticsInfoBuilder}
    , QualityStorage{qualityStorage}
    , ProactivityLogStorage{proactivityLogStorage}
    , AnalyticsLogContextBuilder{analyticsLogContextBuilder}
{
    analyticsLogContextBuilder.SetAnalyticsInfo(AnalyticsInfoBuilder.BuildProto());
    analyticsLogContextBuilder.SetQualityStorage(qualityStorage);
}

void THttpResponseVisitor::operator()(const TDirectiveListResponse& response) {
    const auto skr = Ctx.SpeechKitRequest();
    auto visitor = NImpl::MakeResponseVisitor(skr, AnalyticsInfoBuilder, QualityStorage, ProactivityLogStorage);
    ProcessResponse(visitor(response), HTTP_OK);
}

void THttpResponseVisitor::operator()(const TScenarioResponse& response) {
    const auto skr = Ctx.SpeechKitRequest();
    auto visitor = NImpl::MakeResponseVisitor(skr, AnalyticsInfoBuilder, QualityStorage, ProactivityLogStorage);
    bool hideSensitiveData = CheckResponseSensitivity(response) &&
                             !Ctx.HasExpFlag(EXP_DEBUG_SHOW_SENSITIVE_DATA);
    ProcessResponse(
        visitor(response),
        response.GetHttpCode().GetOrElse(HTTP_OK),
        hideSensitiveData);
}

void THttpResponseVisitor::ProcessResponse(const TSpeechKitResponseProto& skResponse,
                                           HttpCodes httpCode, bool hideSensitiveData)
{
    const auto skr = Ctx.SpeechKitRequest();

    auto& sensors = Ctx.Sensors();

    AnalyticsLogContextBuilder.SetHttpCode(httpCode);
    AnalyticsLogContextBuilder.SetHideSensitiveData(hideSensitiveData);
    AnalyticsLogContextBuilder.SetResponse(skResponse);
    AnalyticsLogContextBuilder.SetProactivityLogStorage(ProactivityLogStorage);

    const auto responseJson = SpeechKitResponseToJson(skResponse);

    const auto statusCode = static_cast<int>(httpCode);
    if (httpCode != HTTP_OK) {
        NMonitoring::TLabels labels;
        labels.Add(NSignal::HTTP_CODE, ToString(statusCode));
        IncErrorOnTestIds(sensors, skr->GetRequest().GetTestIDs(),
                          ETestIdErrorType::HTTP_ERROR, labels);
    }

    CheckIncorrectStateOnExternalButton(responseJson, sensors);
    CheckEmptySessionInExternalSkill(skr, sensors);

    const auto contentType = RequestCtx.ContentType();
    switch (contentType) {
        case TRequestCtx::TRequestMeta::Json:
            Response.SetHttpCode(httpCode)
                    .SetContentType(NContentTypes::APPLICATION_JSON)
                    .SetContent(JsonToString(responseJson));
            break;

        case TRequestCtx::TRequestMeta::Protobuf: {
            TString binaryProto;
            if (skResponse.SerializeToString(&binaryProto)) {
                Response.SetHttpCode(httpCode)
                        .SetContentType(NContentTypes::APPLICATION_PROTOBUF)
                        .SetContent(binaryProto);
            } else {
                Response.SetHttpCode(HTTP_INTERNAL_SERVER_ERROR)
                        .SetContentType(NContentTypes::TEXT_PLAIN)
                        .SetContent("Unable to serialize response protobuf");
            }
            break;
        }

        default:
            Response.SetHttpCode(HTTP_BAD_REQUEST)
                    .SetContentType(NContentTypes::TEXT_PLAIN)
                    .SetContent(TStringBuilder{} << "Unable to make response for content-type: '"
                                                 << NMegamindAppHost::TRequestMeta_EContentType_Name(contentType)
                                                 << "': int value: " << static_cast<int>(contentType));
            break;
    }
}

void THttpResponseVisitor::operator()(const TError& error) {

    LOG_ERROR(Ctx.Logger()) << error.ErrorMsg;
    AnalyticsLogContextBuilder.SetHttpCode(HttpCodes::HTTP_INTERNAL_SERVER_ERROR);
    AnalyticsLogContextBuilder.SetHideSensitiveData(false);
    AnalyticsLogContextBuilder.SetResponse(error);
    AnalyticsLogContextBuilder.SetProactivityLogStorage(ProactivityLogStorage);

    TErrorMetaBuilder{error}.ToHttpResponse(Response, /* flush= */ false);
}

void THttpResponseVisitor::operator()(const TScenariosErrors& errors) {
    LOG_ERROR(Ctx.Logger()) << "All scenarios failed to response: " << errors;
    errors.ToHttpResponse(Response);
}

} // namespace NAlice::NMegamind
