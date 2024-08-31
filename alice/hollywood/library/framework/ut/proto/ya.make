PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(
    g:hollywod
)

SRCS(
    protos_ut.proto
    render_div2_test.proto
)

EXCLUDE_TAGS(GO_PROTO)

END()
