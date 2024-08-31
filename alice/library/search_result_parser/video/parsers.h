#pragma once

#include <alice/library/proto/proto_adapter.h>
#include <alice/library/logger/logger.h>
#include <alice/protos/data/tv/carousel.pb.h>
#include <alice/protos/data/search_result/tv_search_result.pb.h>
#include <alice/protos/data/video/video.pb.h>

#include <library/cpp/json/writer/json_value.h>

namespace NAlice::NHollywood::NVideo {

namespace SearchResultParser {
    TTvSearchResultData ParseJsonResponse(const NJson::TJsonValue& response, TRTLogger& logger, bool useHalfPiratesFromBaseInfo = false);
    TTvSearchResultData ParseProtoResponse(const TProtoAdapter& response, TRTLogger& logger, bool useHalfPiratesFromBaseInfo = false);

    template<typename SomeValue>
    extern TMaybe<NTv::TCarouselItemWrapper> ParseBaseInfo(const SomeValue& entityData, TRTLogger& logger);

    template<typename SomeValue>
    extern TMaybe<NTv::TCarousel> ParseParentCollectionObjects(const SomeValue& entityData, TRTLogger& logger);

    template<typename SomeValue>
    extern TMaybe<NProtoBuf::RepeatedPtrField<NTv::TCarousel>> ParseRelatedObjects(const SomeValue& entityData, TRTLogger& logger);

    template<typename SomeValue>
    extern NProtoBuf::RepeatedPtrField<NTv::TCarouselItemWrapper> ParseClips(const SomeValue& response, TRTLogger& logger);
};

} // namespace NAlice::NHollywood::NVideo
