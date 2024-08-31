
PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

INCLUDE_TAGS(GO_PROTO)

OWNER(
    vkaneva
    g:goods-runtime
)

PEERDIR(
    mapreduce/yt/interface/protos
)

SRCS(
    best_prices.proto
    goods_request.proto
)

END()
