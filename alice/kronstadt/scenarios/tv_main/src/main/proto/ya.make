PROTO_LIBRARY(gallery-args-protos)
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(g:paskills)

PEERDIR(
    mapreduce/yt/interface/protos
    alice/protos/data/tv/tags
)

SRCS(
    gallery_args.proto
)

EXCLUDE_TAGS(GO_PROTO PY3_PROTO PY_PROTO CPP_PROTO)

END()
