PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(
    g:megamind
)

PEERDIR(
    alice/protos/extensions
    mapreduce/yt/interface/protos
)

SRCS(
    centaur_main_screen_callback.proto
)

END()
