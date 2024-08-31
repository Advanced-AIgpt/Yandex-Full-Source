LIBRARY()

OWNER(
    vitvlkv
    g:hollywood
    g:alice
)

PEERDIR(
    alice/hollywood/library/analytics_info
    alice/hollywood/library/framework
    alice/hollywood/library/scenarios/music/music_backend_api/music_queue
    alice/hollywood/library/scenarios/music/proto
    alice/library/analytics/common
    alice/library/json
    alice/library/logger
    alice/megamind/protos/scenarios
)

SRCS(
    analytics_info.cpp
)

END()
