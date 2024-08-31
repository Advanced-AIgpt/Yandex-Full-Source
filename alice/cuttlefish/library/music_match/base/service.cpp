#include "service.h"

#include <alice/cuttlefish/library/apphost/item_parser.h>
#include <alice/cuttlefish/library/cuttlefish/common/item_types.h>

using namespace NAlice::NMusicMatch;
using namespace NAlice::NCuttlefish;

void TService::TRequestProcessor::ProcessInput(NAppHost::TServiceContextPtr ctx, NThreading::TPromise<void> promise) {
    if (!MusicMatchInitialized_) {
        InitializeMusicMatch(CreateMusicMatchCallbacks(ctx));
    }

    if (
        // WARNING: order is important
        // We must process context load response, session context and tvm service ticket before any init request
        !ProcessContextLoadResponseItems(ctx)
        || !ProcessSessionContextItems(ctx)
        || !ProcessTvmServiceTicket(ctx)

        || !ProcessCuttlefishAudioItems(ctx)
        || !ProcessUnknownRequestItems(ctx)
    ) {
        OnAppHostClose(promise);
        return;
    }

    TIntrusivePtr<TRequestProcessor> self(this);
    ctx->NextInput().Apply([ctx, processor = std::move(self), promise = std::move(promise)](auto hasData) mutable {
        if (!hasData.GetValue()) {
            processor->OnAppHostEmptyInput();
            processor->OnAppHostClose(promise, ctx->IsCancelled());
            return;
        }
        processor->ProcessInput(ctx, promise);
    });
}

bool TService::TRequestProcessor::ProcessContextLoadResponseItems(NAppHost::TServiceContextPtr& ctx) {
    const auto contextLoadResponseItemRefs = ctx->GetProtobufItemRefs(ITEM_TYPE_CONTEXT_LOAD_RESPONSE, NAppHost::EContextItemSelection::Input);
    for (const auto& contextLoadResponseItem : contextLoadResponseItemRefs) {
        NProtobuf::TContextLoadResponse contextLoadResponse;
        try {
            ParseProtobufItem(contextLoadResponseItem, contextLoadResponse);
        } catch (...) {
            OnError(CurrentExceptionMessage());
            return false;
        }
        if (!OnContextLoadResponse(contextLoadResponse)) {
            return false;
        }
    }

    // Context load response must be provided in first chunk of requests
    if (!HasContextLoadResponse_) {
        OnError("Context load response not provided");
        return false;
    }

    return true;
}

bool TService::TRequestProcessor::ProcessSessionContextItems(NAppHost::TServiceContextPtr& ctx) {
    const auto sessionContextItemRefs = ctx->GetProtobufItemRefs(ITEM_TYPE_SESSION_CONTEXT, NAppHost::EContextItemSelection::Input);

    for (const auto& sessionContextItem : sessionContextItemRefs) {
        NProtobuf::TSessionContext sessionContext;
        try {
            ParseProtobufItem(sessionContextItem, sessionContext);
        } catch (...) {
            OnError(CurrentExceptionMessage());
            return false;
        }
        if (!OnSessionContext(sessionContext)) {
            return false;
        }
    }

    // Session context must be provided in first chunk of requests
    if (!HasSessionContext_) {
        OnError("Session context not provided");
        return false;
    }

    return true;
}

bool TService::TRequestProcessor::ProcessTvmServiceTicket(NAppHost::TServiceContextPtr& ctx) {
    if (!HasTvmServiceTicket_) {
        if (!OnTvmServiceTicket(ctx->GetTvmTicket())) {
            return false;
        }
    }

    // Tvm service ticket must be provided in first chunk of requests
    if (!HasTvmServiceTicket_) {
        OnError("Tvm service ticket not provided");
        return false;
    }

    return true;
}

