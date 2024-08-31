#pragma once

#include "metrics.h"

#include <alice/cuttlefish/library/tts/cache/base/callbacks_handler.h>
#include <alice/cuttlefish/library/logging/log_context.h>

namespace NAlice::NTtsCacheProxy {
    // Add logging and metrics to callbacks usage
    class TTtsCacheCallbacksWithEventlog: public NTtsCache::TCallbacksHandler {
    public:
        TTtsCacheCallbacksWithEventlog(
            NCuttlefish::TInputAppHostAsyncRequestHandlerPtr requestHandler,
            NRTLog::TRequestLoggerPtr rtLogger,
            TAtomicBase requestNumber
        );

        void OnCacheSetRequestCompleted(const TString& key, const TMaybe<TString>& error) override;
        void OnCacheWarmUpRequestCompleted(const TString& key, const TMaybe<TString>& error) override;

        void OnCacheGetResponse(const NTtsCache::NProtobuf::TCacheGetResponse& cacheGetResponse) override;

        void OnAnyError(const TString& error) override;

    protected:
        void AddCacheGetResponseAndFlush(const NTtsCache::NProtobuf::TCacheGetResponse& cacheGetResponse) override;
        void FlushAppHostContext(bool isFinalFlush) override;

    private:
        static constexpr TStringBuf SOURCE_NAME = "tts_cache_callbacks";
        TSourceMetrics Metrics_;

        NCuttlefish::TLogContext LogContext_;
        const TAtomicBase RequestNumber_;
    };
}
