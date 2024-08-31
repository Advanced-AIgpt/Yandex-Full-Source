PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(
    makatunkin
    g:megamind
)

PEERDIR(
    alice/memento/proto
    alice/protos/api/nlu/generated
    alice/protos/data/language
)

SRCS(
    combinator_config.proto
    common.proto
    config.proto
    rpc_handler_config.proto
)

EXCLUDE_TAGS(GO_PROTO)

END()
