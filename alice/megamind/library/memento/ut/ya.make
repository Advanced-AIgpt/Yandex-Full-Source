OWNER(
    g:megamind
    g-kostin
)

UNITTEST_FOR(alice/megamind/library/memento)

PEERDIR(
    alice/library/unittest
    library/cpp/testing/unittest
)

SRCS(
    memento_ut.cpp
)

END()
