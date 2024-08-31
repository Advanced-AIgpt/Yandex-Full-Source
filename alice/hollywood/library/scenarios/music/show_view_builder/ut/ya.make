UNITTEST_FOR(alice/hollywood/library/scenarios/music/show_view_builder)

OWNER(
    sparkle
    g:alice_scenarios
    g:hollywood
)

SRCS(
    alice/hollywood/library/scenarios/music/show_view_builder/show_view_builder_ut.cpp
)

PEERDIR(
    alice/hollywood/library/scenarios/music/music_backend_api/get_track_url
    alice/hollywood/library/scenarios/music/music_request_builder
    alice/hollywood/library/scenarios/music/show_view_builder
    alice/library/unittest
    apphost/lib/service_testing
    library/cpp/testing/unittest
)

END()
