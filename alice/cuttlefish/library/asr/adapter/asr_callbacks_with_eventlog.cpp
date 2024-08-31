#include "asr_callbacks_with_eventlog.h"

#include "unistat.h"

using namespace NAlice::NAsr;
using namespace NAlice::NAsrAdapter;

TAsrCallbacksWithEventlog::TAsrCallbacksWithEventlog(
    NCuttlefish::TInputAppHostAsyncRequestHandlerPtr requestHandler,
    const TString& requestId,
    TAtomicBase requestNumber,
    NRTLog::TRequestLoggerPtr rtLog,
    const NCuttlefish::TLogContext::TOptions& logOptions
)
    : NAsr::TCallbacksHandler(requestHandler)
    , Log_(NCuttlefish::SpawnLogFrame(), rtLog, logOptions)
    , RequestId_(requestId)
    , RequestNumber_(requestNumber)
{
    Log_.LogEventInfoCombo<NEvClass::AsrCallbacksFrame>(RequestId_, RequestNumber_);
}

void TAsrCallbacksWithEventlog::OnSpotterValidation(bool valid) {
    if (Closed_) {
        return;
    }

    Log_.LogEventInfoCombo<NEvClass::SendToAppHostAsrSpotterValidation>(valid);
    Unistat().OnSendToAppHostRaw(1);  //and42@: need more precise calculation here?
    NAsr::TCallbacksHandler::OnSpotterValidation(valid);
}

void TAsrCallbacksWithEventlog::OnClosed() {
    if (Closed_) {
        return;
    }

    NAsr::TCallbacksHandler::OnClosed();
    Log_.LogEventInfoCombo<NEvClass::FlushAppHostContext>();
}

void TAsrCallbacksWithEventlog::OnAnyError(const TString& error, bool fastError) {
    if (fastError) {
        Log_.LogEventErrorCombo<NEvClass::RaiseAppHostFastError>(error);
    } else {
        Log_.LogEventErrorCombo<NEvClass::ErrorMessage>(error);
    }
    NAsr::TCallbacksHandler::OnAnyError(error, fastError);
}

void TAsrCallbacksWithEventlog::OnSessionLog(const NAliceProtocol::TSessionLogRecord& sessionLogRecord) {
    if (Log_.Options().WriteInfoToEventLog || Log_.Options().WriteInfoToRtLog) {  // NOTE: CPU optimisation (avoid call ShortUtf8DebugString)
        Log_.LogEventInfoCombo<NEvClass::SendToAppHostSessionLog>(sessionLogRecord.ShortUtf8DebugString());
    }
    Unistat().OnSendToAppHostRaw(sessionLogRecord.ByteSizeLong());
    NAsr::TCallbacksHandler::OnSessionLog(sessionLogRecord);
}

void TAsrCallbacksWithEventlog::AddAsrFinished(const NAliceProtocol::TAsrFinished& asrFinished) {
    if (Log_.Options().WriteInfoToEventLog || Log_.Options().WriteInfoToRtLog) {  // NOTE: CPU optimisation (avoid call ShortUtf8DebugString)
        Log_.LogEventInfoCombo<NEvClass::SendToAppHostAsrFinished>(asrFinished.ShortUtf8DebugString());
    }
    Unistat().OnSendToAppHostRaw(asrFinished.ByteSizeLong());
    NAsr::TCallbacksHandler::AddAsrFinished(asrFinished);
}

void TAsrCallbacksWithEventlog::AddAndFlush(const NProtobuf::TResponse& response, bool isFinalResponse) {
    if (Log_.Options().WriteInfoToEventLog || Log_.Options().WriteInfoToRtLog) {  // NOTE: CPU optimisation (avoid call ShortUtf8DebugString)
        Log_.LogEventInfoCombo<NEvClass::SendToAppHostAsrResponse>(response.ShortUtf8DebugString(), isFinalResponse);
    }
    Unistat().OnSendToAppHostRaw(response.ByteSizeLong());
    NAsr::TCallbacksHandler::AddAndFlush(response, isFinalResponse);
    if (isFinalResponse) {
        Log_.LogEventInfoCombo<NEvClass::FlushAppHostContext>();
    }
}

