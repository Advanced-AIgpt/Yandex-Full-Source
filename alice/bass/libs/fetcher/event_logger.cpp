#include "event_logger.h"

#include <alice/bass/libs/logging_v2/logger.h>

#include <util/string/builder.h>

namespace NHttpFetcher {

// IEventLogger::TNehId --------------------------------------------------------
TString IEventLogger::TNehId::AsString() const {
    TStringBuilder str;
    if (!Name.Empty()) {
        str << '(' << Name << TStringBuf(") ");
    }
    str << Id;
    return str;
}

// IEventLogger ----------------------------------------------------------------
void IEventLogger::OnAttemptRegistered(TNehId requestId, ui64 fetcherId, TStringBuf address) const {
    Debug(TStringBuilder() << "Request " << requestId.AsString() << " (fetcher " << fetcherId
                           << ") is about to fetch: " << TString{address}.Quote());
}

void IEventLogger::OnAttemptStarted(TNehId requestId, ui32 attemptId, ui32 maxAttempts) const {
    Debug(TStringBuilder() << "Request " << requestId.AsString() << " sending attempt: " << attemptId << " of " << maxAttempts);
}

void IEventLogger::OnAttemptSuccess(TNehId requestId, ui32 attemptId, const TDuration& duration) const {
    Debug(TStringBuilder() << "Request " << requestId.AsString() << " attempt " << attemptId
                           << " has won, time taken: " << duration);
}

void IEventLogger::OnAttemptError(TNehId requestId, ui32 attemptId, TStringBuf data) const {
    Debug(TStringBuilder() << "Request " << requestId.AsString() << " attempt " << attemptId
                           << " error data: " << data);
}

void IEventLogger::OnAttemptError(TNehId requestId, ui32 attemptId, const TDuration& duration, TStringBuf error) const {
    Debug(TStringBuilder() << "Request " << requestId.AsString() << " attempt " << attemptId
                           << " has failed, time taken: " << duration << ", reason: " << error);
}

void IEventLogger::OnAbandonedRequest(ui64 /* requestId */) const {
    Error("Fetcher error: abandoned request");
}

// TBassEventLogger ------------------------------------------------------------
void TBassEventLogger::Error(TStringBuf msg) const {
    LOG(ERR) << msg << Endl;
}

void TBassEventLogger::Debug(TStringBuf msg) const {
    LOG(DEBUG) << msg << Endl;
}

} // namespace NHttpFetcher
