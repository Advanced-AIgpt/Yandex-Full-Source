PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(
    jan-fazli
    g:hollywood
)

SRCS(
    gif.proto
)

EXCLUDE_TAGS(GO_PROTO)

END()
