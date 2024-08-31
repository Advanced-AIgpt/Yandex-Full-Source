#include "fake.h"

#include <alice/cuttlefish/library/logging/dlog.h>

#include <voicetech/library/proto_api/ttsbackend.pb.h>

#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/stream/str.h>

#include <library/cpp/neh/asio/executor.h>

using namespace NAlice::NTts;

namespace {

NAsio::TIOService& GetIOServiceForFakeAsyncGeneration() {
    static NAsio::TIOServiceExecutor executor;
    return executor.GetIOService();
}

} // namespace

TFake::TFake(
    TIntrusivePtr<TCallbacks>& callbacks,
    const TFakeChunksConfiguration& fakeChunksConfiguration
)
    : TInterface(callbacks)
    , TtsClient_(
        callbacks,
        fakeChunksConfiguration
    )
{}

void TFake::ProcessBackendRequest(const NProtobuf::TBackendRequest& backendRequest) {
    TtsClient_.AddBackendRequest(backendRequest);
}


void TFake::Cancel() {
    TtsClient_.CancelAll();
}

TFake::TTtsClient::TTtsClient(
    TIntrusivePtr<TCallbacks>& callbacks,
    const TFakeChunksConfiguration& fakeChunksConfiguration
)
    : Callbacks_(callbacks)
    , FakeChunksConfiguration_(fakeChunksConfiguration)
    , IOService_(GetIOServiceForFakeAsyncGeneration())
{}

void TFake::TTtsClient::AddBackendRequest(const NProtobuf::TBackendRequest& backendRequest) {
    if (!AddedReqSeqNo_.insert(backendRequest.GetReqSeqNo()).second) {
        IOService_.Post([callbacks = Callbacks_, reqSeqNo = backendRequest.GetReqSeqNo()]() {
            callbacks->OnDublicateRequest(reqSeqNo);
        });
        DLOG("Request with reqSeqNo=" << backendRequest.GetReqSeqNo() << " already exists");
        return;
    }

    TIntrusivePtr<TFake::TTtsClient::TTtsHandler> ttsHandler = MakeIntrusive<TFake::TTtsClient::TTtsHandler>(
        Callbacks_,
        backendRequest,
        FakeChunksConfiguration_,
        IOService_
    );
    ttsHandler->Start();
    TtsHandlers_.push_back(std::move(ttsHandler));
}

void TFake::TTtsClient::CancelAll() {
    // We can't directly access to callbacks
    // We must use IOService to prevent races
    IOService_.Post([callbacks = Callbacks_]() {
        callbacks->OnClosed();
    });

    for (auto ttsHandler : TtsHandlers_) {
        IOService_.Post([ttsHandler = ttsHandler]() {
            ttsHandler->Cancel();

        });
    }
}

TFake::TTtsClient::TTtsHandler::TTtsHandler(
    TIntrusivePtr<TCallbacks>& callbacks,
    const NProtobuf::TBackendRequest& backendRequest,
    const TFakeChunksConfiguration& fakeChunksConfiguration,
    NAsio::TIOService& ioService
)
    : Callbacks_(callbacks)
    , BackendRequest_(backendRequest)
    , FakeChunksConfiguration_(fakeChunksConfiguration)
    , IOService_(ioService)
{}

void TFake::TTtsClient::TTtsHandler::Start() {
    IOService_.Post([self = TIntrusivePtr<TFake::TTtsClient::TTtsHandler>(this)]() {
        if (self->Closed_) {
            return;
        }

        self->Callbacks_->OnStartRequestProcessing(self->BackendRequest_.GetReqSeqNo());

        if (!self->BackendRequest_.HasGenerate()) {
            self->Callbacks_->OnInvalidRequest(self->BackendRequest_.GetReqSeqNo(), "No generate request in tts backend request");
            self->CompletedSended_ = true;
            self->Closed_ = true;
            // Ignore unknown requests
            return;
        }

        self->Callbacks_->OnRequestProcessingStarted(self->BackendRequest_.GetReqSeqNo());

        if (self->BackendRequest_.GetGenerate().has_chunked() && self->BackendRequest_.GetGenerate().chunked()) {
            self->Timer_.Reset(new NAsio::TDeadlineTimer(self->IOService_));
            self->ScheduleTimerUnsafe();
        } else {

            NProtobuf::TBackendResponse backendResponse;
            backendResponse.SetReqSeqNo(self->BackendRequest_.GetReqSeqNo());
            {
                auto& generateResponse = *backendResponse.MutableGenerateResponse();
                static const char* fakeAudio = "fake_audio";
                generateResponse.set_audiodata(fakeAudio, 10);
                generateResponse.set_completed(true);
            }

            self->Callbacks_->OnBackendResponse(backendResponse, self->BackendRequest_.GetGenerate().mime());
            self->CompletedSended_ = true;
            self->Closed_ = true;
        }
    });
}

