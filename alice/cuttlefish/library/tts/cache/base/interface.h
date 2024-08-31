#pragma once

#include "protobuf.h"

#include <library/cpp/threading/future/future.h>

#include <util/generic/ptr.h>
#include <util/generic/maybe.h>

namespace NAlice::NTtsCache {
    class TInterface : public TThrRefBase {
    public:
        class TCallbacks : public TThrRefBase {
        public:
            // There are no response to this request
            // However we want to log something
            virtual void OnCacheSetRequestCompleted(const TString& key, const TMaybe<TString>& error) = 0;
            virtual void OnCacheWarmUpRequestCompleted(const TString& key, const TMaybe<TString>& error) = 0;

            virtual void OnCacheGetResponse(const NProtobuf::TCacheGetResponse& cacheGetResponse) = 0;

            virtual void OnClosed() = 0;
            virtual void OnAnyError(const TString& error) = 0;
            virtual void Finish() = 0;
        };

        explicit TInterface(TIntrusivePtr<TCallbacks> callbacks)
            : Callbacks_(callbacks)
        {}

        virtual void ProcessCacheSetRequest(const NProtobuf::TCacheSetRequest& cacheSetRequest) = 0;
        virtual void ProcessCacheGetRequest(const NProtobuf::TCacheGetRequest& cacheGetRequest) = 0;
        virtual void ProcessCacheWarmUpRequest(const NProtobuf::TCacheWarmUpRequest& cacheWarmUpRequest) = 0;

        virtual void Cancel() = 0;

    protected:
        TIntrusivePtr<TCallbacks> Callbacks_;
    };
}
