LIBRARY()

OWNER(
    dan-anastasev
    g:alice_quality
)

PEERDIR(
    alice/nlu/libs/embedder
    alice/nlu/libs/rnn_tagger
    alice/nlu/libs/sample_features
    library/cpp/json
)

SRCS(
    ellipsis_rewriter.cpp
)

END()

RECURSE_FOR_TESTS(
    ut
)
