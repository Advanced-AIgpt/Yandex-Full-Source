LIBRARY(item_selector)

OWNER(
    volobuev
    g:alice_quality
)

PEERDIR(
    alice/nlu/libs/embedder
    alice/nlu/libs/request_normalizer
    alice/nlu/libs/item_selector/interface
    alice/nlu/libs/rnn_tagger
    catboost/libs/model
    contrib/libs/re2
    kernel/lemmer
    library/cpp/json
    library/cpp/langmask
    library/cpp/langs
    library/cpp/resource
    library/cpp/string_utils/levenshtein_diff
    util
)

SRCS(
    catboost_item_selector.cpp
    easy_tagger.cpp
    loader.cpp
    utils.cpp
)

END()

RECURSE_FOR_TESTS(
    ut
)
