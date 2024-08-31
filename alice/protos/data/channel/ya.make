PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(
    g:alice
    g:yandex_io
)

INCLUDE_TAGS(GO_PROTO)

SRCS(
    channel.proto
)

END()
