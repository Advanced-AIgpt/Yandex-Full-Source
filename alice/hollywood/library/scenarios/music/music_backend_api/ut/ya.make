UNITTEST_FOR(alice/hollywood/library/scenarios/music/music_backend_api)

OWNER(
    stupnik
    g:hollywood
    g:alice
)

DATA(arcadia/alice/hollywood/library/scenarios/music/music_backend_api/ut/data/)

SRCS(
    entity_search_response_parsers_ut.cpp
    find_track_idx_response_parsers_ut.cpp
    get_track_url_handles_ut.cpp
    report_handlers_ut.cpp
    shots_ut.cpp
    vsid_ut.cpp
)

PEERDIR(
    alice/hollywood/library/multiroom
    alice/hollywood/library/scenarios/music/music_backend_api
    alice/hollywood/library/scenarios/music/nlg
    alice/hollywood/library/scenarios/music/util
    alice/hollywood/library/sound
    alice/library/unittest
    apphost/lib/service_testing
    library/cpp/testing/unittest
)

END()

RECURSE_FOR_TESTS(
    ../api_path/ut
    ../content_id/ut
    ../content_parsers/ut
    ../content_requests/ut
    ../get_track_url/ut
    ../music_queue/ut
    ../play_audio/ut
)
