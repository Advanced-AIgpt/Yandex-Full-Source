UNITTEST_FOR(alice/hollywood/library/scenarios/music_what_is_playing)

OWNER(
    alexanderplat
    g:hollywood
)

PEERDIR(
    alice/hollywood/library/framework
    alice/hollywood/library/framework/unittest
    alice/hollywood/library/scenarios/music_what_is_playing/proto
    alice/library/analytics/common
    library/cpp/testing/gmock_in_unittest
)


SRCS(
    music_what_is_playing_dispatch_ut.cpp
)

END()