bool TService::TRequestProcessor::ProcessCuttlefishAudioItems(NAppHost::TServiceContextPtr& ctx) {
    const auto cuttlefishAudioItemRefs = ctx->GetProtobufItemRefs(ITEM_TYPE_AUDIO, NAppHost::EContextItemSelection::Input);

    for (const auto& cuttlefishAudioItem : cuttlefishAudioItemRefs) {
        NAliceProtocol::TAudio audio;
        try {
            ParseProtobufItem(cuttlefishAudioItem, audio);
        } catch (...) {
            OnError(CurrentExceptionMessage());
            return false;
        }
        if (!OnCuttlefishAudio(audio)) {
            return false;
        }
    }

    return true;
}

bool TService::TRequestProcessor::ProcessUnknownRequestItems(NAppHost::TServiceContextPtr& ctx) {
    const auto allInputRquestsItemRefs = ctx->GetProtobufItemRefs(NAppHost::EContextItemSelection::Input);

    for (auto it = allInputRquestsItemRefs.begin(); it != allInputRquestsItemRefs.end(); ++it) {
        // Skip known requests
        if (const auto type = it.GetType();
            type == ITEM_TYPE_CONTEXT_LOAD_RESPONSE
            || type == ITEM_TYPE_SESSION_CONTEXT
            || type == ITEM_TYPE_AUDIO
        ) {
            continue;
        }

        OnUnknownItemType(TString(it.GetTag()), TString(it.GetType()));
    }

    return true;
}

bool TService::TRequestProcessor::OnContextLoadResponse(const NProtobuf::TContextLoadResponse& contextLoadResponse) {
    if (!MusicMatch_) {
        OnError("MusicMatch not initialized on context load response");
        return false;
    }

    HasContextLoadResponse_ = true;
    MusicMatch_->ProcessContextLoadResponse(contextLoadResponse);
    return true;
}

bool TService::TRequestProcessor::OnSessionContext(const NProtobuf::TSessionContext& sessionContext) {
    if (!MusicMatch_) {
        OnError("MusicMatch not initialized on session context");
        return false;
    }

    HasSessionContext_ = true;
    MusicMatch_->ProcessSessionContext(sessionContext);
    return true;
}

bool TService::TRequestProcessor::OnTvmServiceTicket(const TString& tvmServiceTicket) {
    if (!MusicMatch_) {
        OnError("MusicMatch not initialized on tvm service ticket");
        return false;
    }

    if (!tvmServiceTicket) {
        return false;
    }

    HasTvmServiceTicket_ = true;
    MusicMatch_->ProcessTvmServiceTicket(tvmServiceTicket);
    return true;
}

bool TService::TRequestProcessor::OnCuttlefishAudio(const NAliceProtocol::TAudio& audio) {
    switch (audio.GetMessageCase()) {
        case NAliceProtocol::TAudio::MessageCase::kMetaInfoOnly:
            switch (audio.GetMetaInfoCase()) {
                case NAliceProtocol::TAudio::MetaInfoCase::kMusicMatchInitRequest: {
                    if (!OnInitRequest(audio.GetMusicMatchInitRequest())) {
                        return false;
                    }
                    break;
                }
                default: {
                    OnUnknownCuttlefishAudioMessageType(audio);
                    break;
                }
            }
            break;
        case NAliceProtocol::TAudio::MessageCase::kChunk:
            {
                if (ProcessSpotter_) {
                    // Special legacy case when asr and music match streams work simultaneously
                    // Just skip spotter audio
                    if (!OnCuttlefishSpotterAudioChunk(audio.GetChunk())) {
                        return false;
                    }
                } else {
                    NProtobuf::TStreamRequest streamRequest;
                    streamRequest.MutableAddData()->SetAudioData(audio.GetChunk().GetData());
                    if (!OnStreamRequest(streamRequest)) {
                        return false;
                    }
                }
            }
            break;
        case NAliceProtocol::TAudio::MessageCase::kBeginStream:
            HasCuttlefishAudioBeginStream_ = true;
            break;
        case NAliceProtocol::TAudio::MessageCase::kEndStream:
            HasCuttlefishAudioEndStream_ = true;
            break;
        case NAliceProtocol::TAudio::MessageCase::kBeginSpotter:
            ProcessSpotter_ = true;
            break;
        case NAliceProtocol::TAudio::MessageCase::kEndSpotter:
            ProcessSpotter_ = false;
            break;
        default:
            OnUnknownCuttlefishAudioMessageType(audio);
            break;
    }

    return true;
}

