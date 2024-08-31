PROTO_LIBRARY()

OWNER(g:alice)

PEERDIR(
    mapreduce/yt/interface/protos
    alice/protos/data/video
    alice/protos/data/app_metrika
)

SRCS(
    gallery.proto
)

END()
