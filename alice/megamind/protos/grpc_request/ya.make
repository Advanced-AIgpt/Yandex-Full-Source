PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(g:megamind)

PEERDIR(
    alice/library/client/protos
    alice/megamind/protos/common
    alice/megamind/protos/scenarios
    alice/protos/api/rpc

    mapreduce/yt/interface/protos
)

SRCS(
    analytics_info.proto
    request.proto
    response.proto
)

END()
