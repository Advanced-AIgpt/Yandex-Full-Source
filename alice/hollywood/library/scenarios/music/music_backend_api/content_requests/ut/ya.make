UNITTEST_FOR(alice/hollywood/library/scenarios/music/music_backend_api/content_requests)

OWNER(
    stupnik
    g:hollywood
    g:alice
)

SRCS(
    alice/hollywood/library/scenarios/music/music_backend_api/content_requests/content_requests_ut.cpp
)

PEERDIR(
    alice/hollywood/library/scenarios/music/music_backend_api/content_id
    alice/hollywood/library/scenarios/music/music_backend_api/content_requests
    alice/hollywood/library/scenarios/music/music_backend_api/get_track_url
    alice/hollywood/library/scenarios/music/util
    apphost/lib/service_testing
    library/cpp/testing/unittest
)

END()
