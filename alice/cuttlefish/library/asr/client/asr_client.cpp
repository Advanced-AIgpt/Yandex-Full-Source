#include "asr_client.h"
#include <alice/cuttlefish/library/protos/audio.pb.h>

#include <util/string/builder.h>

using namespace NAlice;
using namespace NAlice::NAsr;

TAsrClient::TAsrClient(const TConfig& config)
    : Config_(config)
    , ApphostClientLocal_(new NAppHost::NClient::TClient())
    , ApphostClient_(*ApphostClientLocal_)
{
    NAppHost::NClient::TFixedGrpcBackend backend(Config_.Host, Config_.Port);
    ApphostClient_.AddOrUpdateBackend("APPHOST", backend);
}

TAsrClient::TAsrClient(const TConfig& config, NAppHost::NClient::TClient& c)
    : Config_(config)
    , ApphostClient_(c)
{
}

NAppHost::NClient::TStream TAsrClient::CreateStream() {
    NAppHost::NClient::TStreamOptions streamOptions;
    streamOptions.Timeout = Config_.Timeout;
    streamOptions.ChunkTimeout = Config_.ChunkRWTimeout;
    streamOptions.Path = Config_.Path;
    TString uuid = "bababababababababfabced5b0473f57"; //TODO:? msg.Header->MessageId
    streamOptions.Guid = GetUuid(uuid);                // ?
    return ApphostClient_.CreateStream("APPHOST", std::move(streamOptions));
}

void TAsrClient::TRequest::SendInit(NProtobuf::TInitRequest&& initRequest, bool appHostGraph) {
    ItemType_ = "audio";  //TODO move to const
    NAppHost::NClient::TInputDataChunk dataChunk;
    TStringStream ss;
    if (appHostGraph) {
        ss << TStringBuf("asr_") << initRequest.GetLang() << TStringBuf("_");
    }
    dataChunk.AddFlag("INIT", ss.Str());  // flag for fallback (lang only)
    if (appHostGraph) {
        ss << TStringBuf("_") << initRequest.GetTopic();
    }
    dataChunk.AddFlag("INIT", ss.Str());  // flag for main route (to specific topic)
    // generate audio request (stream begin, spotter begin if need, asr init request)
    {
        NAliceProtocol::TAudio audio;
        auto& beginStream = *audio.MutableBeginStream();
        if (initRequest.HasMime()) {
            beginStream.SetMime(initRequest.GetMime());
        }
        dataChunk.AddItem("INIT", ItemType_, audio);
    }
    if (initRequest.HasHasSpotterPart() && initRequest.GetHasSpotterPart()) {
        NAliceProtocol::TAudio audio;
        audio.MutableBeginSpotter();
        dataChunk.AddItem("INIT", ItemType_, audio);
    }
    {
        NAliceProtocol::TAudio audio;
        audio.MutableMetaInfoOnly();
        audio.MutableAsrInitRequest()->Swap(&initRequest);
        dataChunk.AddItem("INIT", ItemType_, audio);
    }
    Stream_.Write(std::move(dataChunk), false);
    NextAsyncRead();
}

void TAsrClient::TRequest::SendAudioChunk(const void* data, size_t dataSize) {
    NAliceProtocol::TAudio audio;
    audio.MutableChunk()->SetData(data, dataSize);
    NAppHost::NClient::TInputDataChunk dataChunk;
    dataChunk.AddItem("INIT", ItemType_, audio);
    Stream_.Write(std::move(dataChunk), false);
}

void TAsrClient::TRequest::SendEndSpotter() {
    NAliceProtocol::TAudio audio;
    audio.MutableEndSpotter();
    NAppHost::NClient::TInputDataChunk dataChunk;
    dataChunk.AddItem("INIT", ItemType_, audio);
    Stream_.Write(std::move(dataChunk), false);
}

void TAsrClient::TRequest::SendEndStream() {
    NAliceProtocol::TAudio audio;
    audio.MutableEndStream();
    NAppHost::NClient::TInputDataChunk dataChunk;
    dataChunk.AddItem("INIT", ItemType_, audio);
    Stream_.Write(std::move(dataChunk), false);  //TODO? true?
}

bool TAsrClient::TRequest::NextAsyncRead() {
    if (!NeedNextRead_) {
        return false;
    }

    Stream_.Read().Subscribe([client = TPtr(this), execTimer = THPTimer{}](auto future) mutable {
        client->OnSubscibeCallback(future);
    });
    return true;
}

void TAsrClient::TRequest::OnSubscibeCallback(NThreading::TFuture<TMaybe<NAppHost::NClient::TOutputDataChunk>> future) {
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

void TAsrClient::TRequest::OnResponse(const NAppHost::NClient::TOutputDataChunk& dataChunk) {
    for (const auto& item : dataChunk.GetAllItems()) {
        try {
            if (item.GetType() == "asr_proto_response") {
                auto asrResponse = item.ParseProtobuf<NProtobuf::TResponse>();
                OnAsrResponse(asrResponse);
                continue;
            }
        } catch (...) {
            OnError(TStringBuilder() << "fail on process response type=" << item.GetType() << ": " << CurrentExceptionMessage());
        }
    }

    OnEndNextResponseProcessing();
}
