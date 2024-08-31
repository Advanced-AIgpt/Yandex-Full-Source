LIBRARY()

OWNER(
    antonfn
    g:vh
)

PEERDIR(
    alice/library/video_common
    alice/megamind/protos/common
    alice/megamind/protos/scenarios
    alice/hollywood/library/framework
    library/cpp/json/writer
)

SRCS(
    channel_item.cpp
    util.cpp
)

END()
