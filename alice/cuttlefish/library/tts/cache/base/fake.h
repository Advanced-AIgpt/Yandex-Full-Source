#pragma once

#include "interface.h"

namespace NAlice::NTtsCache {
    class TFake : public TInterface {
    public:
        explicit TFake(TIntrusivePtr<TCallbacks> callbacks)
            : TInterface(callbacks)
        {
        }

        void ProcessCacheSetRequest(const NProtobuf::TCacheSetRequest& cacheSetRequest) override;
        void ProcessCacheGetRequest(const NProtobuf::TCacheGetRequest& cacheGetRequest) override;
        void ProcessCacheWarmUpRequest(const NProtobuf::TCacheWarmUpRequest& cacheWarmUpRequest) override;

        void Cancel() override;

    private:
        bool Closed_ = false;
    };
}
