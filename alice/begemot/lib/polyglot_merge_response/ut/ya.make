UNITTEST_FOR(alice/begemot/lib/polyglot_merge_response)

OWNER(
    alexanderplat
    g:alice_quality
    g:begemot
)

SRCS(
    alice_response_polyglot_merger_ut.cpp
)

PEERDIR(
    alice/library/proto
    alice/library/unittest
    library/cpp/testing/unittest
)

END()
