PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(
    smirnovpavel
    g:alice_quality
    g:begemot
)

PEERDIR(
    search/begemot/core/proto
)

SRCS(
    parser_result.proto
)

EXCLUDE_TAGS(GO_PROTO)

END()
