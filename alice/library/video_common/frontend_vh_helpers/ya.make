OWNER(
    g:smarttv
    g:vh
)

LIBRARY()

SRCS(
    frontend_vh_requests.cpp
    util.cpp
    video_item_helper.cpp
)

PEERDIR(
    alice/library/network
    alice/library/video_common/hollywood_helpers
    alice/megamind/protos/common
    alice/megamind/protos/scenarios
)

END()

