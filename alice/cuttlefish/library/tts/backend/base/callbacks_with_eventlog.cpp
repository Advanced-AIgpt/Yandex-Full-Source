#include "callbacks_with_eventlog.h"

#include <alice/cuttlefish/library/cuttlefish/common/utils.h>

#include <voicetech/library/proto_api/ttsbackend.pb.h>

#include <util/string/builder.h>

using namespace NAlice::NTts;

TCallbacksWithEventlog::TCallbacksWithEventlog(
    NCuttlefish::TInputAppHostAsyncRequestHandlerPtr requestHandler,
    NAlice::NCuttlefish::TLogContext&& logContext,
    TAtomicBase requestNumber
)
    : NTts::TCallbacksHandler(requestHandler)
    , Metrics_(TCallbacksWithEventlog::SOURCE_NAME)
    , LogContext_(logContext)
    , RequestNumber_(requestNumber)
{
    LogContext_.LogEventInfoCombo<NEvClass::TtsCallbacksFrame>(
        RequestNumber_,
        GuidToUuidString(RequestHandler_->Context().GetRequestID()),
        RequestHandler_->Context().GetRUID(),
        TString(RequestHandler_->Context().GetLocation().Path),
        TString(RequestHandler_->Context().GetRemoteHost())
    );
}

void TCallbacksWithEventlog::OnStartRequestProcessing(ui32 reqSeqNo) {
    Metrics_.PushRate("startrequestprocessing", "ok");
    LogContext_.LogEventInfoCombo<NEvClass::StartTtsRequestProcessing>(reqSeqNo);

    NTts::TCallbacksHandler::OnStartRequestProcessing(reqSeqNo);
}

void TCallbacksWithEventlog::OnRequestProcessingStarted(ui32 reqSeqNo) {
    Metrics_.PushRate("requestprocessingstarted", "ok");
    LogContext_.LogEventInfoCombo<NEvClass::TtsRequestProcessingStarted>(reqSeqNo);

    NTts::TCallbacksHandler::OnRequestProcessingStarted(reqSeqNo);
}

void TCallbacksWithEventlog::OnDublicateRequest(ui32 reqSeqNo) {
    Metrics_.PushRate("duplicaterequest", "error");
    LogContext_.LogEventErrorCombo<NEvClass::ErrorMessage>(TStringBuilder() << "Dublicate request with reqSeqNo = " << reqSeqNo);

    NTts::TCallbacksHandler::OnDublicateRequest(reqSeqNo);
}

void TCallbacksWithEventlog::OnInvalidRequest(ui32 reqSeqNo, const TString& errorMessage) {
    Metrics_.PushRate("invalidrequest", "error");
    LogContext_.LogEventErrorCombo<NEvClass::ErrorMessage>(TStringBuilder() << "Invalid request with reqSeqNo = " << reqSeqNo << ", error message: " << errorMessage);

    NTts::TCallbacksHandler::OnInvalidRequest(reqSeqNo, errorMessage);
}

void TCallbacksWithEventlog::OnAnyError(const TString& error, bool fastError, ui32 reqSeqNo) {
    if (fastError && !AnyRequestProcessingStarted_) {
        // We can handle fast error as fast error only if we do not response with any chunk
        // TODO(chegoryu, VOICESERV-4075): fix it
        Metrics_.PushRate("fasterror", "error");
        LogContext_.LogEventErrorCombo<NEvClass::RaiseAppHostFastError>(TStringBuilder() << error << " at " << reqSeqNo);
    } else {
        Metrics_.PushRate("anyerror", "error");
        LogContext_.LogEventErrorCombo<NEvClass::ErrorMessage>(TStringBuilder() << error << " at " << reqSeqNo);
    }

    NTts::TCallbacksHandler::OnAnyError(error, fastError, reqSeqNo);
}

void TCallbacksWithEventlog::AddAudioAndFlush(const NAliceProtocol::TAudio& audio) {
    // As old code assume that empty responseCode is 200 code
    ui32 responseCode = 200;
    if (audio.GetTtsBackendResponse().GetGenerateResponse().has_responsecode() && audio.GetTtsBackendResponse().GetGenerateResponse().responsecode()) {
        responseCode = audio.GetTtsBackendResponse().GetGenerateResponse().responsecode();
    }
    bool isResponseCodeBad = (responseCode != 200);
    Metrics_.PushRate("backendresponse", ToString(responseCode));

    if (audio.HasChunk()) {
        Metrics_.PushRate(audio.GetChunk().GetData().size(), "audiosent", "ok");

        NAliceProtocol::TAudio audioExceptChunk = audio;
        audioExceptChunk.ClearChunk();
        if (isResponseCodeBad) {
            LogContext_.LogEventErrorCombo<NEvClass::SendToAppHostAudioChunk>(audio.GetChunk().GetData().size(), audioExceptChunk.ShortUtf8DebugString());
        } else {
            LogContext_.LogEventInfoCombo<NEvClass::SendToAppHostAudioChunk>(audio.GetChunk().GetData().size(), audioExceptChunk.ShortUtf8DebugString());
        }
    } else {
        if (isResponseCodeBad) {
            LogContext_.LogEventErrorCombo<NEvClass::SendTtsResponse>(audio.ShortUtf8DebugString());
        } else {
            LogContext_.LogEventInfoCombo<NEvClass::SendTtsResponse>(audio.ShortUtf8DebugString());
        }
    }

    NTts::TCallbacksHandler::AddAudioAndFlush(audio);
}

void TCallbacksWithEventlog::FlushAppHostContext(bool isFinalFlush) {
    NTts::TCallbacksHandler::FlushAppHostContext(isFinalFlush);

    if (isFinalFlush) {
        LogContext_.LogEventInfoCombo<NEvClass::FlushAppHostContext>();
    }
}
