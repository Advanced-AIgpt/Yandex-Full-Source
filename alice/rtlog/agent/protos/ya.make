PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(
    gusev-p
)

SRCS(
    config.proto
    rtlog_agent.proto
)

PEERDIR(
    alice/rtlog/protos
)

EXCLUDE_TAGS(GO_PROTO)

END()
