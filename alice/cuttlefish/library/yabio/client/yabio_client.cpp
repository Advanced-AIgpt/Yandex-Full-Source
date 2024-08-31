#include "yabio_client.h"

#include <util/string/builder.h>

using namespace NAlice;
using namespace NAlice::NYabio;

TYabioClient::TYabioClient(const TConfig& config)
    : Config_(config)
    , ApphostClientLocal_(new NAppHost::NClient::TClient())
    , ApphostClient_(*ApphostClientLocal_)
{
    NAppHost::NClient::TFixedGrpcBackend backend(Config_.Host, Config_.Port);
    ApphostClient_.AddOrUpdateBackend("APPHOST", backend);
}

TYabioClient::TYabioClient(const TConfig& config, NAppHost::NClient::TClient& c)
    : Config_(config)
    , ApphostClient_(c)
{
}

NAppHost::NClient::TStream TYabioClient::CreateStream() {
    NAppHost::NClient::TStreamOptions streamOptions;
    streamOptions.Timeout = Config_.Timeout;
    streamOptions.ChunkTimeout = Config_.ChunkRWTimeout;
    streamOptions.Path = Config_.Path;
    TString uuid = "bababababababababfabced5b0473f57"; //TODO:? msg.Header->MessageId
    streamOptions.Guid = GetUuid(uuid);                // ?
    return ApphostClient_.CreateStream("APPHOST", std::move(streamOptions));
}

void TYabioClient::TRequest::SendInit(NProtobuf::TInitRequest&& initRequest, bool appHostGraph) {
    (void)appHostGraph;
    TString mime = initRequest.mime();
    ItemType_ = "audio";
    NAppHost::NClient::TInputDataChunk dataChunk;
    {
        NAliceProtocol::TAudio audio;
        audio.MutableMetaInfoOnly();
        audio.MutableYabioInitRequest()->Swap(&initRequest);
        dataChunk.AddItem("INIT", ItemType_, audio);
    }
    {
        NAliceProtocol::TAudio audio;
        audio.MutableBeginStream()->SetMime(mime);
        dataChunk.AddItem("INIT", ItemType_, audio);
    }
    Stream_.Write(std::move(dataChunk), false);
    NextAsyncRead();
}

void TYabioClient::TRequest::SendAudioChunk(const char* data, size_t size) {
    NAliceProtocol::TAudio audio;
    audio.MutableChunk()->SetData(data, size);
    NAppHost::NClient::TInputDataChunk dataChunk;
    dataChunk.AddItem("INIT", ItemType_, audio);
    Stream_.Write(std::move(dataChunk), false);
}

void TYabioClient::TRequest::SendEndSpotter() {
    NAliceProtocol::TAudio audio;
    audio.MutableEndSpotter();
    NAppHost::NClient::TInputDataChunk dataChunk;
    dataChunk.AddItem("INIT", ItemType_, audio);
    Stream_.Write(std::move(dataChunk), false);
}

void TYabioClient::TRequest::SendEndStream() {
    NAliceProtocol::TAudio audio;
    audio.MutableEndStream();
    NAppHost::NClient::TInputDataChunk dataChunk;
    dataChunk.AddItem("INIT", ItemType_, audio);
    Stream_.Write(std::move(dataChunk), true);
}

bool TYabioClient::TRequest::NextAsyncRead() {
    if (!NeedNextRead_) {
        return false;
    }

    Stream_.Read().Subscribe([client = TPtr(this), execTimer = THPTimer{}](auto future) mutable {
        client->OnSubscibeCallback(future);
    });
    return true;
}

void TYabioClient::TRequest::OnSubscibeCallback(NThreading::TFuture<TMaybe<NAppHost::NClient::TOutputDataChunk>> future) {
    try {
        auto response = future.GetValueSync();
        IOService_.Post([client = TPtr(this), response = std::move(response)]() mutable {
            try {
                if (response.Defined()) {
                    client->OnResponse(*response);
                } else {
                    client->OnEndOfResponsesStream();
                }
            } catch (...) {
                client->OnError(CurrentExceptionMessage());
            }
        });
    } catch (const NAppHost::NClient::TStreamTimeout&) {
        IOService_.Post([client = TPtr(this)]() mutable {
            client->OnTimeout();
        });
    } catch (const NAppHost::NClient::TStreamError& exception) {
        OnError(TStringBuilder() << "stream_error: " << CurrentExceptionMessage());
        IOService_.Post([client = TPtr(this)]() mutable {
            client->OnError(TStringBuilder() << "TStreamError: " << CurrentExceptionMessage());
        });
    }
}

void TYabioClient::TRequest::OnResponse(const NAppHost::NClient::TOutputDataChunk& dataChunk) {
    for (const auto& item : dataChunk.GetAllItems()) {
        try {
            if (item.GetType() == "yabio_proto_response") {
                auto yabioResponse = item.ParseProtobuf<NProtobuf::TResponse>();
                OnYabioResponse(yabioResponse);
                continue;
            }
        } catch (...) {
            OnError(TStringBuilder() << "fail on process response type=" << item.GetType() << ": " << CurrentExceptionMessage());
        }
    }

    OnEndNextResponseProcessing();
}
