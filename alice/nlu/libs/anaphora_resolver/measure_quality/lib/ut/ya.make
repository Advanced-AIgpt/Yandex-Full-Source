UNITTEST_FOR(alice/nlu/libs/anaphora_resolver/measure_quality/lib)

OWNER(
    smirnovpavel
    g:alice_quality
)

PEERDIR(
    library/cpp/resource
    library/cpp/testing/unittest
)

SRCS(
    measure_quality_ut.cpp
)

END()
