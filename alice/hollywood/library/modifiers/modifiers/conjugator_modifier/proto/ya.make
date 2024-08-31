PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(
    g:hollywood
)

SRCS(
    conjugatable_scenarios_config.proto
)

PEERDIR(
    alice/protos/data/language
)

EXCLUDE_TAGS(GO_PROTO)

END()
