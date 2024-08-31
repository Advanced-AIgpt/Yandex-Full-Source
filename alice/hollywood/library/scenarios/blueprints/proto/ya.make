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
    blueprints.proto
    blueprints_fastdata.proto
)

EXCLUDE_TAGS(GO_PROTO)

END()
