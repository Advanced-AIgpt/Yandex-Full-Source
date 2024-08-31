PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(g:smart_display)

PEERDIR(
    mapreduce/yt/interface/protos
    alice/protos/data/scenario/dialogovo
    alice/protos/data/scenario/news
    alice/protos/data/scenario/photoframe
    alice/protos/data/scenario/afisha
    alice/protos/data/scenario/weather
)

SRCS(
    teaser_settings.proto
)

END()
