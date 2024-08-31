#include "tts_cache_client.h"

#include <alice/cuttlefish/library/cuttlefish/common/item_types.h>

#include <util/string/builder.h>

using namespace NAlice;

TTtsCacheClient::TTtsCacheClient(const TConfig& config)
    : Config_(config)
{
    NAppHost::NClient::TFixedGrpcBackend backend(Config_.Host, Config_.Port);
    ApphostClient_.AddOrUpdateBackend("APPHOST", backend);
}

NAppHost::NClient::TStream TTtsCacheClient::CreateStream() {
    NAppHost::NClient::TStreamOptions streamOptions;
    streamOptions.Timeout = Config_.Timeout;
    streamOptions.Path = Config_.Path;
    TString uuid = "bababababababababfabced5b0473f57";
    streamOptions.Guid = GetUuid(uuid);

    return ApphostClient_.CreateStream("APPHOST", std::move(streamOptions));
}

void TTtsCacheClient::TRequest::SendCacheSetRequest(NTtsCache::NProtobuf::TCacheSetRequest&& cacheSetRequest) {
    NAppHost::NClient::TInputDataChunk dataChunk;
    dataChunk.AddItem("INIT", TString(NCuttlefish::ITEM_TYPE_TTS_CACHE_SET_REQUEST), cacheSetRequest);

    Stream_.Write(std::move(dataChunk), false);
}

void TTtsCacheClient::TRequest::SendCacheGetRequest(NTtsCache::NProtobuf::TCacheGetRequest&& cacheGetRequest) {
    NAppHost::NClient::TInputDataChunk dataChunk;
    dataChunk.AddItem("INIT", TString(NCuttlefish::ITEM_TYPE_TTS_CACHE_GET_REQUEST), cacheGetRequest);

    Stream_.Write(std::move(dataChunk), false);
}

void TTtsCacheClient::TRequest::SendCacheWarmUpRequest(NTtsCache::NProtobuf::TCacheWarmUpRequest&& cacheWarmUpRequest) {
    NAppHost::NClient::TInputDataChunk dataChunk;
    dataChunk.AddItem("INIT", TString(NCuttlefish::ITEM_TYPE_TTS_CACHE_WARM_UP_REQUEST), cacheWarmUpRequest);

    Stream_.Write(std::move(dataChunk), false);
}

void TTtsCacheClient::TRequest::SendEndOfStream() {
    Stream_.WritesDone();
}

void TTtsCacheClient::TRequest::Cancel() {
    Stream_.Cancel();
}

bool TTtsCacheClient::TRequest::NextAsyncRead() {
    if (!NeedNextRead_) {
        return false;
    }

    Stream_.Read().Subscribe([client = TPtr(this), execTimer = THPTimer{}](auto future) mutable {
        client->OnSubscibeCallback(future);
    });
    return true;
}

void TTtsCacheClient::TRequest::OnSubscibeCallback(NThreading::TFuture<TMaybe<NAppHost::NClient::TOutputDataChunk>> future) {
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
    } catch (const NAppHost::NClient::TStreamError&) {
        TString currentExceptionMessage = CurrentExceptionMessage();

        DLOG(TStringBuilder() << "stream_error: " << currentExceptionMessage);
        IOService_.Post([client = TPtr(this), currentExceptionMessage = std::move(currentExceptionMessage)]() mutable {
            client->OnError(TStringBuilder() << "TStreamError: " << currentExceptionMessage);
        });
    }
}

void TTtsCacheClient::TRequest::OnResponse(const NAppHost::NClient::TOutputDataChunk& response) {
    for (const auto& item : response.GetAllItems()) {
        try {
            if (item.GetType() == NCuttlefish::ITEM_TYPE_TTS_CACHE_GET_RESPONSE) {
                auto cacheGetResponse = item.ParseProtobuf<NTtsCache::NProtobuf::TCacheGetResponse>();
                OnCacheGetResponse(cacheGetResponse);
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
