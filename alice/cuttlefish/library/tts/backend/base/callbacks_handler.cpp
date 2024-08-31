#include "callbacks_handler.h"

#include <alice/cuttlefish/library/cuttlefish/common/item_types.h>

#include <voicetech/library/proto_api/ttsbackend.pb.h>

#include <apphost/api/service/cpp/service_exceptions.h>

using namespace NAlice::NTts;
using namespace NAlice::NCuttlefish;

void TCallbacksHandler::OnStartRequestProcessing(ui32 reqSeqNo) {
    // Useful for logging
    Y_UNUSED(reqSeqNo);

    if (Closed_) {
        return;
    }
}

void TCallbacksHandler::OnRequestProcessingStarted(ui32 reqSeqNo) {
    // Useful for logging
    Y_UNUSED(reqSeqNo);

    if (Closed_) {
        return;
    }

    AnyRequestProcessingStarted_ = true;
}

void TCallbacksHandler::OnBackendResponse(NProtobuf::TBackendResponse& backendResponse, const TString& mime) {
    if (Closed_) {
        return;
    }

    ui32 reqSeqNo = backendResponse.GetReqSeqNo();
    if (FinishedReqSeqNo_.contains(reqSeqNo)) {
        // Log erorr + sensor
        return;
    }

    if (backendResponse.HasGenerateResponse()) {
        auto& generateResponse = *backendResponse.MutableGenerateResponse();
        bool streamStarted = StartedReqSeqNo_.contains(reqSeqNo);

        if (generateResponse.has_responsecode() && generateResponse.responsecode() != 200) {
            NAliceProtocol::TAudio audio;

            if (streamStarted) {
                audio.MutableEndStream();
            } else {
                audio.MutableMetaInfoOnly();
            }

            FinishedReqSeqNo_.insert(reqSeqNo);
            audio.MutableTtsBackendResponse()->Swap(&backendResponse);

            AddAudioAndFlush(audio);

            return;
        } else {
            bool finalResponse = (generateResponse.has_completed() && generateResponse.completed());

            if (generateResponse.has_audiodata() && generateResponse.audiodata().Size()) {
                if (!streamStarted) {
                    // On first audio data chunk, send begin stream (with mime format)
                    NAliceProtocol::TAudio audio;
                    audio.MutableBeginStream()->SetMime(mime);
                    audio.MutableTtsBackendResponse()->SetReqSeqNo(reqSeqNo);
                    AddAudioAndFlush(audio);

                    StartedReqSeqNo_.insert(reqSeqNo);
                    // First chunk probably can be last chunk
                    // So we need to keep actual state in this variable
                    streamStarted = true;
                }


                NAliceProtocol::TAudio audio;

                // Send audio once in chunk
                // Add all other fields to meta info
                audio.MutableChunk()->SetData(generateResponse.audiodata());
                generateResponse.ClearaudioData();
                audio.MutableTtsBackendResponse()->Swap(&backendResponse);
                AddAudioAndFlush(audio);
            } else {
                NAliceProtocol::TAudio audio;

                audio.MutableMetaInfoOnly();
                audio.MutableTtsBackendResponse()->Swap(&backendResponse);

                // If stream already started we must send EndOfStream after this meta info
                // Otherwise tts return only meta info (without audio) and this is final result (TODO(chegoryu) log as error?)
                AddAudioAndFlush(audio);
            }

            if (finalResponse && streamStarted) {
                NAliceProtocol::TAudio audio;
                audio.MutableEndStream();
                audio.MutableTtsBackendResponse()->SetReqSeqNo(reqSeqNo);
                AddAudioAndFlush(audio);
            }

            if (finalResponse) {
                FinishedReqSeqNo_.insert(reqSeqNo);
            }
        }
    } else {
        // Log + sensor error ?
    }
}

void TCallbacksHandler::OnDublicateRequest(ui32 reqSeqNo) {
    // Useful for logging
    Y_UNUSED(reqSeqNo);

    if (Closed_) {
        return;
    }
}

void TCallbacksHandler::OnInvalidRequest(ui32 reqSeqNo, const TString& errorMessage) {
    // Useful for logging
    Y_UNUSED(reqSeqNo);
    Y_UNUSED(errorMessage);

    if (Closed_) {
        return;
    }
}

void TCallbacksHandler::OnClosed() {
    if (Closed_) {
        return;
    }

    FlushAppHostContext(/* isFinalFlush = */ true);
}

void TCallbacksHandler::OnAnyError(const TString& error, bool fastError, ui32 reqSeqNo) {
    if (Closed_) {
        return;
    }
    if (FinishedReqSeqNo_.contains(reqSeqNo)) {
        // TODO log error + sensor
        // Do not send error after finish
        // Ignore harash disconnected to real tts-server after finish for example
        return;
    }

    if (fastError && !AnyRequestProcessingStarted_) {
        RequestHandler_->SetException(std::make_exception_ptr(NAppHost::NService::TFastError()));
        Closed_ = true;
        Finished_ = true;
        return;
    }

    NAliceProtocol::TAudio audio;
    {
        auto& backendResponse = *audio.MutableTtsBackendResponse();
        {
            auto& generateResponse = *backendResponse.MutableGenerateResponse();
            generateResponse.set_completed(true);
            generateResponse.set_responsecode(BasicProtobuf::ConnectionResponse::ProtocolError);
            generateResponse.set_message(error);
        }

        if (StartedReqSeqNo_.contains(reqSeqNo)) {
            audio.MutableEndStream();
        } else {
            audio.MutableMetaInfoOnly();
        }
    }

    FinishedReqSeqNo_.insert(reqSeqNo);
    AddAudioAndFlush(audio);
}

void TCallbacksHandler::Finish() {
    RequestHandler_->Finish();
    Finished_ = true;
}

void TCallbacksHandler::AddAudioAndFlush(const NAliceProtocol::TAudio& audio) {
    if (Closed_) {
        return;
    }

    RequestHandler_->Context().AddProtobufItem(audio, ITEM_TYPE_AUDIO);
    FlushAppHostContext(/* isFinalFlush = */ false);
}

void TCallbacksHandler::FlushAppHostContext(bool isFinalFlush) {
    if (isFinalFlush) {
        RequestHandler_->Context().Flush();
        Closed_ = true;
    } else {
        RequestHandler_->Context().IntermediateFlush();
    }
}
