PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(
    sdll
    g:hollywood
)

SRCS(
    call_payload.proto
    state.proto
)

EXCLUDE_TAGS(GO_PROTO)

END()
