PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(
    d-dima
    g:hollywood
)

PEERDIR(
    alice/hollywood/library/framework/proto
    alice/megamind/protos/common
    alice/protos/data/entities
)

SRCS(
    test_scenario.proto
)

EXCLUDE_TAGS(GO_PROTO)

END()
