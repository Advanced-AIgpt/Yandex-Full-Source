
PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(
    vkaneva
    g:goods-runtime
)

SRCS(
    goods.proto
)

EXCLUDE_TAGS(GO_PROTO)

END()