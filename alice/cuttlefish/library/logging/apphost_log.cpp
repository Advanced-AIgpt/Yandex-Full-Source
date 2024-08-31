#include "apphost_log.h"

#include <alice/cuttlefish/library/cuttlefish/common/edge_flags.h>

using namespace NAlice::NCuttlefish;

void NAlice::NCuttlefish::TryGetLogOptionsFromAppHostContext(NAppHost::IServiceContext& ctx, TLogContext::TOptions& logOptions) {
    logOptions.WriteInfoToRtLog = !ctx.CheckFlagInInputContext(EDGE_FLAG_DO_NOT_WRITE_INFO_TO_RTLOG);
    logOptions.WriteInfoToEventLog = !ctx.CheckFlagInInputContext(EDGE_FLAG_DO_NOT_WRITE_INFO_TO_EVENTLOG);
}

TLogContext NAlice::NCuttlefish::LogContextFor(NAppHost::IServiceContext& ctx, NAlice::NCuttlefish::TRtLogClient* rtLogClient) {
    NRTLog::TRequestLoggerPtr rtLogger;
    if (rtLogClient) {
        rtLogger = rtLogClient->CreateRequestLogger(NCuttlefish::TryGetRtLogTokenFromAppHostContext(ctx).GetOrElse(""), /*session=*/false);
    }
    TLogContext::TOptions logOptions;
    TryGetLogOptionsFromAppHostContext(ctx, logOptions);
    return NCuttlefish::TLogContext(NCuttlefish::SpawnLogFrame(), rtLogger, logOptions);
}
