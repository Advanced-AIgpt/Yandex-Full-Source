#pragma once

#include <alice/cuttlefish/library/protos/tts.pb.h>

namespace NAlice::NTtsCache::NProtobuf {
    using TCacheSetRequest = ::NTts::TCacheSetRequest;
    using TCacheGetRequest = ::NTts::TCacheGetRequest;
    using TCacheWarmUpRequest = ::NTts::TCacheWarmUpRequest;

    using TCacheGetResponse = ::NTts::TCacheGetResponse;
    using ECacheGetResponseStatus = ::NTts::ECacheGetResponseStatus;
    using TCacheGetResponseStatus = ::NTts::TCacheGetResponseStatus;

    using TCacheEntry = ::NTts::TCacheEntry;
}
