LIBRARY(relevance_based_item_selector)

OWNER(
    volobuev
    g:alice_quality
)

PEERDIR(
    alice/nlu/libs/embedder
    alice/nlu/libs/item_selector/common
    alice/nlu/libs/item_selector/interface
    alice/nlu/libs/tf_nn_model
    alice/nlu/libs/request_normalizer
    library/cpp/json
    util
)

SRCS(
    lstm_relevance_computer.cpp
    relevance_based.cpp
)

END()

RECURSE_FOR_TESTS(
    ut
)
