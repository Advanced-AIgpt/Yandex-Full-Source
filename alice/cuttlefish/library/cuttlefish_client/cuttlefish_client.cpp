#include "cuttlefish_client.h"

#include <util/string/builder.h>

using namespace NAlice;
using namespace NAlice::NCuttlefish;

TCuttlefishClient::TCuttlefishClient(const TConfig& config)
    : Config_(config)
    , ApphostClientLocal_(new NAppHost::NClient::TClient())
    , ApphostClient_(*ApphostClientLocal_)
{
    NAppHost::NClient::TFixedGrpcBackend backend(Config_.Host, Config_.Port);
    ApphostClient_.AddOrUpdateBackend("APPHOST", backend);
}

TCuttlefishClient::TCuttlefishClient(const TConfig& config, NAppHost::NClient::TClient& c)
    : Config_(config)
    , ApphostClient_(c)
{
}

NAppHost::NClient::TStream TCuttlefishClient::CreateStream() {
    NAppHost::NClient::TStreamOptions streamOptions;
    streamOptions.Timeout = Config_.Timeout;
    streamOptions.ChunkTimeout = Config_.ChunkRWTimeout;
    streamOptions.Path = Config_.Path;
    TString uuid = "bababababababababfabced5b0473f57"; //TODO:? msg.Header->MessageId
    streamOptions.Guid = GetUuid(uuid);                // ?
    return ApphostClient_.CreateStream("APPHOST", std::move(streamOptions));
}

void TCuttlefishClient::TRequest::SendWsEvent(const NAliceProtocol::TWsEvent& wsEvent, bool final) {
    NAppHost::NClient::TInputDataChunk dataChunk;
    dataChunk.AddItem("INIT", ItemType_, wsEvent);
    Stream_.Write(std::move(dataChunk), final);
}

void TCuttlefishClient::TRequest::SendAsrResponse(const NAlice::NAsr::NProtobuf::TResponse& response, bool final) {
    NAppHost::NClient::TInputDataChunk dataChunk;
    dataChunk.AddItem("INIT", TString(NCuttlefish::ITEM_TYPE_ASR_PROTO_RESPONSE), response);
    Stream_.Write(std::move(dataChunk), final);
}

bool TCuttlefishClient::TRequest::NextAsyncRead() {
    if (!NeedNextRead_) {
        return false;
    }

    Stream_.Read().Subscribe([client = TPtr(this), execTimer = THPTimer{}](auto future) mutable {
        client->OnSubscibeCallback(future);
    });
    return true;
}

void TCuttlefishClient::TRequest::OnSubscibeCallback(NThreading::TFuture<TMaybe<NAppHost::NClient::TOutputDataChunk>> future) {
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

void TCuttlefishClient::TRequest::OnResponse(const NAppHost::NClient::TOutputDataChunk& dataChunk) {
    for (const auto& item : dataChunk.GetAllItems()) {
        try {
            if (item.GetType() == ITEM_TYPE_AUDIO) {
                auto audio = item.ParseProtobuf<NAliceProtocol::TAudio>();
                OnCuttlefishResponse(audio);
                continue;
            } else if (item.GetType() == ITEM_TYPE_DIRECTIVE) {
                auto directive = item.ParseProtobuf<NAliceProtocol::TDirective>();
                OnCuttlefishResponse(directive);
                continue;
            } else if (item.GetType() == ITEM_TYPE_WS_MESSAGE) {
                auto wsEvent = item.ParseProtobuf<NAliceProtocol::TWsEvent>();
                OnCuttlefishResponse(wsEvent);
                continue;
            } else if (item.GetType() == ITEM_TYPE_TTS_BACKEND_REQUEST) {
                auto ttsRequest = item.ParseProtobuf<NAlice::NTts::NProtobuf::TBackendRequest>();
                OnCuttlefishResponse(ttsRequest);
                continue;
            }
        } catch (...) {
            OnError(TStringBuilder() << "fail on process response type=" << item.GetType() << ": " << CurrentExceptionMessage());
        }
    }

    OnEndNextResponseProcessing();
}
