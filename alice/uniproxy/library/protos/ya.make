PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(
    g:voicetech-infra
)

SRCS(
    ttscache.proto
    uniproxy.proto
    notificator.proto
)

PEERDIR(
    mssngr/router/lib/protos
    alice/megamind/protos/scenarios
    alice/megamind/protos/speechkit
    voicetech/library/proto_api
)

EXCLUDE_TAGS(GO_PROTO)

END()
