#include "music_match_client.h"

#include <alice/cuttlefish/library/cuttlefish/common/edge_flags.h>
#include <alice/cuttlefish/library/cuttlefish/common/item_types.h>
#include <alice/cuttlefish/library/protos/audio.pb.h>

#include <util/string/builder.h>

using namespace NAlice;
using namespace NAlice::NMusicMatch;

TMusicMatchClient::TMusicMatchClient(const TConfig& config)
    : Config_(config)
    , AppHostClientLocal_(new NAppHost::NClient::TClient())
    , AppHostClient_(*AppHostClientLocal_)
{
    NAppHost::NClient::TFixedGrpcBackend backend(Config_.Host, Config_.Port);
    AppHostClient_.AddOrUpdateBackend("APPHOST", backend);
}

TMusicMatchClient::TMusicMatchClient(const TConfig& config, NAppHost::NClient::TClient& appHostClient)
    : Config_(config)
    , AppHostClient_(appHostClient)
{
}

NAppHost::NClient::TStream TMusicMatchClient::CreateStream() {
    NAppHost::NClient::TStreamOptions streamOptions;

    streamOptions.Timeout = Config_.Timeout;
    streamOptions.ChunkTimeout = Config_.ChunkRWTimeout;
    streamOptions.Path = Config_.Path;
    TString uuid = "bababababababababfabced5b0473f57"; //TODO:? msg.Header->MessageId
    streamOptions.Guid = GetUuid(uuid);                // ?
    streamOptions.TvmTicket = Config_.TvmTicket;

    return AppHostClient_.CreateStream("APPHOST", std::move(streamOptions));
}

void TMusicMatchClient::TRequest::SendMusicMatchFlag() {
    NAppHost::NClient::TInputDataChunk dataChunk;
    dataChunk.AddFlag("WS_ADAPTER_IN", TString(NCuttlefish::EDGE_FLAG_MUSIC_MATCH));

    Stream_.Write(std::move(dataChunk), false);
}

void TMusicMatchClient::TRequest::SendContextLoadResponseAndSessionContext(
    NProtobuf::TContextLoadResponse&& contextLoadResponse,
    NProtobuf::TSessionContext&& sessionContext
) {
    NAppHost::NClient::TInputDataChunk dataChunk;
    dataChunk.AddItem("CONTEXT_LOAD", TString(NCuttlefish::ITEM_TYPE_CONTEXT_LOAD_RESPONSE), contextLoadResponse);
    dataChunk.AddItem("INIT", TString(NCuttlefish::ITEM_TYPE_SESSION_CONTEXT), sessionContext);

    Stream_.Write(std::move(dataChunk), false);
}

void TMusicMatchClient::TRequest::SendInitRequest(NProtobuf::TInitRequest&& initRequest) {
    NAppHost::NClient::TInputDataChunk dataChunk;
    {
        NAliceProtocol::TAudio audio;
        audio.MutableMetaInfoOnly();
        audio.MutableMusicMatchInitRequest()->Swap(&initRequest);
        dataChunk.AddItem("WS_ADAPTER_IN", TString(NCuttlefish::ITEM_TYPE_AUDIO), audio);
    }

    Stream_.Write(std::move(dataChunk), false);
}

void TMusicMatchClient::TRequest::SendBeginStream(const TString& audioFormat) {
    NAppHost::NClient::TInputDataChunk dataChunk;
    {
        NAliceProtocol::TAudio audio;
        audio.MutableBeginStream()->SetMime(audioFormat);
        dataChunk.AddItem("WS_ADAPTER_IN", TString(NCuttlefish::ITEM_TYPE_AUDIO), audio);
    }

    Stream_.Write(std::move(dataChunk), false);
}

void TMusicMatchClient::TRequest::SendEndStream() {
    NAppHost::NClient::TInputDataChunk dataChunk;
    {
        NAliceProtocol::TAudio audio;
        audio.MutableEndStream();
        dataChunk.AddItem("WS_ADAPTER_IN", TString(NCuttlefish::ITEM_TYPE_AUDIO), audio);
    }

    Stream_.Write(std::move(dataChunk), false);
}

