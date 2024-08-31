LIBRARY()

OWNER(
    volobuev
    g:alice_quality
)

PEERDIR(
    alice/protos/data/language
    alice/nlu/libs/embedder
    alice/nlu/libs/item_selector/catboost_item_selector
    alice/nlu/libs/item_selector/composite
    alice/nlu/libs/item_selector/interface
    alice/nlu/libs/item_selector/relevance_based
    alice/nlu/libs/rnn_tagger
)

SRCS(
    item_selector.cpp
)

END()
