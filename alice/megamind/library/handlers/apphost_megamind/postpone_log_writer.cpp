#include "postpone_log_writer.h"

#include <alice/megamind/library/apphost_request/item_adapter.h>
#include <alice/megamind/library/apphost_request/item_names.h>
#include <alice/megamind/library/context/context.h>
#include <alice/megamind/library/experiments/flags.h>
#include <alice/megamind/library/handlers/apphost_megamind/components.h>
#include <alice/megamind/library/handlers/utils/analytics_logs_context_builder.h>
#include <alice/megamind/library/handlers/utils/logs_util.h>
#include <alice/megamind/library/response/utils.h>
#include <alice/megamind/library/speechkit/request.h>

#include <alice/library/experiments/flags.h>
#include <alice/library/json/json.h>
#include <alice/library/network/common.h>
#include <alice/library/network/headers.h>

#include <alice/megamind/protos/speechkit/request.pb.h>

#include <util/string/ascii.h>

namespace NAlice::NMegamind {
namespace {

inline const TVector<TStringBuf> ADDITIONAL_PATHS_TO_BE_OBFUSCATED_IN_REQUEST = {
    "iot_user_info_data",
    "memento",
    "request/device_state",
    "request/smart_home",
    "session",
};

inline const TVector<TStringBuf> ADDITIONAL_PATHS_TO_BE_OBFUSCATED_IN_RESPONSE = {
    "response/quality_storage/scenarios_information",
    "sessions",
};

NJson::TJsonValue LogRequest(IAppHostCtx& ahCtx, const bool haveErrorsInResponse) {
    auto& ipa = ahCtx.ItemProxyAdapter();

    NAppHostHttp::THttpRequest httpRequest;
    if (auto err = ipa.GetFromContextCached<NAppHostHttp::THttpRequest>(NAppHost::PROTO_HTTP_REQUEST).MoveTo(httpRequest)) {
        LOG_ERR(ahCtx.Log()) << "Unable to log request: No '" << NAppHost::PROTO_HTTP_REQUEST << "' found in apphost context";
        return {};
    }

    auto pathsToBeObfuscated = NLogsUtil::BASE_PATHS_TO_BE_OBFUSCATED_IN_REQUEST;
    if (ahCtx.Log().GetLogPriority() == ELogPriority::TLOG_INFO && !haveErrorsInResponse) {
        pathsToBeObfuscated.insert(pathsToBeObfuscated.end(), ADDITIONAL_PATHS_TO_BE_OBFUSCATED_IN_REQUEST.begin(),
                                   ADDITIONAL_PATHS_TO_BE_OBFUSCATED_IN_REQUEST.end());
    }

    auto createFromJson = [&httpRequest, &pathsToBeObfuscated](NLogsUtil::TFilterLogger& reqLogger) -> NJson::TJsonValue {
        try {
            NJson::TJsonValue skrJson;
            NJson::ReadJsonFastTree(httpRequest.GetContent(), &skrJson, true);
            reqLogger.SetContent(skrJson, pathsToBeObfuscated);
            return skrJson;
        } catch (const NJson::TJsonException& e) {
            reqLogger.SetContent(httpRequest.GetContent());
        }
        return {};
    };

    auto createFromProto = [&httpRequest, &ahCtx, &pathsToBeObfuscated](NLogsUtil::TFilterLogger& reqLogger) -> NJson::TJsonValue {
        TSpeechKitRequestProto proto;
        if (proto.ParseFromString(httpRequest.GetContent())) {
            NJson::TJsonValue skrJson = JsonFromProto(proto);
            if (ahCtx.ItemProxyAdapter().CheckFlagInInputContext(EXP_LOG_REQUEST_AS_PROTOBUF)) {
                reqLogger.SetContent(std::move(proto));
            } else {
                reqLogger.SetContent(skrJson, pathsToBeObfuscated);
            }
            return skrJson;
        }
        LOG_ERR(ahCtx.Log()) << "LogRequest: parse protobuf error";
        return {};
    };

    TStringBuilder headers;
    const TString* ct = nullptr;
    for (const auto& h : httpRequest.GetHeaders()) {
        headers << h.GetName() << ": " << h.GetValue() << "\n";

        if (!ct && AsciiEqualsIgnoreCase(h.GetName(), NNetwork::HEADER_CONTENT_TYPE)) {
            ct = &h.GetValue();
        }
    }

    NLogsUtil::TFilterLogger reqLogger;
    NJson::TJsonValue skrJson;
    reqLogger.AddTag(TLogMessageTag{"Megamind request"});
    if (ct
        && (AsciiEqualsIgnoreCase(*ct, NContentTypes::TEXT_PROTOBUF)
            || AsciiEqualsIgnoreCase(*ct, NContentTypes::APPLICATION_PROTOBUF)))
    {
        skrJson = createFromProto(reqLogger);
    } else {
        skrJson = createFromJson(reqLogger);
    }

    reqLogger.SetFirstLine(
        TString::Join("AppHost Request: ", NAppHostHttp::THttpRequest::EMethod_Name(httpRequest.GetMethod()),
                      httpRequest.GetPath(), "\r\n", headers));

    reqLogger.Log(ahCtx.Log());
    return skrJson;
}

bool HaveAnyErrorsInResponse(const TErrorOr<NMegamindAppHost::TAnalyticsLogsContext>& logContextOrError,
                             TRTLogger& logger) {
    if (logContextOrError.Error()) {
        return true;
    }
    const auto& logContext = logContextOrError.Value();

    if (logContext.GetResponseCase() == NMegamindAppHost::TAnalyticsLogsContext::ResponseCase::kError) {
        LOG_ERR(logger) << "Got error instead of speechkit response: " << logContext.GetError().ShortUtf8DebugString();
        return true;
    }
    if (logContext.GetResponseCase() == NMegamindAppHost::TAnalyticsLogsContext::ResponseCase::RESPONSE_NOT_SET) {
        LOG_ERR(logger) << "No speechkit response (response not set)";
        return true;
    }

    const auto& skResponseProto = logContext.GetSpeechKitResponse();
    return logContext.GetHttpCode() != HttpCodes::HTTP_OK || skResponseProto.HasError() ||
           !skResponseProto.GetResponse().GetMeta().empty();
}

} // namespace

namespace NImpl {

void LogResponse(NJson::TJsonValue&& responseJson, const bool haveErrorsInResponse, TRTLogger& logger,
                 const int statusCode) {
    NLogsUtil::TFilterLogger respLogger;
    respLogger.AddTag(TLogMessageTag{"Megamind response"});

    const auto pathsToBeObfuscated = logger.GetLogPriority() == ELogPriority::TLOG_INFO && !haveErrorsInResponse
                                         ? ADDITIONAL_PATHS_TO_BE_OBFUSCATED_IN_RESPONSE
                                         : TVector<TStringBuf>{};

    respLogger.SetContent(std::move(responseJson), pathsToBeObfuscated);
    respLogger.SetFirstLine(TString::Join("Megamind response (status code: ", ToString(statusCode), "):"));

    respLogger.Log(logger);
}

} // namespace NImpl

void RegisterPostponeLogWriterHander(IGlobalCtx& globalCtx, TRegistry& registry) {
    static const TAppHostPostponeLogWriter postponeLogWriter{globalCtx};
    registry.Add("/mm_postpone_log_write", [](NAppHost::IServiceContext& ctx) { postponeLogWriter.RunSync(ctx); });
}

TAppHostPostponeLogWriter::TAppHostPostponeLogWriter(IGlobalCtx& globalCtx)
    : TAppHostNodeHandler{globalCtx, /* useAppHostStreaming= */ false}
{
}

TStatus TAppHostPostponeLogWriter::Execute(IAppHostCtx& ahCtx) const {
    auto& logger = ahCtx.Log();

    TStatus status = Success();

    auto analyticsLogsOrError = ahCtx.ItemProxyAdapter().GetFromContext<NMegamindAppHost::TAnalyticsLogsContext>(
        AH_ITEM_ANALYTICS_LOG_CONTEXT);
    const bool haveErrorsInResponse = HaveAnyErrorsInResponse(analyticsLogsOrError, logger);

    NJson::TJsonValue skrJson;

    try {
        skrJson = LogRequest(ahCtx, haveErrorsInResponse);
    } catch (...) {
        status = TError{TError::EType::Exception} << "Exception during logging the request: " << CurrentExceptionMessage();
        LOG_ERR(logger) << *status;
    }

    if (haveErrorsInResponse) {
        LOG_INFO(logger) << "Megamind response have errors -> logging full response/request";
    }
    if (analyticsLogsOrError.Error()) {
        LOG_ERR(logger) << "Failed to retreive analytics log context";
    } else {
        const auto logContext = analyticsLogsOrError.Value();

        auto responseJson = SpeechKitResponseToJson(logContext.GetSpeechKitResponse());
        if (logContext.GetHideSensitiveData()) {
            responseJson = NLogsUtil::RemoveSensitiveData(responseJson);
        }

        try {
            NLogsUtil::PrepareAndProcessAnalyticsLogs(responseJson, logContext.GetHideSensitiveData(), skrJson,
                                                      logContext.GetAnalyticsInfo(), logContext.GetQualityStorage(),
                                                      ahCtx.GlobalCtx(), logContext.GetProactivityLogStorage(),
                                                      ahCtx.ItemProxyAdapter().CheckFlagInInputContext(NExperiments::DUMP_SESSIONS_TO_LOGS));
        } catch (...) {
            status = TError{TError::EType::Exception} << "Exception during writing analytics logs: "
                                                      << CurrentExceptionMessage();
            LOG_ERR(logger) << *status;
        }

        try {
            NImpl::LogResponse(std::move(responseJson), haveErrorsInResponse, logger,
                        static_cast<int>(logContext.GetHttpCode()));
        } catch (...) {
            status = TError{TError::EType::Exception} << "Exception during logging the response: "
                                                      << CurrentExceptionMessage();
            LOG_ERR(logger) << *status;
        }
    }

    return status;
}

} // namespace NAlice::NMegamind
