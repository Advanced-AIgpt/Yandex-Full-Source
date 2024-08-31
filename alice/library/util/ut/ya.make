UNITTEST_FOR(alice/library/util)

OWNER(g:alice)

PEERDIR(
    library/cpp/statistics
)

SRCS(
    charchecker_ut.cpp
    min_heap_ut.cpp
    rng_ut.cpp
    variant_ut.cpp
)

END()
