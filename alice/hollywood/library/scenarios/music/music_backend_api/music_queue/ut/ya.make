UNITTEST_FOR(alice/hollywood/library/scenarios/music/music_backend_api/music_queue)

OWNER(
    stupnik
    g:hollywood
    g:alice
)

SRCS(
    music_queue_ut.cpp
    what_is_playing_answer_ut.cpp
)

PEERDIR(
    alice/hollywood/library/scenarios/music/music_backend_api/content_id
    alice/hollywood/library/scenarios/music/music_backend_api/music_config
    alice/hollywood/library/scenarios/music/proto
    alice/library/unittest
    library/cpp/testing/unittest
)

END()
