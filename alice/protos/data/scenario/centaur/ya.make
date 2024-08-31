PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(g:smart_display)

PEERDIR(
    alice/megamind/protos/common
    alice/protos/data/scenario/centaur/my_screen
    alice/protos/data/scenario/centaur/teasers
    alice/protos/data/scenario/weather
    alice/protos/div
    mapreduce/yt/interface/protos
)

SRCS(
    main_screen.proto
    teasers.proto
    upper_shutter.proto
    webview.proto
)

END()
