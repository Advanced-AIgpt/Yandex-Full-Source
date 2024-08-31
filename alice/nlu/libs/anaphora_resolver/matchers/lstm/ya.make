LIBRARY(lstm_anaphora_matcher)

OWNER(
    smirnovpavel
    g:alice_quality
)

PEERDIR(
    alice/nlu/libs/anaphora_resolver/common
    alice/nlu/libs/embedder
    alice/nlu/libs/sample_features
    alice/nlu/libs/tf_nn_model
    library/cpp/json
)

SRCS(
    lstm.cpp
)

END()

RECURSE_FOR_TESTS(
    ut
)
