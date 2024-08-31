#pragma once

#include <alice/cachalot/api/protos/cachalot.pb.h>

namespace NAlice::NAppHostServices {

    class TCachalotCache {
    public:
        [[nodiscard]] static NCachalotProtocol::TGetRequest MakeGetRequest(
            TString cacheKey,
            TString storageTag
        );

        [[nodiscard]] static NCachalotProtocol::TSetRequest MakeSetRequest(
            TString cacheKey,
            TString data,
            TString storageTag
        );

        [[nodiscard]] static NCachalotProtocol::TSetRequest MakeSetRequest(
            TString cacheKey,
            TString data,
            TString storageTag,
            uint64_t ttlSeconds
        );

        [[nodiscard]] static NCachalotProtocol::TDeleteRequest MakeDeleteRequest(
            TString cacheKey,
            TString storageTag
        );
    };

}
