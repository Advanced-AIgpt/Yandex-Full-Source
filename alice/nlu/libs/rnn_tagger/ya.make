LIBRARY(rnn_tagger)

OWNER(
    the0
    g:alice_quality
    the0
    vl-trifonov
    g:alice_quality
)

PEERDIR(
    alice/nlu/libs/sample_features
    alice/nlu/libs/tf_nn_model
    library/cpp/tf/graph_processor_base
    contrib/libs/tf
    util
)

SRCS(
    rnn_tagger.cpp
)

CFLAGS(
    -Wno-c++11-narrowing
)

END()

RECURSE_FOR_TESTS(
    quality_test
    ut
)