void TMusicMatchClient::TRequest::SendBeginSpotter() {
    NAppHost::NClient::TInputDataChunk dataChunk;
    {
        NAliceProtocol::TAudio audio;
        audio.MutableBeginSpotter();
        dataChunk.AddItem("WS_ADAPTER_IN", TString(NCuttlefish::ITEM_TYPE_AUDIO), audio);
    }

    Stream_.Write(std::move(dataChunk), false);
}

void TMusicMatchClient::TRequest::SendEndSpotter() {
    NAppHost::NClient::TInputDataChunk dataChunk;
    {
        NAliceProtocol::TAudio audio;
        audio.MutableEndSpotter();
        dataChunk.AddItem("WS_ADAPTER_IN", TString(NCuttlefish::ITEM_TYPE_AUDIO), audio);
    }

    Stream_.Write(std::move(dataChunk), false);
}

void TMusicMatchClient::TRequest::SendAudioChunk(const void* data, size_t dataSize) {
    NAppHost::NClient::TInputDataChunk dataChunk;
    {
        NAliceProtocol::TAudio audio;
        audio.MutableChunk()->SetData(data, dataSize);
        dataChunk.AddItem("WS_ADAPTER_IN", TString(NCuttlefish::ITEM_TYPE_AUDIO), audio);
    }

    Stream_.Write(std::move(dataChunk), false);
}

bool TMusicMatchClient::TRequest::NextAsyncRead() {
    if (!NeedNextRead_) {
        return false;
    }

    Stream_.Read().Subscribe([client = TPtr(this), execTimer = THPTimer{}](auto future) mutable {
        client->OnSubscibeCallback(future);
    });
    return true;
}

void TMusicMatchClient::TRequest::OnSubscibeCallback(NThreading::TFuture<TMaybe<NAppHost::NClient::TOutputDataChunk>> future) {
    try {
        auto response = future.GetValueSync();
        IOService_.Post([client = TPtr(this), response = std::move(response)]() mutable {
            try {
                if (response.Defined()) {
                    client->OnResponse(*response);
                } else {
                    DLOG("Response not defined");
                    // TODO(chegoryu) handle
                }
            } catch (...) {
                client->OnError(CurrentExceptionMessage());
            }
        });
    } catch (const NAppHost::NClient::TStreamTimeout&) {
        IOService_.Post([client = TPtr(this)]() mutable {
            client->OnTimeout();
        });
    } catch (const NAppHost::NClient::TStreamError&) {
        TString currentExceptionMessage = CurrentExceptionMessage();

        DLOG(TStringBuilder() << "stream_error: " << currentExceptionMessage);
        IOService_.Post([client = TPtr(this), currentExceptionMessage = std::move(currentExceptionMessage)]() mutable {
            client->OnError(TStringBuilder() << "TStreamError: " << currentExceptionMessage);
        });
    }
}

void TMusicMatchClient::TRequest::OnResponse(const NAppHost::NClient::TOutputDataChunk& dataChunk) {
    for (const auto& item : dataChunk.GetAllItems()) {
        try {
            if (item.GetType() == NCuttlefish::ITEM_TYPE_MUSIC_MATCH_INIT_RESPONSE) {
                auto initResponse = item.ParseProtobuf<NProtobuf::TInitResponse>();
                OnInitResponse(initResponse);
                continue;
            }
            if (item.GetType() == NCuttlefish::ITEM_TYPE_MUSIC_MATCH_STREAM_RESPONSE) {
                auto streamResponse = item.ParseProtobuf<NProtobuf::TStreamResponse>();
                OnStreamResponse(streamResponse);
                continue;
            }
        } catch (...) {
            OnError(TStringBuilder() << "fail on process response type=" << item.GetType() << ": " << CurrentExceptionMessage());
        }
    }

    OnEndNextResponseProcessing();
}
