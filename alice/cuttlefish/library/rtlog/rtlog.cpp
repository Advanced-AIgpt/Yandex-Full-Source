#include "rtlog.h"

#include <util/string/builder.h>
#include <util/system/hostname.h>

using namespace NAlice::NCuttlefish;

namespace {

TMaybe<TString> TryGetRtLogTokenFromRtLogTokenHeader(const THttpHeaders& Headers) {
    static constexpr TStringBuf RT_LOG_TOKEN_HEADER_NAME = "x-rtlog-token";

    for (const auto& header : Headers) {
        if (to_lower(header.Name()) == RT_LOG_TOKEN_HEADER_NAME) {
            return header.Value();
        }
    }

    return Nothing();
}

TMaybe<TString> TryGetRtLogTokenFromAppHostHeaders(const THttpHeaders& Headers) {
    static constexpr TStringBuf APPHOST_REQID_HEADER_NAME = "x-apphost-request-reqid";
    static constexpr TStringBuf APPHOST_RUID_HEADER_NAME = "x-apphost-request-ruid";

    TMaybe<TString> reqId = Nothing();
    TMaybe<TString> ruid = Nothing();

    for (const auto& header : Headers) {
        if (auto lowerHeaderName = to_lower(header.Name()); lowerHeaderName == APPHOST_REQID_HEADER_NAME) {
            reqId = header.Value();
        } else if (lowerHeaderName == APPHOST_RUID_HEADER_NAME) {
            ruid = header.Value();
        }
    }

    if (reqId.Defined() && ruid.Defined()) {
        return TStringBuilder() << *reqId << '-' << *ruid;
    }

    return Nothing();
}

} // namespace

void TRtLogClient::Init(const NAliceServiceConfig::TRtLog& cfg) {
    ClientOptions_.Async = true;
    if (cfg.has_flush_size()) {
        ClientOptions_.FlushSize = cfg.flush_size();
    } else {
        ClientOptions_.FlushSize = 1024 * 1024;
    }
    if (cfg.has_flush_period()) {
        ClientOptions_.FlushPeriod = cfg.flush_period();
    } else {
        ClientOptions_.FlushPeriod = TDuration::Seconds(60);
    }
    if (cfg.has_file_stat_check_period()) {
        ClientOptions_.FileStatCheckPeriod = cfg.file_stat_check_period();
    } else {
        ClientOptions_.FileStatCheckPeriod = TDuration::Seconds(1);
    }
    try {
        ClientOptions_.Hostname = FQDNHostName();
    } catch (...) {
        ClientOptions_.Hostname = "unknown";
    }
    if (cfg.has_file()) {
        if (cfg.has_unified_agent_uri() && !cfg.unified_agent_uri().empty()) {
            RTLogClient_.Reset(
                new NRTLog::TClient(
                    cfg.file(),
                    cfg.service(),
                    ClientOptions_,
                    COMPRESSED_LOG_FORMAT_V5,
                    cfg.unified_agent_uri(),
                    cfg.unified_agent_log_file()
            ));
        } else {
            RTLogClient_.Reset(new NRTLog::TClient(cfg.file(), cfg.service(), ClientOptions_));
        }
    }
}

void NAlice::NCuttlefish::SaveRtLogEvent(NRTLog::TRequestLogger* logger, const TString& message, NRTLogEvents::ESeverity severity) {
    if (!logger) {
        return;
    }

    NRTLogEvents::TLogEvent ev;
    ev.SetSeverity(severity);
    ev.SetMessage(message);
    logger->LogEvent(ev);
}

void TRtLogInputRequest::RtLogInfoEvent(const TString& message, NRTLog::TRequestLogger* logger) {
    if (!logger) {
        logger = RtLogger_.get();
    }
    if (Options.WriteInfoToRtLog) {
        SaveRtLogInfoEvent(logger, message);
    }
}

void TRtLogInputRequest::RtLogErrorEvent(const TString& message, NRTLog::TRequestLogger* logger) {
    if (!logger) {
        logger = RtLogger_.get();
    }
    SaveRtLogErrorEvent(logger, message);
}

TMaybe<TString> NAlice::NCuttlefish::TryGetRtLogTokenFromAppHostContext(const NAppHost::IServiceContext& ahContext) {
    auto appHostParams = ahContext.FindFirstItem(NAppHost::APP_HOST_PARAMS_TYPE);
    if (appHostParams && appHostParams->IsDefined()) {
        return TStringBuilder() << (*appHostParams)["reqid"].GetString() << '-' << ahContext.GetRUID();
    }
    return Nothing();
}

TString NAlice::NCuttlefish::GetRtLogTokenFromTypedAppHostServiceContext(const NAppHost::ITypedServiceContext& typedServiceContext) {
    return TStringBuilder() << typedServiceContext.GetRequestID() << '-' << typedServiceContext.GetRUID();
}

TMaybe<TString> NAlice::NCuttlefish::TryGetRtLogTokenFromHttpRequestHeaders(const THttpHeaders& httpHeaders) {
    // WARNING: Do not change order here
    // We want to get token from x-rtlog-token header if it exists
    // and only if it does not exist fallback to apphost headers
    TMaybe<TString> rtLogToken = TryGetRtLogTokenFromRtLogTokenHeader(httpHeaders);
    if (!rtLogToken.Defined()) {
        rtLogToken = TryGetRtLogTokenFromAppHostHeaders(httpHeaders);
    }

    return rtLogToken;
}
