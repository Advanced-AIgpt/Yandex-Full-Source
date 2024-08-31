PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(g:alice_boltalka g:zeliboba)

PEERDIR(
    alice/boltalka/generative/inference/core/proto
    alice/library/logger/proto
    kernel/server/protos
)

SRCS(
    config.proto
)

EXCLUDE_TAGS(GO_PROTO)

END()
