#include "tts_cache_callbacks_with_eventlog.h"

#include <alice/cuttlefish/library/cuttlefish/common/utils.h>

using namespace NAlice::NTtsCache;
using namespace NAlice::NTtsCacheProxy;

TTtsCacheCallbacksWithEventlog::TTtsCacheCallbacksWithEventlog(
    NCuttlefish::TInputAppHostAsyncRequestHandlerPtr requestHandler,
    NRTLog::TRequestLoggerPtr rtLogger,
    TAtomicBase requestNumber
)
    : NTtsCache::TCallbacksHandler(requestHandler)
    , Metrics_(TTtsCacheCallbacksWithEventlog::SOURCE_NAME)
    , LogContext_(NCuttlefish::SpawnLogFrame(), rtLogger)
    , RequestNumber_(requestNumber)
{
    LogContext_.LogEventInfoCombo<NEvClass::TtsCacheCallbacksFrame>(
        RequestNumber_,
        GuidToUuidString(RequestHandler_->Context().GetRequestID()),
        RequestHandler_->Context().GetRUID(),
        TString(RequestHandler_->Context().GetLocation().Path),
        TString(RequestHandler_->Context().GetRemoteHost())
    );
}

void TTtsCacheCallbacksWithEventlog::OnCacheSetRequestCompleted(const TString& key, const TMaybe<TString>& error) {
    if (error.Defined()) {
        Metrics_.PushRate("set", "error");
        LogContext_.LogEventErrorCombo<NEvClass::TtsCacheSetRequestError>(key, *error);
    } else {
        Metrics_.PushRate("set", "ok");
        LogContext_.LogEventInfoCombo<NEvClass::TtsCacheSetRequestCompleted>(key);
    }

    NTtsCache::TCallbacksHandler::OnCacheSetRequestCompleted(key, error);
}

void TTtsCacheCallbacksWithEventlog::OnCacheWarmUpRequestCompleted(const TString& key, const TMaybe<TString>& error) {
    if (error.Defined()) {
        Metrics_.PushRate("warmup", "error");
        LogContext_.LogEventErrorCombo<NEvClass::TtsCacheWarmUpRequestError>(key, *error);
    } else {
        Metrics_.PushRate("warmup", "ok");
        LogContext_.LogEventInfoCombo<NEvClass::TtsCacheWarmUpRequestCompleted>(key);
    }

    NTtsCache::TCallbacksHandler::OnCacheWarmUpRequestCompleted(key, error);
}

void TTtsCacheCallbacksWithEventlog::OnCacheGetResponse(const NProtobuf::TCacheGetResponse& cacheGetResponse) {
    Metrics_.PushRate("get", NTts::ECacheGetResponseStatus_Name(cacheGetResponse.GetStatus()));

    if (cacheGetResponse.GetStatus() == NProtobuf::ECacheGetResponseStatus::HIT) {
        Metrics_.PushRate(cacheGetResponse.GetCacheEntry().GetAudio().size(), "audiosent", "ok");
    }
    if (cacheGetResponse.GetStatus() == NProtobuf::ECacheGetResponseStatus::ERROR) {
        // For easy errors search
        // No completed message because of SendToAppHostTtsCacheGetResponse message (do not double logging)
        LogContext_.LogEventErrorCombo<NEvClass::TtsCacheGetRequestError>(cacheGetResponse.GetKey(), cacheGetResponse.GetErrorMessage());
    }

    NTtsCache::TCallbacksHandler::OnCacheGetResponse(cacheGetResponse);
}

void TTtsCacheCallbacksWithEventlog::OnAnyError(const TString& error) {
    Metrics_.PushRate("anyerror", "error");
    LogContext_.LogEventErrorCombo<NEvClass::ErrorMessage>(error);

    NTtsCache::TCallbacksHandler::OnAnyError(error);
}

void TTtsCacheCallbacksWithEventlog::AddCacheGetResponseAndFlush(const NTtsCache::NProtobuf::TCacheGetResponse& cacheGetResponse) {
    {
        NProtobuf::TCacheGetResponse cacheGetResponseCopy = cacheGetResponse;
        // Do not log audio
        cacheGetResponseCopy.MutableCacheEntry()->ClearAudio();
        if (cacheGetResponse.GetStatus() == NProtobuf::ECacheGetResponseStatus::ERROR) {
            LogContext_.LogEventErrorCombo<NEvClass::SendToAppHostTtsCacheGetResponse>(
                cacheGetResponseCopy.ShortUtf8DebugString(),
                cacheGetResponse.GetCacheEntry().GetAudio().size()
            );
        } else {
            LogContext_.LogEventInfoCombo<NEvClass::SendToAppHostTtsCacheGetResponse>(
                cacheGetResponseCopy.ShortUtf8DebugString(),
                cacheGetResponse.GetCacheEntry().GetAudio().size()
            );
        }
    }

    NTtsCache::TCallbacksHandler::AddCacheGetResponseAndFlush(cacheGetResponse);
}

void TTtsCacheCallbacksWithEventlog::FlushAppHostContext(bool isFinalFlush) {
    NTtsCache::TCallbacksHandler::FlushAppHostContext(isFinalFlush);

    if (isFinalFlush) {
        LogContext_.LogEventInfoCombo<NEvClass::FlushAppHostContext>();
    }
}
