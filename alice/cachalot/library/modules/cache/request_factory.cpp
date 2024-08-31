#include "request_factory.h"

namespace NCachalot {

template<typename TProtoRequest, typename TCacheRequest>
TRequestPtr MakeCacheRequestFromProto(
    TProtoRequest&& protoRequest,
    const TStorageTag2Context& storageTag2Context,
    NAppHost::TServiceContextPtr ahCtx
) {
    auto requestContext = (
        protoRequest.HasStorageTag() ?
            storageTag2Context[protoRequest.GetStorageTag()] :
            storageTag2Context.Default()
    );
    auto protoRequestPtr = MakeAtomicShared<TProtoRequest>(std::forward<TProtoRequest>(protoRequest));
    auto domainRequestPtr = MakeIntrusive<TCacheRequest>(protoRequestPtr, std::move(ahCtx), requestContext);

    return domainRequestPtr;
}

TRequestPtr MakeCacheGetRequestFromProto(
    NCachalotProtocol::TGetRequest protoRequest,
    const TStorageTag2Context& storageTag2Context,
    NAppHost::TServiceContextPtr ahCtx
) {
    return MakeCacheRequestFromProto<NCachalotProtocol::TGetRequest, TRequestCacheGet>(
        std::move(protoRequest),
        storageTag2Context,
        std::move(ahCtx)
    );
}

TRequestPtr MakeCacheSetRequestFromProto(
    NCachalotProtocol::TSetRequest protoRequest,
    const TStorageTag2Context& storageTag2Context,
    NAppHost::TServiceContextPtr ahCtx
) {
    return MakeCacheRequestFromProto<NCachalotProtocol::TSetRequest, TRequestCacheSet>(
        std::move(protoRequest),
        storageTag2Context,
        std::move(ahCtx)
    );
}

TRequestPtr MakeCacheDeleteRequestFromProto(
    NCachalotProtocol::TDeleteRequest protoRequest,
    const TStorageTag2Context& storageTag2Context,
    NAppHost::TServiceContextPtr ahCtx
) {
    return MakeCacheRequestFromProto<NCachalotProtocol::TDeleteRequest, TRequestCacheDelete>(
        std::move(protoRequest),
        storageTag2Context,
        std::move(ahCtx)
    );
}

} // namespace NCachalot
