#include "cachalot_client.h"

#include <apphost/api/client/stream_error.h>

#include <util/string/builder.h>

using namespace NAlice;

TCachalotClient::TCachalotClient(const TConfig& config)
    : Config_(config)
    , ApphostClientLocal_(new NAppHost::NClient::TClient())
    , ApphostClient_(*ApphostClientLocal_)
{
    NAppHost::NClient::TFixedGrpcBackend backend(Config_.Host, Config_.Port);
    ApphostClient_.AddOrUpdateBackend("APPHOST", backend);
}

TCachalotClient::TCachalotClient(const TConfig& config, NAppHost::NClient::TClient& c)
    : Config_(config)
    , ApphostClient_(c)
{
}

NAppHost::NClient::TStream TCachalotClient::CreateStream() {
    NAppHost::NClient::TStreamOptions streamOptions;
    streamOptions.Timeout = Config_.Timeout;
    streamOptions.ChunkTimeout = Config_.ChunkRWTimeout;
    streamOptions.Path = Config_.Path;
    TString uuid = "bababababababababfabced5b0473f57"; //TODO:? msg.Header->MessageId
    streamOptions.Guid = GetUuid(uuid);                // ?
    return ApphostClient_.CreateStream("APPHOST", std::move(streamOptions));
}

void TCachalotClient::TRequest::SendYabioContextRequest(NCachalotProtocol::TYabioContextRequest&& request, bool appHostGraph) {
    (void)appHostGraph;
    TStringOutput so(ItemType_);
    so << TStringBuf("yabio_context_request");
    NAppHost::NClient::TInputDataChunk dataChunk;
    dataChunk.AddItem("INIT", ItemType_, request);
    Stream_.Write(std::move(dataChunk), true);
    NextAsyncRead();
}

bool TCachalotClient::TRequest::NextAsyncRead() {
    if (!NeedNextRead_) {
        return false;
    }

    Stream_.Read().Subscribe([client = TPtr(this), execTimer = THPTimer{}](auto future) mutable {
        client->OnSubscibeCallback(future);
    });
    return true;
}

void TCachalotClient::TRequest::OnSubscibeCallback(NThreading::TFuture<TMaybe<NAppHost::NClient::TOutputDataChunk>> future) {
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
    } catch (const NAppHost::NClient::TStreamError& exception) {
        OnError(TStringBuilder() << "stream_error: " << CurrentExceptionMessage());
        IOService_.Post([client = TPtr(this)]() mutable {
            client->OnError(TStringBuilder() << "TStreamError: " << CurrentExceptionMessage());
        });
    }
}

void TCachalotClient::TRequest::OnResponse(const NAppHost::NClient::TOutputDataChunk& dataChunk) {
    for (const auto& item : dataChunk.GetAllItems()) {
        try {
            if (item.GetType() == TStringBuf("yabio_context_response")) {
                auto response = item.ParseProtobuf<NCachalotProtocol::TYabioContextResponse>();
                OnYabioContextResponse(response);
                continue;
            } //TODO: more responses types
        } catch (...) {
            OnError(TStringBuilder() << "fail on process response type=" << item.GetType() << ": " << CurrentExceptionMessage());
        }
    }

    OnEndNextResponseProcessing();
}
