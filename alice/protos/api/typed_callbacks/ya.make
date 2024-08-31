PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(
    g:megamind
)

PEERDIR(
    alice/megamind/protos/common
    mapreduce/yt/interface/protos
)

SRCS(
    typed_callback_request.proto
)

END()

RECURSE(
    centaur_main_screen_callback
)
