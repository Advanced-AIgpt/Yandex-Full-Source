PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(
    {{username}}
    g:hollywood
)

PEERDIR(
    alice/hollywood/library/framework/proto
    alice/megamind/protos/common
    alice/protos/data/entities
)

SRCS(
    {{scenario_name}}.proto
)

EXCLUDE_TAGS(GO_PROTO)

END()
