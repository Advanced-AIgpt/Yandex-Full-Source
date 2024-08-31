PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(
    g:hollywood
)

SRCS(
    used_scenarios.proto
)

EXCLUDE_TAGS(GO_PROTO)

END()
