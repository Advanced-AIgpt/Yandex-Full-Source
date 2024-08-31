UNITTEST_FOR(alice/hollywood/library/scenarios/music/music_backend_api/play_audio)

OWNER(
    stupnik
    g:hollywood
    g:alice
)

SRCS(
    alice/hollywood/library/scenarios/music/music_backend_api/play_audio/play_audio_ut.cpp
)

PEERDIR(
    alice/library/proto
    library/cpp/testing/unittest
)

END()
