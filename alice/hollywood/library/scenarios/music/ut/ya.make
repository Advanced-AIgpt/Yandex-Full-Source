UNITTEST_FOR(alice/hollywood/library/scenarios/music)

OWNER(
    g:megamind
    a-square
    vitvlkv
    klim-roma
)

PEERDIR(
    alice/hollywood/library/scenarios/music/proto
    alice/hollywood/library/context
    alice/hollywood/library/testing
    alice/library/json
    library/cpp/testing/gmock_in_unittest
    apphost/lib/service_testing
)

SRCS(
    alice/hollywood/library/scenarios/music/common_ut.cpp
    alice/hollywood/library/scenarios/music/music_play_anaphora_ut.cpp
)

END()
