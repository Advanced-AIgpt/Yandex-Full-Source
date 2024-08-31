UNITTEST_FOR(alice/library/music)

OWNER(g:alice)

PEERDIR(
    alice/library/json
    library/cpp/scheme
    alice/library/unittest
)

SRCS(
    common_special_playlists_ut.cpp
)

END()
