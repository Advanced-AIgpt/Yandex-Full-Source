UNITTEST_FOR(alice/begemot/lib/query_normalizer)

OWNER(
    tolyandex
    g:alice_quality
    g:begemot
)

SRCS(
    query_normalizer_ut.cpp
)

PEERDIR(
    alice/library/json
    library/cpp/testing/unittest
)

END()
