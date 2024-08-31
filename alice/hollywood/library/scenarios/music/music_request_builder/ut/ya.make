UNITTEST_FOR(alice/hollywood/library/scenarios/music/music_request_builder)

OWNER(
    vitvlkv
    g:hollywood
    g:alice
)

SRCS(
    alice/hollywood/library/scenarios/music/music_request_builder/music_request_builder_ut.cpp
    alice/hollywood/library/scenarios/music/music_request_builder/music_request_mode_ut.cpp
)

PEERDIR(
    alice/hollywood/library/scenarios/music/music_request_builder
    library/cpp/regex/pcre
    library/cpp/testing/unittest
)

END()
