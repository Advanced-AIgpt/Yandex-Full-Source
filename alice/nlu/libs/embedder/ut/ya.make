UNITTEST_FOR(alice/nlu/libs/embedder)

OWNER(
    vl-trifonov
    g:alice_quality
)

PEERDIR(
    alice/nlu/libs/embedder
    library/cpp/containers/comptrie
    library/cpp/testing/unittest
)

SRCS(
    embedder_ut.cpp
)

END()