void TFake::TTtsClient::TTtsHandler::Cancel() {
    IOService_.Post([self = TIntrusivePtr<TFake::TTtsClient::TTtsHandler>(this)]() {
        if (self->Closed_) {
            return;
        }

        if (self->Timer_) {
            // Async cancel request
            self->Timer_->Cancel();
        } else {
            // Cancel before initialization
            // Impossible or lockfree queue magic?
            self->Closed_ = true;
            self->CompletedSended_ = true;
        }
    });
}

bool TFake::TTtsClient::TTtsHandler::SendNextChunkUnsafe() {
    if (FakeChunksConfiguration_.ChunksNum_) {
        --FakeChunksConfiguration_.ChunksNum_;
        NProtobuf::TBackendResponse backendResponse;
        backendResponse.SetReqSeqNo(BackendRequest_.GetReqSeqNo());
        {
            auto& generateResponse = *backendResponse.MutableGenerateResponse();
            // Create here chunk (fill with FakeChunkContent_)
            TVector<char> chunk;
            chunk.resize(FakeChunksConfiguration_.ChunkSize_);
            size_t filled = 0;
            for (;;) {
                size_t need = chunk.size() - filled;
                if (!need) {
                    break;
                }
                TStringBuf contentLeft(
                    FakeChunksConfiguration_.Content_.data() + ContentConsumed_,
                    FakeChunksConfiguration_.Content_.data() + FakeChunksConfiguration_.Content_.size()
                );
                size_t leftBytes = contentLeft.size();

                size_t fill = Min(need, leftBytes);
                std::memmove(chunk.data() + filled, contentLeft.data(), fill);
                filled += fill;
                ContentConsumed_ += fill;
                if (ContentConsumed_ == FakeChunksConfiguration_.Content_.size()) {
                    if (FakeChunksConfiguration_.RepeatContent_) {
                        // Continus sending
                        ContentConsumed_ = 0;
                    } else {
                        // Finish sending
                        chunk.resize(filled);
                        FakeChunksConfiguration_.ChunksNum_ = 0;
                    }
                }
            }
            generateResponse.set_audiodata(chunk.data(), chunk.size());
            if (!FakeChunksConfiguration_.ChunksNum_) {
                generateResponse.set_completed(true);
                CompletedSended_ = true;
            } else {
                generateResponse.set_completed(false);
            }
        }
        Callbacks_->OnBackendResponse(backendResponse, BackendRequest_.GetGenerate().mime());
    }

    return FakeChunksConfiguration_.ChunksNum_;
}

void TFake::TTtsClient::TTtsHandler::SendErrorUnsafe(const TString& error) {
    NProtobuf::TBackendResponse backendResponse;
    backendResponse.SetReqSeqNo(BackendRequest_.GetReqSeqNo());
    {
        auto* generateResponse = backendResponse.MutableGenerateResponse();
        generateResponse->SetresponseCode(BasicProtobuf::ConnectionResponse::ProtocolError);
        generateResponse->set_message(error);
        generateResponse->set_completed(true);
    }

    Callbacks_->OnBackendResponse(backendResponse, BackendRequest_.GetGenerate().mime());
    CompletedSended_ = true;
}

void TFake::TTtsClient::TTtsHandler::ScheduleTimerUnsafe() {
    Y_ASSERT(Timer_);
    Timer_->AsyncWaitExpireAt(FakeChunksConfiguration_.ChunkDuration_, [self = TIntrusivePtr<TFake::TTtsClient::TTtsHandler>(this)](const NAsio::TErrorCode& err, NAsio::IHandlingContext&) {
        self->OnTimerCallback(err);
    });
}

void TFake::TTtsClient::TTtsHandler::OnTimerCallback(const NAsio::TErrorCode& err) {
    if (Closed_) {
        return;
    }

    if (Error_) {
        SendErrorUnsafe(Error_);

        Timer_.Reset();
        Closed_ = true;
    } else if (err || !SendNextChunkUnsafe()) {
        // Probably request in cancelled
        if (!CompletedSended_) {
            // Send last (and empty = not contain audioData) chunk
            NProtobuf::TBackendResponse backendResponse;
            backendResponse.SetReqSeqNo(BackendRequest_.GetReqSeqNo());
            {
                auto& generateResponse = *backendResponse.MutableGenerateResponse();
                generateResponse.set_completed(true);
            }

            Callbacks_->OnBackendResponse(backendResponse, BackendRequest_.GetGenerate().mime());
            CompletedSended_ = true;
        }

        Timer_.Reset();
        Closed_ = true;
    } else {
        ScheduleTimerUnsafe();
    }
}
