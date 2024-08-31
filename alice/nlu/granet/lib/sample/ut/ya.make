UNITTEST_FOR(alice/nlu/granet/lib/sample)

OWNER(
    samoylovboris
    g:alice_quality
)

SRCS(
    markup_ut.cpp
    sample_ut.cpp
    tag_ut.cpp
)

PEERDIR(
    alice/nlu/libs/ut_utils
)

END()
