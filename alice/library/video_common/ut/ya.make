UNITTEST_FOR(alice/library/video_common)

OWNER(g:alice)

PEERDIR(
    alice/library/unittest
    alice/megamind/library/testing
)

SRCS(
    age_restriction_ut.cpp
    vh_player_ut.cpp
)

END()
