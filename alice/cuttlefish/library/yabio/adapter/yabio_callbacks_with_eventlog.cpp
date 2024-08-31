#include "yabio_callbacks_with_eventlog.h"

#include <util/string/builder.h>

using namespace NAlice::NYabio;
using namespace NAlice::NYabioAdapter;

TYabioCallbacksWithEventlog::TYabioCallbacksWithEventlog(
    NCuttlefish::TInputAppHostAsyncRequestHandlerPtr rh,
    const TString& requestId,
    TAtomicBase requestNumber,
    NAlice::NCuttlefish::TLogContext&& logContext
)
    : NYabio::TCallbacksHandler(rh)
    , Log_(std::move(logContext))
    , RequestId_(requestId)
    , RequestNumber_(requestNumber)
{
    Log_.LogEventInfoCombo<NEvClass::YabioCallbacksFrame>(RequestId_, RequestNumber_);
}

void TYabioCallbacksWithEventlog::OnAnyError(NProtobuf::EResponseCode responseCode, const TString& error, bool fastError) {
    NYabio::TCallbacksHandler::OnAnyError(responseCode, error, fastError);
}

void TYabioCallbacksWithEventlog::AddNewEnrolling(const NAliceProtocol::TBioContextSaveNewEnrolling& newEnrolling) {
    NAliceProtocol::TBioContextSaveNewEnrolling reducedEnrolling(newEnrolling);
    size_t voiceprintCounter = 0;
    for (auto &enroll : *reducedEnrolling.MutableYabioEnrolling()) {
        voiceprintCounter += enroll.voiceprint().size();
        enroll.clear_voiceprint();
    }
    Log_.LogEventInfoCombo<NEvClass::SendToAppHostBioContextSaveNewEnrolling>(
        TStringBuilder() << reducedEnrolling.ShortUtf8DebugString() << " hide voiceprint(s) count=" << voiceprintCounter);
    NYabio::TCallbacksHandler::AddNewEnrolling(newEnrolling);
}

void TYabioCallbacksWithEventlog::AddAndFlush(const NProtobuf::TResponse& response, bool isFinalResponse) {
    NProtobuf::TResponse reducedResponse(response);
    // cut often repeated fields from response for reduce logged data volume: voiceprint
    if (reducedResponse.HasAddDataResponse()) {
        auto& addDataResponse = *reducedResponse.MutableAddDataResponse();
        if (addDataResponse.has_context()) {
            auto& context = *addDataResponse.mutable_context();
            if (context.enrolling().size()) {
                for (auto& voiceprint : *context.mutable_enrolling()) {
                    voiceprint.clear_voiceprint();
                }
            }
            if (context.users().size()) {
                for (auto& user : *context.mutable_users()) {
                    if (user.voiceprints().size()) {
                        for (auto& voiceprint : *user.mutable_voiceprints()) {
                            voiceprint.clear_voiceprint();
                        }
                    }
                }
            }
        }
    }
    Log_.LogEventInfoCombo<NEvClass::SendToAppHostYabioResponse>(reducedResponse.ShortUtf8DebugString(), isFinalResponse);
    NYabio::TCallbacksHandler::AddAndFlush(response, isFinalResponse);
    if (isFinalResponse) {
        Log_.LogEventInfoCombo<NEvClass::FlushAppHostContext>();
    }
}

void TYabioCallbacksWithEventlog::OnClosed() {
    if (Closed_) {
        return;
    }

    NYabio::TCallbacksHandler::OnClosed();
    Log_.LogEventInfoCombo<NEvClass::FlushAppHostContext>();
}
