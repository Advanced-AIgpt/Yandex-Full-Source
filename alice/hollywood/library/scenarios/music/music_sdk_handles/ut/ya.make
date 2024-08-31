UNITTEST_FOR(alice/hollywood/library/scenarios/music/music_sdk_handles)

OWNER(
    sparkle
    g:hollywood
    g:alice
)

DATA(arcadia/alice/hollywood/library/scenarios/music/music_sdk_handles/ut/data/)

SRCS(
    alice/hollywood/library/scenarios/music/music_sdk_handles/requests_helper_ut.cpp
)

PEERDIR(
    alice/hollywood/library/scenarios/music
    alice/hollywood/library/scenarios/music/music_sdk_handles
    alice/library/unittest
    library/cpp/resource
    library/cpp/testing/unittest
)

END()
