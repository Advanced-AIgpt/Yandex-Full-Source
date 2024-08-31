PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(
    vl-trifonov
    skorodumov-s
    olegator
    g:alice_quality
)

SRCS(
    io.proto
)

EXCLUDE_TAGS(GO_PROTO)

END()