bool TService::TRequestProcessor::OnCuttlefishSpotterAudioChunk(const NAliceProtocol::TAudioChunk& audioChunk) {
    Y_UNUSED(audioChunk);
    return true;
}

bool TService::TRequestProcessor::OnInitRequest(const NProtobuf::TInitRequest& initRequest) {
    if (!MusicMatch_) {
        OnError("MusicMatch not initialized on init request");
        return false;
    }

    if (HasInitRequest_) {
        OnWarning("Got more than one InitRequests (ignore extra)");
        return true;
    }

    if (!HasContextLoadResponse_) {
        // Just sanity check
        // This case must be unreachable
        OnError("No context load response was provided before init request sent");
        return false;
    }

    if (!HasSessionContext_) {
        // Just sanity check
        // This case must be unreachable
        OnError("No session context was provided before init request sent");
        return false;
    }

    if (!HasTvmServiceTicket_) {
        // Just sanity check
        // This case must be unreachable
        OnError("No tvm service ticket was provided before init request sent");
        return false;
    }

    HasInitRequest_ = true;
    MusicMatch_->ProcessInitRequest(initRequest);
    return true;
}

bool TService::TRequestProcessor::OnStreamRequest(const NProtobuf::TStreamRequest& streamRequest) {
    if (!MusicMatch_) {
        OnError("MusicMatch not initialized on stream request");
        return false;
    }

    if (!HasContextLoadResponse_) {
        // Just sanity check
        // This case must be unreachable
        OnError("No context load response was provided before stream request sent");
        return false;
    }

    if (!HasSessionContext_) {
        // Just sanity check
        // This case must be unreachable
        OnError("No session context was provided before stream request sent");
        return false;
    }

    if (!HasTvmServiceTicket_) {
        // Just sanity check
        // This case must be unreachable
        OnError("No tvm service ticket was provided before stream request sent");
        return false;
    }

    if (!HasInitRequest_) {
        OnError("No init request was provided before stream request sent");
        return false;
    }

    // Not fatal
    // We still can process the request
    // It's just warning
    if (!HasCuttlefishAudioBeginStream_) {
        OnWarning("No cuttlefish begin audio stream request was provided before stream request sent");
    }
    if (HasCuttlefishAudioEndStream_) {
        OnWarning("Got stream request message after cuttlefish audio end of stream request");
    }

    MusicMatch_->ProcessStreamRequest(streamRequest);
    return true;
}

void TService::TRequestProcessor::OnAppHostClose(NThreading::TPromise<void>& promise, bool requestCancelled) {
    DLOG("RequestProcessor.OnAppHostClose requestCancelled=" << requestCancelled);

    if (!MusicMatch_) {
        promise.SetValue();
        return;
    }

    if (requestCancelled) {
        MusicMatch_->Close();
    } else {
        // Need more time for finish music match
        MusicMatch_->SetFinishPromise(promise);
        MusicMatch_.Reset();
    }
}

void TService::TRequestProcessor::OnError(const TString& error) {
    DLOG("RequestProcessor.OnError: " << error);

    if (!HasInitRequest_) {
        // can not send response for unknown request(format), only log problem
        OnWarning(error);
        return;
    }

    if (MusicMatch_) {
        // for sending responses to apphost can be used another thread, so for avoid race we MUST entrust sending error to music stream impl.
        MusicMatch_->CauseError(error);
    } else {
        OnWarning(error);
    }
}
