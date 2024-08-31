LIBRARY()
#PROGRAM()

OWNER(
    smirnovpavel
    g:alice_quality
)

PEERDIR(
    alice/nlu/libs/anaphora_resolver/common
    alice/nlu/libs/anaphora_resolver/matchers/lstm
    alice/nlu/libs/embedder
    alice/nlu/libs/sample_features
)

SRCS(
    dsv_test_reader.cpp
    measure_quality.cpp
)

GENERATE_ENUM_SERIALIZATION(measure_quality.h)

END()

RECURSE_FOR_TESTS(
    ut
)
