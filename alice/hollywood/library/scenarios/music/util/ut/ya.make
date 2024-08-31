UNITTEST_FOR(alice/hollywood/library/scenarios/music/util)

OWNER(
    klim-roma
    g:hollywood
    g:alice
)

SRCS(
    alice/hollywood/library/scenarios/music/util/util_ut.cpp
)

PEERDIR(
    alice/hollywood/library/scenarios/music/music_backend_api/content_id
    alice/hollywood/library/scenarios/music/music_backend_api/music_queue
    alice/hollywood/library/scenarios/music/proto
    alice/hollywood/library/scenarios/music/util

    alice/library/json

    alice/megamind/protos/common
    alice/protos/endpoint/capabilities/bio
    
    library/cpp/testing/unittest
    apphost/lib/service_testing
)

END()
