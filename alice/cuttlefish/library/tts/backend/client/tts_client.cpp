#include "tts_client.h"

#include <alice/cuttlefish/library/cuttlefish/common/item_types.h>

#include <util/string/builder.h>

using namespace NAlice;

TTtsClient::TTtsClient(const TConfig& config)
    : Config_(config)
{
    NAppHost::NClient::TFixedGrpcBackend backend(Config_.Host, Config_.Port);
    ApphostClient_.AddOrUpdateBackend("APPHOST", backend);
}

NAppHost::NClient::TStream TTtsClient::CreateStream() {
    NAppHost::NClient::TStreamOptions streamOptions;
    streamOptions.Timeout = Config_.Timeout;
    streamOptions.Path = Config_.Path;
    TString uuid = "bababababababababfabced5b0473f57";
    streamOptions.Guid = GetUuid(uuid);
    return ApphostClient_.CreateStream("APPHOST", std::move(streamOptions));
}

void TTtsClient::TRequest::SendBackendRequest(::NTts::TBackendRequest&& backendRequest) {
    NAppHost::NClient::TInputDataChunk dataChunk;
    dataChunk.AddItem("INIT", TString(NCuttlefish::ITEM_TYPE_TTS_BACKEND_REQUEST), backendRequest);

    Stream_.Write(std::move(dataChunk), false);
}

void TTtsClient::TRequest::SendEndOfStream() {
    Stream_.WritesDone();
}

void TTtsClient::TRequest::Cancel() {
    Stream_.Cancel();
}

bool TTtsClient::TRequest::NextAsyncRead() {
    if (!NeedNextRead_) {
        return false;
    }

    Stream_.Read().Subscribe([client = TPtr(this), execTimer = THPTimer{}](auto future) mutable {
        client->OnSubscibeCallback(future);
    });
    return true;
}

void TTtsClient::TRequest::OnSubscibeCallback(NThreading::TFuture<TMaybe<NAppHost::NClient::TOutputDataChunk>> future) {
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
        TString currentExceptionMessage = CurrentExceptionMessage();

        DLOG(TStringBuilder() << "stream_error: " << currentExceptionMessage);
        IOService_.Post([client = TPtr(this), currentExceptionMessage = std::move(currentExceptionMessage)]() mutable {
            client->OnError(TStringBuilder() << "TStreamError: " << currentExceptionMessage);
        });
    }
}

void TTtsClient::TRequest::OnResponse(const NAppHost::NClient::TOutputDataChunk& response) {
    for (const auto& item : response.GetAllItems()) {
        try {
            if (item.GetType() == "audio") {
                auto audio = item.ParseProtobuf<::NAliceProtocol::TAudio>();
                OnProtocolAudio(audio);
                if (!NeedNextRead_) {
                    break;
                }
            } else {
                DLOG("OnResponse: " << item.GetType());
            }
        } catch (...) {
            OnError(TStringBuilder() << "fail on process response type=" << item.GetType() << ": " << CurrentExceptionMessage());
        }
    }

    OnEndNextResponseProcessing();
}
