PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(
    g:megamind
    petrk
)

SRCS(
    protos.proto
)

EXCLUDE_TAGS(GO_PROTO)

END()
