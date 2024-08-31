#pragma once

#include <alice/cachalot/library/modules/cache/request.h>
#include <alice/cachalot/library/modules/cache/storage_tag_to_context.h>

namespace NCachalot {

TRequestPtr MakeCacheGetRequestFromProto(
    NCachalotProtocol::TGetRequest protoRequest,
    const TStorageTag2Context& storageTag2Context,
    NAppHost::TServiceContextPtr ahCtx
);

TRequestPtr MakeCacheSetRequestFromProto(
    NCachalotProtocol::TSetRequest protoRequest,
    const TStorageTag2Context& storageTag2Context,
    NAppHost::TServiceContextPtr ahCtx
);

TRequestPtr MakeCacheDeleteRequestFromProto(
    NCachalotProtocol::TDeleteRequest protoRequest,
    const TStorageTag2Context& storageTag2Context,
    NAppHost::TServiceContextPtr ahCtx
);

} // namespace NCachalot
